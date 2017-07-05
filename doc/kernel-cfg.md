KAMProbes require a kernel compiled with the following options:

* for finding required private kernel symbols
CONFIG_KALLSYMS=y
CONFIG_KALLSYMS_ALL=y   // for text_mutex

* for finding subsystems:
CONFIG_DEBUG_INFO=y

-- the script for finding subsystems should also work if you set
CONFIG_DEBUG_INFO_REDUCED=y

All the required options are set in the stock ubuntu kernel.

