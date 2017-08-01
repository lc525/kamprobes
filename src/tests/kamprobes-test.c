/**** Notice
 * kamprobes-test.c: kamprobes source code
 *
 * Copyright 2015-2017 The kamprobes owners <lucian.carata@cl.cam.ac.uk>
 *
 * This file is part of the kamprobes open-source project: github.com/lc525/kamprobes;
 * Its licensing is governed by the LICENSE file at the root of the project.
 **/

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "kam/config.h"
#include "kam/probes.h"
#define _PRIV_KALLSYMS_IMPL_
#include "kam/kallsyms.h"

int kam_is_stopped = 0;

int wq_create_pre(const char *fmt, unsigned int flags, int max_active,
                  void *key, const char *lock_name, ...)
{
  KAM_PRE_ENTRY(subsys);
  va_list args;

  printk("workqueue created: ");
  va_start(args, lock_name);
  printk(fmt, args);
  va_end(args);

  KAM_PRE_RETURN(-1);
}

void wq_create_rtn(int tag)
{

}

static int __init kam_init(void)
{

  int rc;
  kamprobe test_kam;

  // Get addresses for private kernel symbols.
  rc = init_priv_kallsyms();
  if (rc) {
    printk(KERN_ERR "rscfl: cannot find required kernel kallsyms\n");
    return rc;
  }

  rc = kamprobes_init(2);
  if (rc) {
    // Do not fail just because we couldn't set a couple of probes
    // instead, print a warning.
    printk(KERN_WARNING "rscfl: failed to insert %d probes\n", rc);
  }

  test_kam = (kamprobe){.tag = 10,
                        .state = PROBE_NO_HANDLERS,
                        .addr = (u8 *)0xffffffff812e085b,
                        .addr_type = 9,
                        .on_entry = wq_create_pre,
                        .on_return = wq_create_rtn
                       };

  kamprobe_register(&test_kam);
  printk(KERN_NOTICE "rscfl: running\n");
  return 0;


}

static void __exit kam_cleanup(void)
{
}

module_init(kam_init);
module_exit(kam_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lucian Carata <lucian.carata@cl.cam.ac.uk>");
MODULE_VERSION(KAMPROBES_KVERSION);
MODULE_DESCRIPTION(KAMPROBES_DESC);
