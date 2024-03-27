#include <core/error.h>
#include <stdio.h>
#include <string.h>
#include <value.h>

Value print(int arg_n, Value* args) {
  if (arg_n < 1) THROW("Print expects at least 1 argument");
  Value v = args[0];

  switch (v.type) {
    case VALUE_INT:
      printf("%lld", v.int_value);
      break;
    case VALUE_FLOAT:
      printf("%f", v.float_value);
      break;
    case VALUE_STRING:
      printf("%s", v.string_value);
      break;
    default:
      THROW("Print expects integer, float, or string arguments");
  }

  return MAKE_INTEGER(0);
}

Value add(int arg_n, Value* args) {
  if (arg_n != 2) THROW("Add expects exactly 2 arguments");

  Value a = args[0];
  Value b = args[1];

  ASSERT(a.type == b.type, "Cannot add values of different types");

  Value result;

  switch (a.type) {
    case VALUE_INT:
      result = MAKE_INTEGER(a.int_value + b.int_value);
      break;
    case VALUE_FLOAT:
      result = MAKE_FLOAT(a.float_value + b.float_value);
      break;
    case VALUE_STRING:
      result = MAKE_STRING(strcat(a.string_value, b.string_value));
      break;
    default:
      THROW("Add expects integer or float arguments");
  }

  return result;
}