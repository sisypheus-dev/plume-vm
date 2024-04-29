#ifndef MODULE_H
#define MODULE_H

#include <bytecode.h>
#include <core/library.h>
#include <stack.h>
#include <stdlib.h>
#include <value.h>

typedef Value *Constants;

typedef struct Module {
  int32_t base_pointer;
  int32_t callstack;

  Stack *stack;
  struct {
    Value (**functions)(int argc, struct Module *m, Value *args);
  } *natives;

  int32_t *locals;
  int32_t locals_count;
} Module;

typedef Value (*Native)(int argc, Module *m, Value *args);

typedef struct {
  Module *module;
  Libraries libraries;
  
  int32_t instr_count;
  int32_t *instrs;
} Deserialized;

#endif  // MODULE_H