
#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

// Disable debug and trace logging for release builds.
#ifndef NDEBUG
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

typedef enum log_level {
  LOG_LEVEL_FATAL = 0,
  LOG_LEVEL_ERROR = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_INFO = 3,
  LOG_LEVEL_DEBUG = 4,
  LOG_LEVEL_TRACE = 5,
#ifdef NDEBUG
  LOG_LEVEL_VK_ERROR = 6
#endif
} log_level;

// Declaring in a 'macro looking' label, however maintaining
// a 'proper' function call for type checking and so on
void OE_LOG(log_level level, const char* message, ...);

