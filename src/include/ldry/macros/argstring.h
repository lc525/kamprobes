#ifndef LDRY_MACROS_ARGSTRING_H_
#define LDRY_MACROS_ARGSTRING_H_

#define ARG_STRING(x) #x
#define CONCAT_(a, b) a##b
#define STR(x) ARG_STRING(x)
#define CONCAT(a, b) CONCAT_(a, b)
#define STR_CONCAT(a, b) ARG_STRING(CONCAT_(a, b))

#endif
