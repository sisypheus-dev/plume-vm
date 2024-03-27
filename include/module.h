#ifndef MODULE_H
#define MODULE_H

#include <bytecode.h>
#include <callstack.h>
#include <stack.h>
#include <stdlib.h>
#include <value.h>

typedef Value *Constants;

typedef Value (*Native)(int argc, Value *args);

typedef struct {
  Native *functions;
} NativeTable;

typedef struct {
  size_t instruction_pointer;

  Constants constants;
  Stack *stack;
  CallStack *call_stack;
  NativeTable *natives;
  void **handles;
} Module;

#endif  // MODULE_H