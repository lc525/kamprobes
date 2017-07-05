// When you include this file, it expects you to have defined KPRIV_INCLUDE to
// point to the path of a header file containing private symbol requirements and
// KSYM_TABLE_NAME(_) to replace the name of the X-macro inside that header
// file listing the private symbols (see example below).
//
// For some symbols to be found (global variables), the kernel needs to have
// been compiled with CONFIG_KALLSYMS_ALL=y
//
//
/* example:

   **file: example/require_kallsyms.h **

   #define PRIV_KSYM_TABLE(_)   \
     _(text_poke)               \


   #ifdef _once
   #include <linux/types.h>

   _once void* (*KPRIV(text_poke))(void *addr, const void *opcode, size_t len);
   #endif

   **file: kallsyms.h **
  
   #include "example/require_kallsyms.h"
  
   #define KPRIV_INCLUDE "example/require_kallsyms.h"
   #define KSYM_TABLE_NAME(_) PRIV_KSYM_TABLE(_)
   #include "ldry/kernel/priv_kallsyms.h"
*/

#ifndef _LDRY_PRIV_KALLSYMS_H_

#ifdef _PRIV_KALLSYMS_IMPL_
  #define _once
#else
  #define _once extern
#endif

#include "ldry/macros/argstring.h"
#include "ldry/kernel/macros/debug.h"

#define KPRIV(name) name##_ptr
#include KPRIV_INCLUDE

#undef _once

#include <linux/kallsyms.h>

#define KSYM_INIT(ksym_name)                                        \
{                                                                   \
  KPRIV(ksym_name) = (void *)kallsyms_lookup_name(STR(ksym_name));  \
  if(KPRIV(ksym_name) == NULL) {                                    \
    printk(KERN_ERR STR(ksym_name) ": not found\n");                \
    symbols_not_found++;                                            \
  } else {                                                          \
    debugk(KERN_NOTICE STR(ksym_name) ": %p\n", KPRIV(ksym_name));  \
  }                                                                 \
}                                                                   \

static inline int init_priv_kallsyms(void)
{
  int symbols_not_found = 0;
  KSYM_TABLE_NAME(KSYM_INIT);

  if(symbols_not_found)
    return -ENOSYS;
  else
    return 0;
}

#endif /* _LDRY_PRIV_KALLSYMS_H_ */
