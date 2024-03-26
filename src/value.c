#include <core/error.h>
#include <stdio.h>
#include <string.h>
#include <value.h>

char* type_of(Value v) {
  switch (v.type) {
    case VALUE_INT:
      return "int";
    case VALUE_FLOAT:
      return "float";
    case VALUE_STRING:
      return "string";
    case VALUE_LIST:
      return "[]";
    default:
      THROW_FMT("Unknown value type: %d", v.type);
  }
}

Value equal(Value x, Value y) {
  ASSERT(x.type == y.type, "Cannot compare values of different types");

  switch (x.type) {
    case VALUE_INT:
      return MAKE_INTEGER(x.int_value == y.int_value);
    case VALUE_FLOAT:
      return MAKE_INTEGER(x.float_value == y.float_value);
    case VALUE_STRING:
      return MAKE_INTEGER(strcmp(x.string_value, y.string_value) == 0);
    case VALUE_LIST: {
      if (x.list_value.length != y.list_value.length) {
        return MAKE_INTEGER(0);
      }

      for (int i = 0; i < x.list_value.length; i++) {
        if (!equal(x.list_value.values[i], y.list_value.values[i]).int_value) {
          return MAKE_INTEGER(0);
        }
      }

      return MAKE_INTEGER(1);
    }
    default:
      THROW("Cannot compare values of unknown type");
  }
}