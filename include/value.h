#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>
#include <nanbox/nanbox.h>

typedef nanbox_t Value;
typedef int32_t reg;

char *type_of(Value v);
Value equal(Value x, Value y);
char *constructor_name(Value x);
void native_print(Value value);

// The type of the stored value
typedef enum {
  TYPE_INTEGER = 0,
  TYPE_FLOAT,
  TYPE_STRING,
  TYPE_LIST,
  TYPE_SPECIAL,
  TYPE_MUTABLE,
  TYPE_UNKNOWN,
} ValueType;

// Container for arrays
typedef struct {
  Value* data;
  uint32_t length;
} Array;

typedef struct {
  char* data;
  uint32_t length;
} String;

// Container type for values
typedef struct {
  ValueType type;
  uint32_t length;

  union {
    char* as_string;
    Value* as_ptr;
  };
} HeapValue;

#define MAKE_INTEGER(x) nanbox_from_int(x)
#define MAKE_FLOAT(x) nanbox_from_double(x)

static inline Value MAKE_STRING(char* x, uint32_t len) {
  HeapValue* v = malloc(sizeof(HeapValue));
  v->length = len;
  v->type = TYPE_STRING;
  v->as_string = x;
  return nanbox_from_pointer(v);
}

static inline Value MAKE_LIST(Value* x, uint32_t len) {
  HeapValue* v = malloc(sizeof(HeapValue));
  v->length = len;
  v->type = TYPE_LIST;
  v->as_ptr = x;
  return nanbox_from_pointer(v);
}

static inline Value MAKE_MUTABLE(Value x) {
  HeapValue* v = malloc(sizeof(HeapValue));
  v->length = 1;
  v->type = TYPE_MUTABLE;
  v->as_ptr = &x;
  return nanbox_from_pointer(v);
}

#define MAKE_SPECIAL() nanbox_null()
#define MAKE_FLOAT(x) nanbox_from_double(x)
#define MAKE_ADDRESS(x) nanbox_from_int(x)
#define MAKE_NATIVE(x) MAKE_STRING(x, strlen(x))

#define GET_STRING(x) ((HeapValue*)nanbox_to_pointer(x))->as_string
#define GET_LIST(x) ((HeapValue*)nanbox_to_pointer(x))->as_ptr
#define GET_MUTABLE(x) *((Value*)((HeapValue*)nanbox_to_pointer(x))->as_ptr)

#define GET_INT(x) nanbox_to_int(x)
#define GET_FLOAT(x) nanbox_to_double(x)
#define GET_ADDRESS(x) nanbox_to_int(x)
#define GET_NATIVE(x) GET_STRING(x)

#define GET_PTR(x) nanbox_to_pointer(x)

static inline ValueType get_type(Value value) {
  if (nanbox_is_int(value)) {
    return TYPE_INTEGER;
  } else if (nanbox_is_double(value)) {
    return TYPE_FLOAT;
  } else if (nanbox_is_pointer(value)) {
    HeapValue* v = nanbox_to_pointer(value);
    return v->type;
  } else if (nanbox_is_null(value)) {
    return TYPE_SPECIAL;
  } else {
    return TYPE_UNKNOWN;
  }
}

#endif  // VALUE_H