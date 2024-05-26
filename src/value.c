#include <core/error.h>
#include <stdio.h>
#include <string.h>
#include <value.h>

ValueType get_type(Value value) {
  uint64_t signature = value & MASK_SIGNATURE;
  if ((~value & MASK_EXPONENT) != 0) return TYPE_FLOAT;

  // Check for encoded pointer
  if (signature == SIGNATURE_POINTER) {
    HeapValue* ptr = GET_PTR(value);
    return ptr->type;
  }

  // Short encoded types
  switch (signature) {
    case SIGNATURE_NAN:      return TYPE_UNKNOWN;
    case SIGNATURE_SPECIAL:  return TYPE_SPECIAL;
    case SIGNATURE_INTEGER:  return TYPE_INTEGER;
    case SIGNATURE_FUNCTION: return TYPE_FUNCTION;
    case SIGNATURE_FUNCENV:  return TYPE_FUNCENV;
  }

  return TYPE_UNKNOWN;
}

Value MAKE_STRING(char* x) {
  HeapValue* v = gc_malloc(&gc, sizeof(HeapValue));
  v->length = strlen(x);
  v->type = TYPE_STRING;
  v->as_string = x;
  v->refcount = 0;
  return MAKE_PTR(v);
}

Value MAKE_LIST(Value* x, uint32_t len) {
  HeapValue* v = gc_malloc(&gc, sizeof(HeapValue));
  v->length = len;
  v->type = TYPE_LIST;
  v->as_ptr = x;
  v->refcount = 0;
  return MAKE_PTR(v);
}

Value MAKE_MUTABLE(Value x) {
  HeapValue* v =gc_malloc(&gc, sizeof(HeapValue));
  v->length = 1;
  v->type = TYPE_MUTABLE;
  v->as_ptr = &x;
  v->refcount = 0;
  return MAKE_PTR(v);
}


char* type_of(Value value) {
  switch (get_type(value)) {
    case TYPE_INTEGER:
      return "integer";
    case TYPE_FUNCTION:
      return "function";
    case TYPE_FUNCENV:
      return "function_env";
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

      for (uint32_t i = 0; i < x_heap->length; i++) {
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
  if (value == 0) {
    printf("null");
    return;
  }
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
      if (list->length == 0) {
        printf("]");
        break;
      }
      for (uint32_t i = 0; i < list->length; i++) {
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
    case TYPE_FUNCTION: {
      printf("<function>");
      break;
    }
    case TYPE_FUNCENV: {
      printf("<funcenv>");
      break;
    }
    case TYPE_UNKNOWN: default: {
      printf("<unknown>");
      break;
    }
  }
}