#ifndef ERROR_H
#define ERROR_H

#include <stdlib.h>

#define THROW(message)  \
  do {                  \
    printf(message);    \
    printf("\n");       \
    exit(EXIT_FAILURE); \
  } while (0);

#define THROW_FMT(...)   \
  do {                   \
    printf(__VA_ARGS__); \
    printf("\n");        \
    exit(EXIT_FAILURE);  \
  } while (0);

#define ASSERT(condition, message) \
  if (!(condition)) {              \
    THROW(message);                \
  }

#endif  // ERROR_H