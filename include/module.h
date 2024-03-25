#ifndef MODULE_H
#define MODULE_H

#include <callstack.h>
#include <stack.h>
#include <stdlib.h>
#include <value.h>

typedef Value *Constants;

typedef struct {
  size_t instruction_pointer;

  Constants constants;
  Stack *stack;
  CallStack *call_stack;
} Module;

#endif  // MODULE_H