#ifndef LDRY_KERNEL_MACROS_DEBUG_H_
#define LDRY_KERNEL_MACROS_DEBUG_H_

#ifndef NDEBUG
#define debugk(format, ...) printk(format, ##__VA_ARGS__)
#else
#define debugk(format, ...)
#endif

#endif
