
#include "engine/logger.h"
#include "engine/asserts.h"

#include <stdio.h>
#include <stdarg.h>

void OE_LOG(log_level level, const char* fmt_str, ...) {

    // Formatted so things look nice
    const char* level_strings[] = {
      "[FATAL]:   ",
      "[ERROR]:   ", 
      "[WARN]:    ", 
      "[INFO]:    ", 
      "[DEBUG]:   ", 
      "[TRACE]:   ", 
#ifdef NDEBUG
      "[VK_ERROR]:"
#endif
};

    // TODO: Color output?
    printf("%s", level_strings[level]);
  va_list args; // Declare a variable of type va_list
  va_start(args, fmt_str); // Initialize the args variable with the last known fixed argument

  // Use vprintf to handle the variable arguments
  vprintf(fmt_str, args);

  va_end(args); // Clean up the va_list
}

void report_assertion_failed(const char *expression, const char *message,
                             const char *file, int line) {
  OE_LOG(LOG_LEVEL_FATAL,
             "Assertion failure: %s, message: '%s', in file: %s, line: %d\n",
             expression, message, file, line);
}
