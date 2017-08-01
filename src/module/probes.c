/**** Notice
 * probes.c: kamprobes source code
 *
 * Copyright 2015-2017 The kamprobes owners <lucian.carata@cl.cam.ac.uk>
 *
 * This file is part of the kamprobes open-source project: github.com/lc525/kamprobes;
 * Its licensing is governed by the LICENSE file at the root of the project.
 **/

/* KAMprobes (Kernel Advanced Measurement Probes) implementation
 *
 * A KAMprobe is a probe targeted at instrumenting __call sites__ (the place
 * from where a function gets called). An exception is made for system calls,
 * which are explicitly probed
 *
 * The role of the implementation is to write the text section of function
 * call wrappers into a buffer, then modify the existing kernel code to
 * jump into the pre-handler instead of the original function.
 *
 * The code below is not thread-safe, so if you need to call the functions here
 * from multiple threads, first acquire the kamprobes_lock mutex.
 */
#include "kam/probes.h"

#include <linux/cpu.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>

#include "kam/constants.h"
#include "kam/asm2bin.h"
#include "kam/kallsyms_config.h"
#include "ldry/macros/unused.h"
#include "ldry/kernel/macros/debug.h"


//TODO(lc525) align wrapper functions on boundaries? @see gcc -falign-functions

//NOTE(lc525) saving caller-saved registers in the pre-handler is required
//because the compiler assumes it can clobber those inside a function, but we
//need them preserved for when calling the original function.

static unsigned int no_active_probes = 0;

static char *wrapper_start = NULL;
static char *wrapper_end;
static struct mutex UNUSED(kamprobes_lock);
static void mark_probe_active(kamprobe *probe);

int kamprobes_init(int max_probes)
{
  if (wrapper_start == NULL) {
    wrapper_start = KPRIV(module_alloc)(WRAPPER_SIZE * max_probes);

    wrapper_end = wrapper_start;

    debugk("wrapper_start:%p\n", wrapper_start);
  }
  return 0;
}
EXPORT_SYMBOL(kamprobes_init);

u8* resolve_module_addr(module_addr m_addr) {
 //TODO(lc525) build module cache?
 //investigate find_module_sections
 struct module* mod;
 mod = find_module(m_addr.name);
 if (mod != NULL) {
  switch(m_addr.section) {
    case MODULE_CORE:
      return (u8*)(mod->core_layout.base + m_addr.offset);
    case MODULE_INIT:
      return (u8*)(mod->init_layout.base + m_addr.offset);
    default: {
      printk(KERN_NOTICE "kamprobes: unrecognized module section %d\n", m_addr.section);
      return NULL;
    }
  }
 }
 else {
   printk(KERN_NOTICE "kamprobes: kernel module not found %s for setting probe.\n", m_addr.name);
   return NULL;
 }
}

int kamprobe_register(kamprobe* probe)
{
  // There are two types of probes covered by the wrapper:
  // 1. call-site probes, placed on a callq instruction:
  //    * is_call_insn(addr) - returns true
  //    these replace the call to a function to a call to the wrapper (that
  //    later cals the original function)
  // 2. callee probes on functions
  //    * is_call_insn(addr) - returns false
  //    for these, we need __fentry__ support and on execution of the original
  //    function will change the return address so that the function returns
  //    into the bottom part of our wrapper (for the return handler)
  char *wrapper_fp;
  int offset;
  char *target;
  int32_t addr_ptr;
  unsigned char text_poke_isns[CALL_WIDTH];
  u8 *addr;
  int i;

  const char callq_opcode = 0xe8;
  const char jmpq_opcode = 0xe9;
  // test rax, rax
  const char jmpnz_cond[3] = {0x48, 0x85, 0xC0};

  switch (probe->addr_type & ADDR_LOC_MASK) {
    case ADDR_MODULE:
      addr = resolve_module_addr(probe->m_addr);
      break;
    case ADDR_KERNEL:
    default:
      addr = probe->addr;
  }

  // Refuse to register probes on any addr which is not a callq or a noop
  if((!is_call_insn(addr) && !is_noop(addr))) {
    printk(KERN_ERR "Failed to set probe at %p\n", (void *)addr);
    return -EINVAL;
  }

  printk(KERN_ERR "wrapper_start:%p\n", wrapper_end);
  // If *addr is not a call instruction then we assume it is the start
  // of a sys_ function, called though other means. We don't want to rewrite
  // this code, so instead make use of the __fentry__ call placed at the
  // beginning of each function, replacing it with a call to our wrapper.
  // However, we need to save the return address somewhere. We use 8 bytes just
  // before wrapper_fp to do this, and they are initialised to 0 here.
  if (!is_call_insn(addr)) {
    for (i = 0; i < WORD_SZ; i++) {
      emit_insn(&wrapper_end, 0x00);
    }
  }

  // wrapper_fp always points to the start of the current wrapper frame.
  wrapper_fp = wrapper_end;


  // if we're in a type 2 probe (callee), then move the return address from the
  // stack to the reserved space in the wrapper, using r11 as an intermediate
  // register.
  if (!is_call_insn(addr)) {
    debugk("sys at %p, firstb: %#0x, wrapper @ %p\n", addr,
           *addr, wrapper_fp - WORD_SZ);

    // save return address in %r11; we can clobber it as it is caller-saved and
    // not used for other purposes (i.e passing function arguments)
    emit_mov_rsp_r11(&wrapper_end);
    // mov r11 wrapper_fp - 8
    emit_mov_r11_addr(&wrapper_end, wrapper_fp - WORD_SZ);
  } else {
    // Store the probe tag inside a kamprobe-reserved region
    // of the pre-handler stack (where the pre-handler stack will be)
    // leave space for the worst case scenario of all callee-saved registers
    // being saved (7 of them), start saving extra data at the 8th available
    // position. We already know the return address as in the case of type
    // 1 probes (caller), it's probe->addr + CALL_WIDTH
    emit_mov_int_rsp(&wrapper_end, probe->tag, neg_c2(8 * WORD_SZ));
  }
  // replace old return address with address just after the jmp into the
  // pre-handler
  emit_mov_addr_rsp(&wrapper_end, wrapper_end + JMP_WIDTH, 0);

  // Find the target of the callq in the original instruction stream.
  // We need this so that after calling the pre handler we can then call
  // the original function.
  offset = (addr[1]) + (addr[2] << 8) +
           (addr[3] << 16) + (addr[4] << 24) + CALL_WIDTH;
  target = (void *)addr + offset;

  // jump into the pre-handler. we're simulating a call but without pushing
  // a new return address so that the pre-handler can access stack arguments
  // using the same offsets as the original function.
  emit_jump(&wrapper_end, (char *)probe->on_entry);

  // restore return address at the top of the stack; old one was pop-ed
  // by the "on_entry" pre-handler. In this way, we restore the stack state
  // from before the original function was called
  if(!is_call_insn(addr)) {
    emit_push_addr(&wrapper_end, wrapper_fp - WORD_SZ);
  } else {
    emit_push_addr(&wrapper_end, (char*)(addr + CALL_WIDTH));
  }

  // optimisation: if the probe has a on_return handler but the pre_handler
  // returned -1, skip the bottom half of the wrapper (the rtn-handler). The
  // original function returns directly to the original caller, without passing
  // the wrapper. This is done by skipping over the instruction changing the
  // return address to point to the wrapper (the next call to emit_mov_addr_rsp)
  if(probe->on_return != NULL) {
    // test rax, rax
    // jnz MOV_WIDTH [over emit_mov_addr_rsp]
    emit_short_cond_jmp(&wrapper_end, jmpnz_cond, sizeof(jmpnz_cond), MOV_WIDTH);

    // Change the top of the stack so it points at the bottom-half of the wrapper,
    // which is the bit that does the calling of the rtn-handler.
    emit_mov_addr_rsp(&wrapper_end, wrapper_end + JMP_WIDTH + MOV_WIDTH, 0);
  }

  if (is_call_insn(addr)) { // probe on call instruction (in caller)
    // Run the original function.
    // If this is a normal function (not a SyS_) then the code we run is the
    // target of the call instruction that we're replacing. We jump into it as
    // we've already pushed a return address onto the stack.
    switch(probe->addr_type & ADDR_TYPE_MASK){
      case ADDR_OF_CALL:
        emit_jump(&wrapper_end, target);
        break;
      case ADDR_KERNEL_SYSCALL:
        emit_jump(&wrapper_end, target + CALL_WIDTH);
        break;
      case ADDR_INVALID:
      case ADDR_OF_FUNC:
        // we shouldn't get syscalls that come directly from userspace
        // as callqs, nor should we probe invalid addresses.
        BUG();
    }

    if(probe->on_return != NULL) {
      // Set up the return address of the on_return handler.
      // This is set to be the next instruction in the original instruction
      // stream. This means that control flow goes directly back from the return
      // handler to the original caller, without going back through the wrapper.
      // We don't return to where we came from, gaining some efficiency in the
      // process.
      emit_push_addr(&wrapper_end, (char *)(addr + CALL_WIDTH));
    }

  } else { // probe after function entry (in callee)

    // The original code we must now run is not at addr (which is the address of
    // __fentry__ inside the probed function. Rather, it is the next instruction
    // after it.
    emit_jump(&wrapper_end, (char *)(addr + CALL_WIDTH));

    // For the cases where we've changed the return address on the stack, we now
    // need to restore it, from wrapper_fp - 8

    if(probe->on_return != NULL){
      // First, move the return address into a register that we can trash (r11).
      emit_return_address_to_r11(&wrapper_end, wrapper_fp);

      // Now push r11, which contains the return address, onto the stack.
      emit_push_r11(&wrapper_end);
    }
  }

  // As we have setup the stack so that [rsp] points to the address that the
  // retq of the original function would have normally taken us to, we jump
  // (rather than call) into post-handler. A call would not let us skip the
  // wrapper on the return path from the post-handler as it would implicitly
  // push the address just after the call onto the stack.
  if(probe->on_return != NULL) {
    emit_jump(&wrapper_end, (char *)probe->on_return);
  }

  printk(KERN_ERR "wrapper_end:%p\n", wrapper_end);
  // End of setting up the wrapper. Now change the text section to point to it.

  // Store the original address so that we can remove kamprobes.
  mark_probe_active(probe);
  // Ensure we start with a callq opcode, in case of nop-ed insns.

  // Poke the original instruction to point to our wrapper.
  addr_ptr = wrapper_fp - CALL_WIDTH - (char *)addr;

  if(is_call_insn(addr)) { // callq
    text_poke_isns[0] = callq_opcode;
  } else {                // SyS_ call
    text_poke_isns[0] = jmpq_opcode;
  }
  memcpy(text_poke_isns + 1, &addr_ptr, CALL_WIDTH - 1);

  /*get_online_cpus();
   *mutex_lock(KPRIV(text_mutex));
   *KPRIV(text_poke)(addr, &text_poke_isns, CALL_WIDTH);
   *mutex_unlock(KPRIV(text_mutex));
   *put_online_cpus();*/
  return 0;
}
EXPORT_SYMBOL(kamprobe_register);

int kamprobe_unregister(kamprobe *probe){
  if(probe->state == PROBE_ACTIVE) {
    get_online_cpus();
    mutex_lock(KPRIV(text_mutex));
    KPRIV(text_poke)(probe->addr, probe->orig_code, CALL_WIDTH);
    no_active_probes--;
    probe->state = PROBE_REMOVED;
    mutex_unlock(KPRIV(text_mutex));
    put_online_cpus();
  } else {
    return -EEXIST;
  }
  return 0;
}

void kamprobes_free() {
  vfree(wrapper_start);
}

static void mark_probe_active(kamprobe *probe)
{
  int i;
  for (i = 0; i < CALL_WIDTH; i++) {
    probe->orig_code[i] = probe->addr[i];
  }
  probe->state = PROBE_ACTIVE;
  no_active_probes++;
}

void kamprobes_unregister_all(void)
{
  int i;

  get_online_cpus();
  for (i = 0; i < no_active_probes; i++) {
    /*KPRIV(text_poke)(probe_list[i].loc, &probe_list[i].vals, CALL_WIDTH);*/
  }
  debugk(KERN_NOTICE "Unregistered %d probes\n", no_active_probes);
  no_active_probes = 0;
  put_online_cpus();
}
