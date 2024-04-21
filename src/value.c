#include <core/error.h>
#include <stdio.h>
#include <string.h>
#include <value.h>
#include <nanbox/nanbox.h>

char* type_of(Value value) {
  switch (get_type(value)) {
    case TYPE_INTEGER:
      return "integer";
    case TYPE_FLOAT:
      return "float";
    case TYPE_STRING:
      return "string";
    case TYPE_LIST:
      return "list";
    case TYPE_SPECIAL:
      return "special";
    case TYPE_MUTABLE:
      return "mutable";
    case TYPE_UNKNOWN:
      return "unknown";
  }
}

char* constructor_name(Value v) {
  ASSERT(get_type(v) == TYPE_LIST, "Cannot get constructor name of non-list value");

  HeapValue* arr = GET_PTR(v);
  Value* data = arr->as_ptr;

  ASSERT(arr->length > 0, "Cannot get constructor name of empty value");
  ASSERT(get_type(data[0]) == TYPE_SPECIAL,
         "Cannot get constructor name of non-type value");
  ASSERT(get_type(data[1]) == TYPE_STRING,
         "Constructor name must be a string");

  return GET_STRING(data[1]);
}

Value equal(Value x, Value y) {
  ValueType x_type = get_type(x);
  ASSERT(x_type == get_type(y), "Cannot compare values of different types");

  switch (x_type) {
    case TYPE_INTEGER:
      return MAKE_INTEGER(x == y);
    case TYPE_FLOAT:
      return MAKE_INTEGER(x == y);
    case TYPE_STRING:
      return MAKE_INTEGER(strcmp(GET_STRING(x), GET_STRING(y)) == 0);
    case TYPE_LIST: {
      HeapValue* x_heap = GET_PTR(x);
      HeapValue* y_heap = GET_PTR(y);
      if (x_heap->length != y_heap->length) {
        return MAKE_INTEGER(0);
      }

      Value* x_values = x_heap->as_ptr;
      Value* y_values = y_heap->as_ptr;

      for (int i = 0; i < x_heap->length; i++) {
        if ((int32_t) !equal(x_values[i], y_values[i])) {
          return MAKE_INTEGER(0);
        }
      }

      return MAKE_INTEGER(1);
    }
    case TYPE_SPECIAL: {
      return MAKE_INTEGER(1);
    }
    default:
      THROW("Cannot compare values of unknown type");
  }
}

void native_print(Value value) {
  ValueType val_type = get_type(value);
  switch (val_type) {
    case TYPE_INTEGER:
      printf("%d", (int32_t) value);
      break;
    case TYPE_SPECIAL:
      printf("<special>");
      break;
    case TYPE_FLOAT:
      printf("%f", GET_FLOAT(value));
      break;
    case TYPE_STRING:
      printf("%s", GET_STRING(value));
      break;
    case TYPE_LIST: {
      HeapValue* list = GET_PTR(value);
      printf("[");
      for (int i = 0; i < list->length; i++) {
        native_print(list->as_ptr[i]);
        if (i < list->length - 1) {
          printf(", ");
        }
      }
      printf("]");
      break;
    }
    case TYPE_MUTABLE: {
      printf("<mutable ");
      native_print(GET_MUTABLE(value));
      printf(">");
      break;
    }
    case TYPE_UNKNOWN: default: {
      printf("<unknown>");
      break;
    }
  }
}