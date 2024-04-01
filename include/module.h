#ifndef MODULE_H
#define MODULE_H

#include <bytecode.h>
#include <callstack.h>
#include <core/library.h>
#include <stack.h>
#include <stdlib.h>
#include <value.h>

typedef Value *Constants;

typedef struct Module {
  size_t instruction_pointer;

  Constants constants;
  Stack *stack;
  CallStack *call_stack;
  struct {
    Value (**functions)(int argc, struct Module *m, Value *args);
  } *natives;
  DLL *handles;
  ValueList args;
} Module;

typedef Value (*Native)(int argc, Module *m, Value *args);

typedef struct {
  Bytecode bytecode;
  Module *module;
  Libraries libraries;
} Deserialized;

#endif  // MODULE_H