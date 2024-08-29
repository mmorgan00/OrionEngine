#ifndef ASSERTS_H
#define ASSERTS_H

#include <source_location>


#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __builtin_trap()
#endif


void report_assertion_failed(const char *expression, const char *message,
                                  const char *file, int line);

#define OE_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            report_assertion_failed(#expr, "", __FILE__, __LINE__); \
            debugBreak(); \
        } \
    } while(0)

#define OE_ASSERT_MSG(expr, message) \
    do { \
        if (!(expr)) { \
            report_assertion_failed(#expr, message, __FILE__, __LINE__); \
            debugBreak(); \
        } \
    } while(0)

#endif
