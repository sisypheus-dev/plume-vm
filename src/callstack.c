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

size_t create_frame(Module* mod, size_t pc, size_t num_locals) {
  size_t old_sp = mod->stack->stack_pointer - num_locals;

  Value* stack = mod->stack->values;
  memmove(&stack[old_sp + 3], &stack[old_sp], num_locals * sizeof(Value));
  
  stack[old_sp] = MAKE_ADDRESS(pc);
  stack[old_sp + 1] = MAKE_ADDRESS(old_sp);
  stack[old_sp + 2] = MAKE_ADDRESS(mod->base_pointer);

  mod->stack->stack_pointer += 3;
  
  mod->base_pointer = old_sp;
  mod->callstack++;
  
  return mod->stack->stack_pointer;
}

Frame pop_frame(Module* mod) {
  Value pc_v = mod->stack->values[mod->base_pointer];
  Value old_sp_v = mod->stack->values[mod->base_pointer + 1];
  Value base_ptr_v = mod->stack->values[mod->base_pointer + 2];

  ASSERT_FMT(pc_v.type == VALUE_ADDRESS, "Expected address, got %d", pc_v.type);
  ASSERT_FMT(old_sp_v.type == VALUE_ADDRESS, "Expected address, got %d", old_sp_v.type);
  ASSERT_FMT(base_ptr_v.type == VALUE_ADDRESS, "Expected address, got %d", base_ptr_v.type);

  size_t pc = pc_v.address_value;
  size_t old_sp = old_sp_v.address_value;
  size_t base_ptr = base_ptr_v.address_value;
  
  return (Frame) { pc, old_sp, base_ptr };
}