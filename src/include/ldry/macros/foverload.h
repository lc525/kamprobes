#ifndef LDRY_MACROS_FOVERLOAD_H_
#define LDRY_MACROS_FOVERLOAD_H_

#define _ARG11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, ...) _10
#define HAS_NO_COMMA(...) _ARG11(__VA_ARGS__, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1)
#define _TRIGGER_ARGS(...) ,

#define HAS_ONE_ARG(...) HAS_NO_COMMA(_TRIGGER_ARGS __VA_ARGS__ (/* empty */))

#define VARGS_(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define VARGS_NR(...) VARGS_(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, HAS_ONE_ARG(__VA_ARGS__), HAS_ONE_ARG(__VA_ARGS__))

#endif
