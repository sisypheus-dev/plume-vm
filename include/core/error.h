#ifndef ERROR_H
#define ERROR_H

#include <stdlib.h>

#define ENABLE_ASSERTIONS 0

#define THROW(message)                       \
  do {                                       \
    printf(message);                         \
    printf(" at %s:%d", __FILE__, __LINE__); \
    printf("\n");                            \
    exit(EXIT_FAILURE);                      \
  } while (0);

#define THROW_FMT(...)   \
  do {                   \
    printf(__VA_ARGS__); \
    printf("\n");        \
    exit(EXIT_FAILURE);  \
  } while (0);

#if ENABLE_ASSERTIONS

#define ASSERT(condition, message) \
  if (!(condition)) {              \
    THROW(message);                \
  }

#define ASSERT_FMT(condition, ...) \
  if (!(condition)) {              \
    THROW_FMT(__VA_ARGS__);        \
  }

#else
#define ASSERT(condition, message)
#define ASSERT_FMT(condition, ...)
#endif

#endif  // ERROR_H