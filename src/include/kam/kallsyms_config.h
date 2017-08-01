/**** Notice
 * kallsyms_config.h: kamprobes source code
 *
 * Copyright 2015-2017 The kamprobes owners <lucian.carata@cl.cam.ac.uk>
 *
 * This file is part of the kamprobes open-source project: github.com/lc525/kamprobes;
 * Its licensing is governed by the LICENSE file at the root of the project.
 **/

#ifndef _KAM_KALLSYMS_H_
#define _KAM_KALLSYMS_H_

#include "kam/kallsyms_require.h"

#define KPRIV_INCLUDE "kam/kallsyms_require.h"
#define KSYM_TABLE_NAME(_) PRIV_KSYM_TABLE(_)
#include "ldry/kernel/priv_kallsyms.h"

#endif
