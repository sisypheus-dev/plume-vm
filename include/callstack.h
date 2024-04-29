#ifndef CALLSTACK_H
#define CALLSTACK_H

#include <stdlib.h>
#include <value.h>
#include <module.h>
#include <core/error.h>
#include <core/debug.h>
#include <stdio.h>

#define MAX_FRAMES 1024

typedef struct {
  reg instruction_pointer;
  int32_t stack_pointer;
  int32_t base_ptr;
} Frame;

typedef struct {
  int32_t frame_pointer;

  Frame frames[MAX_FRAMES];
} CallStack;

CallStack *callstack_new();
void callstack_free(CallStack *callstack);

static inline Frame pop_frame(Module* mod) {
  Value clos_env = mod->stack->values[mod->base_pointer];

  ASSERT_FMT(get_type(clos_env) == TYPE_FUNCENV, "Expected closure environment got %s", type_of(clos_env));

  reg pc = (int16_t) GET_NTH_ELEMENT(clos_env, 0);
  size_t old_sp = (int16_t) GET_NTH_ELEMENT(clos_env, 1);
  size_t base_ptr = (int16_t) GET_NTH_ELEMENT(clos_env, 2);

  mod->callstack--;

  return (Frame) { pc, old_sp, base_ptr };
}

#endif  // CALLSTACK_H