#include <callstack.h>
#include <stdlib.h>
#include <module.h>
#include <string.h>
#include <core/error.h>
#include <stdio.h>
#include <core/debug.h>

CallStack* callstack_new() {
  CallStack* callstack = malloc(sizeof(CallStack));
  callstack->frame_pointer = 0;
  return callstack;
}

void callstack_free(CallStack* callstack) {
  free(callstack->frames);
  free(callstack);
}

size_t create_frame(Module* mod, reg pc, size_t num_locals) {
  size_t old_sp = mod->stack->stack_pointer - num_locals;

  Value* stack = mod->stack->values;
  memmove(&stack[old_sp + 3], &stack[old_sp], num_locals * sizeof(Value));
  
  stack[old_sp] = MAKE_ADDRESS(pc);
  stack[old_sp + 1] = MAKE_ADDRESS((reg) old_sp);
  stack[old_sp + 2] = MAKE_ADDRESS((reg) mod->base_pointer);

  mod->stack->stack_pointer += 3;
  
  mod->base_pointer = old_sp;
  mod->callstack++;
  
  return mod->stack->stack_pointer;
}

Frame pop_frame(Module* mod) {
  DEBUG_STACK_FROM(mod->stack, mod->base_pointer);
  Value pc_v = mod->stack->values[mod->base_pointer];
  Value old_sp_v = mod->stack->values[mod->base_pointer + 1];
  Value base_ptr_v = mod->stack->values[mod->base_pointer + 2];

  ASSERT_FMT(get_type(pc_v) == TYPE_INTEGER, "Expected address, got %s", type_of(pc_v));
  ASSERT_FMT(get_type(old_sp_v) == TYPE_INTEGER, "Expected address, got %s", type_of(old_sp_v));
  ASSERT_FMT(get_type(base_ptr_v) == TYPE_INTEGER, "Expected address, got %s", type_of(base_ptr_v));

  reg pc = GET_ADDRESS(pc_v);
  size_t old_sp = (size_t) GET_ADDRESS(old_sp_v);
  size_t base_ptr = (size_t) GET_ADDRESS(base_ptr_v);

  return (Frame) { pc, old_sp, base_ptr };
}