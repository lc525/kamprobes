#ifndef _KAM_KALLSYMS_H_
#define _KAM_KALLSYMS_H_

#include "kam/kallsyms_require.h"

#define KPRIV_INCLUDE "kam/kallsyms_require.h"
#define KSYM_TABLE_NAME(_) PRIV_KSYM_TABLE(_)
#include "ldry/kernel/priv_kallsyms.h"

#endif
