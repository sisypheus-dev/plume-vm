#ifndef CALLSTACK_H
#define CALLSTACK_H

#include <stdlib.h>
#include <value.h>
#include <module.h>
#include <core/error.h>
#include <core/debug.h>

#define MAX_FRAMES 1024

typedef struct {
  reg instruction_pointer;
  size_t stack_pointer;
  size_t base_ptr;
} Frame;

typedef struct {
  size_t frame_pointer;

  Frame frames[MAX_FRAMES];
} CallStack;

CallStack *callstack_new();
void callstack_free(CallStack *callstack);

static void create_frame(Module* mod, reg pc, int16_t num_locals, int16_t argc) {
  int16_t old_sp = mod->stack->stack_pointer - argc;
  
  Stack *stack = mod->stack;
  stack_push(stack, MAKE_FUNCENV(pc, old_sp, mod->base_pointer));
  
  mod->base_pointer = stack->stack_pointer - 1;
  mod->locals[mod->locals_count++] = num_locals;
  mod->callstack++;
  
  return;
}

static inline Frame pop_frame(Module* mod) {
  Value clos_env = mod->stack->values[mod->base_pointer];

  ASSERT_FMT(get_type(clos_env) == TYPE_CLOSENV, "Expected closure environment got %s", type_of(clos_env));

  reg pc = (int16_t) GET_NTH_ELEMENT(clos_env, 0);
  size_t old_sp = (int16_t) GET_NTH_ELEMENT(clos_env, 1);
  size_t base_ptr = (int16_t) GET_NTH_ELEMENT(clos_env, 2);

  mod->locals_count--;
  mod->callstack--;

  return (Frame) { pc, old_sp, base_ptr };
}

#endif  // CALLSTACK_H