#ifndef MODULE_H
#define MODULE_H

#include <bytecode.h>
#include <core/library.h>
#include <stack.h>
#include <stdlib.h>
#include <value.h>

typedef Value *Constants;

typedef struct {
  Libraries libraries;
  
  int32_t instr_count;
  int32_t *instrs;

  int32_t base_pointer;
  int32_t callstack;

  Constants constants;
  Stack *stack;
  struct {
    Value (**functions)(int argc, struct Deserialized *des, Value *args);
  } *natives;
  DLL* handles;

  int32_t argc;
  Value *argv;

  GarbageCollector gc;
  int32_t pc;
  Value (*call_function)(struct Deserialized *m, Value callee, int32_t argc, Value* argv);
} Deserialized;

typedef Value (*Native)(int argc, struct Deserialized *m, Value *args);

typedef Deserialized Module;

#endif  // MODULE_H