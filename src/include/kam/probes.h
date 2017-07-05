#ifndef _RSCFL_KAMPROBES_H_
#define _RSCFL_KAMPROBES_H_

#include <linux/types.h>
#include "kam/constants.h"
#include "ldry/macros/unused.h"

typedef enum {
    ADDR_INVALID        = 0,
    ADDR_OF_CALL        = 1,
    ADDR_OF_FUNC        = 2,
    ADDR_KERNEL_SYSCALL = 3
    //If modifying this enum, make sure that the value is representable on
    //ADDR_TYPE_BITS bits (MAX_VALUE=2^ADDR_TYPE_BITS); the other bits are
    //reserved for custom-defined subsystem probe types
} addr_type;
#define ADDR_TYPE_BITS 3
#define ADDR_TYPE_MASK 7

typedef enum {
  PROBE_NO_HANDLERS,
  PROBE_DEFAULT_HANDLERS,
  PROBE_INIT_DONE,
  PROBE_ACTIVE,
  PROBE_REMOVED
} kamprobe_state;

struct kamprobe {
  int tag;
  kamprobe_state state;

  u8 *addr;
  char addr_type;
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
EXPORT_SYMBOL(kamprobes_init);

int kamprobe_register(kamprobe *probe);
EXPORT_SYMBOL(kamprobe_register);

int kamprobe_unregister(kamprobe *probe);
void kamprobes_unregister_all(void);
void kamprobes_free(void);


#endif

