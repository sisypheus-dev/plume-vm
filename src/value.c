#include <core/error.h>
#include <stdio.h>
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