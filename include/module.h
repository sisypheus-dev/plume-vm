#ifndef MODULE_H
#define MODULE_H

#include <bytecode.h>
#include <core/library.h>
#include <stack.h>
#include <stdlib.h>
#include <value.h>

typedef Value *Constants;

typedef struct Module {
  size_t base_pointer;
  size_t callstack;

  Constants constants;
  Stack *stack;
  struct {
    Value (**functions)(int argc, struct Module *m, Value *args);
  } *natives;
  DLL *handles;

  size_t argc;
  Value* argv;

  size_t *locals;
  size_t locals_count;
} Module;

typedef Value (*Native)(int argc, Module *m, Value *args);

typedef struct {
  Module *module;
  Libraries libraries;
  
  size_t instr_count;
  int32_t *instrs;
} Deserialized;

#endif  // MODULE_H