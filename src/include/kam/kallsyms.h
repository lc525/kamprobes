#ifndef _KAM_KALLSYMS_H_
#define _KAM_KALLSYMS_H_

#include "kam/external/require_kallsyms.h"

#define KPRIV_INCLUDE "kam/external/require_kallsyms.h"
#define KSYM_TABLE_NAME(_) PRIV_KSYM_TABLE(_)
#include "ldry/kernel/priv_kallsyms.h"

#endif
