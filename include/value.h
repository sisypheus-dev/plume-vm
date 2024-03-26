#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>

typedef enum {
  VALUE_INT = 0,
  VALUE_FLOAT = 1,
  VALUE_STRING = 2,
  VALUE_ADDRESS = 3,
  VALUE_NATIVE = 4,
  VALUE_LIST = 5
} ValueType;

typedef struct {
  struct Value *values;
  uint16_t length;
} ValueList;

typedef struct Value {
  ValueType type;
  union {
    int64_t int_value;
    double float_value;
    char *string_value;
    uint16_t address_value;
    uint8_t native_value;
    ValueList list_value;
  };
} Value;

#define MAKE_INTEGER(value) ((Value){.type = VALUE_INT, .int_value = value})
#define MAKE_FLOAT(value) ((Value){.type = VALUE_FLOAT, .float_value = value})
#define MAKE_LIST(value) ((Value){.type = VALUE_LIST, .list_value = value})
#define MAKE_STRING(value) \
  ((Value){.type = VALUE_STRING, .string_value = value})
#define MAKE_ADDRESS(value) \
  ((Value){.type = VALUE_ADDRESS, .address_value = value})
#define MAKE_NATIVE(value) \
  ((Value){.type = VALUE_NATIVE, .native_value = value})

char *type_of(Value v);

#endif  // VALUE_H