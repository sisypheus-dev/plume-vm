#ifndef MODULE_H
#define MODULE_H

#include <bytecode.h>
#include <core/library.h>
#include <stack.h>
#include <stdlib.h>
#include <value.h>

typedef Value *Constants;

typedef struct Module {
  size_t instruction_pointer;
  size_t base_pointer;
  size_t callstack;

  Constants constants;
  Stack *stack;
  struct {
    Value (**functions)(int argc, struct Module *m, Value *args);
  } *natives;
  DLL *handles;
  ValueList args;
} Module;

typedef Value (*Native)(int argc, Module *m, Value *args);

typedef struct {
  Module *module;
  Libraries libraries;
  
  size_t instr_count;
  int64_t *instrs;
} Deserialized;

#endif  // MODULE_H