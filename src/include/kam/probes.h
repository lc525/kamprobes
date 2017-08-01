/**** Notice
 * probes.h: kamprobes source code
 *
 * Copyright 2015-2017 The kamprobes owners <lucian.carata@cl.cam.ac.uk>
 *
 * This file is part of the kamprobes open-source project: github.com/lc525/kamprobes;
 * Its licensing is governed by the LICENSE file at the root of the project.
 **/

#ifndef _RSCFL_KAMPROBES_H_
#define _RSCFL_KAMPROBES_H_

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>

#include "kam/constants.h"
#include "ldry/macros/unused.h"
#include "ldry/macros/bits.h"

typedef enum {
    ADDR_INVALID        = 0,
    ADDR_OF_CALL        = 1,
    ADDR_OF_FUNC        = 2,
    ADDR_KERNEL_SYSCALL = 3
    //If modifying this enum, make sure that the value is representable on
    //ADDR_TYPE_BITS bits (MAX_VALUE=2^ADDR_TYPE_BITS); the other bits are
    //reserved for custom-defined subsystem probe types
} addr_type;

typedef enum {
  ADDR_KERNEL = 0,
  ADDR_MODULE = 1
} addr_loc;

/*
The probe type is a 1 byte field where the bits have the following meaning:
- bits: 7654 | 3 | 210
- role: SSSS | L | TTT

The codification of roles is as follows:
    S: Subtype that can be arbitrarily set by custom subsystems - each custom
       subsystem can set 16 subtypes of probes, with each subtype defining
       particular entry and return handler functions;
    L: Location of probed address (1 bit) - defines whether the probe is on a
       location in the kernel itself or inside a module -- module probes need
       their addresses to be computed at runtime, we only store a pre-relocation
       offset in the list;
    T: Address type (3 bits) - identifies the type of address stored in the
       probe, with possible values being given by the addr_type enum. The
       mechanisms of how kamprobes work vary depending on the address type, with
       ADDR_OF_CALL probing caller locations and ADDR_OF_FUNC,
       ADDR_KERNEL_SYSCALL probing the callee (and being triggered from within
       the stack frame of the called function)

*/
#define ADDR_TYPE_BITS 3 // 3 bits dedicated to addr type
#define ADDR_LOC_BITS 1  // 1 bit  dedicated to probe location
                         //    the bit will be 0 for kernel probes
                         //    the bit will be 1 for module probes
#define ADDR_FIXED_BITS (ADDR_TYPE_BITS + ADDR_LOC_BITS)
#define ADDR_TYPE_MASK SET_ALL_BITS(ADDR_TYPE_BITS)
#define ADDR_LOC_MASK SET_ALL_BITS(ADDR_LOC_BITS) << ADDR_TYPE_BITS
#define SUBSYS_PROBE_TYPE(subtype, loc, addr_type)   \
        (subtype << ADDR_FIXED_BITS) | (loc << ADDR_TYPE_BITS) | addr_type

typedef enum {
  PROBE_NO_HANDLERS,
  PROBE_DEFAULT_HANDLERS,
  PROBE_INIT_DONE,
  PROBE_ACTIVE,
  PROBE_REMOVED
} kamprobe_state;

typedef enum {
  MODULE_INIT,
  MODULE_CORE
} module_section;

struct module_addr {
  unsigned int offset;
  char name[MODULE_NAME_LEN];
  module_section section;
};
typedef struct module_addr module_addr;

struct kamprobe {
  int tag;
  void *tag_data;
  kamprobe_state state;

  char addr_type;
  union {
    u8 *addr;
    module_addr m_addr; // the probe is set on a kernel module
  };
  unsigned char orig_code[CALL_WIDTH];

  unsigned char *probe_code;

  void *on_entry;
  void *on_return;
};
typedef struct kamprobe kamprobe;


/*
 * Definitions for the probe-handler api
 */

/*
 * Save the registers that normally store function arguments so that they are
 * passed unchanged to the original function. The pre-handler can modify
 * fields inside structures or anything that gets allocated on the stack, but
 * not directly arguments passed through registers (so that the modified values
 * are seen by the original function).
 *
 * Modifying arguments passed by registers is still possible, but you will need
 * to know the ABI and write your own KAM_PRE_{SAVE, RESTORE} macros. By
 * default, we save and restore all caller-saved registers. When writing your
 * own, please mind the following note:
 *
 * NOTE: make sure to align the stack on 16 byte boundary (even nr. of pushes)
 * This is because some MMX instructions/registers fail when operating on
 * a stack that is not aligned in this way.
 */
#define KAM_PRE_SAVE       \
  asm("push %rdi;");       \
  asm("push %rsi;");       \
  asm("push %rdx;");       \
  asm("push %rcx;");       \
  asm("push %r8;");        \
  asm("push %r9;");        \
  asm("push %r10;");       \
  asm("push %rax;")  /* for align */      \

#define KAM_PRE_RESTORE   \
  asm("pop %rax;");  /* for align */      \
  asm("pop %r10;");       \
  asm("pop %r9;");        \
  asm("pop %r8;");        \
  asm("pop %rcx;");       \
  asm("pop %rdx;");       \
  asm("pop %rsi;");       \
  asm("pop %rdi;")        \

#define CALLEE_SAVE_NO 7
/* In the pre-handler, reserve stack (size in bytes) as follows:
 *                           Callee saved (max)  + tag
                                      |             |                       */
#define RES_STACK_SZ   CALLEE_SAVE_NO * WORD_SZ  +  4

#define KAM_PRE_ENTRY(tag_var)                                               \
volatile char __rscfl_reserved_stack[RES_STACK_SZ];                          \
asm __volatile__("" :: "m" (__rscfl_reserved_stack)); /* prevent optim */    \
{                                                                            \
  KAM_PRE_SAVE;  {                                                           \
  char *__rscfl_reserved_rbp = __builtin_frame_address(0);                   \
  uint32_t *UNUSED(tag_var) = (uint32_t*)(__rscfl_reserved_rbp -             \
                                  (CALLEE_SAVE_NO + 1) * WORD_SZ)            \


#define PROBE_END             \
}

#define KAM_PRE_RETURN(ret)   \
  } KAM_PRE_RESTORE;          \
  return ret;                 \
  PROBE_END

#define KAM_RTN_ENTRY()       \
  asm("push %rax;")           \

#define KAM_RTN_RETURN()      \
  asm("pop %rax;")            \


int kamprobes_init(int max_probes);
int kamprobe_register(kamprobe *probe);

int kamprobe_unregister(kamprobe *probe);
void kamprobes_unregister_all(void);
void kamprobes_free(void);


#endif
