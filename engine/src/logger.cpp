
#include "engine/logger.h"


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
    };
#endif

    // TODO: Color output?
    printf("%s", level_strings[level]);
  va_list args; // Declare a variable of type va_list
  va_start(args, fmt_str); // Initialize the args variable with the last known fixed argument

  // Use vprintf to handle the variable arguments
  vprintf(fmt_str, args);

  va_end(args); // Clean up the va_list
}
