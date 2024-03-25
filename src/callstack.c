#include <callstack.h>
#include <stdlib.h>

CallStack* callstack_new() {
  CallStack* callstack = malloc(sizeof(CallStack));
  callstack->frame_pointer = 0;
  return callstack;
}

void callstack_free(CallStack* callstack) {
  free(callstack->frames);
  free(callstack);
}

inline Frame frame_new(size_t instruction_pointer, size_t stack_pointer,
                       size_t num_locals) {
  Frame frame;
  frame.instruction_pointer = instruction_pointer;
  frame.stack_pointer = stack_pointer;
  frame.num_locals = num_locals;
  frame.locals = malloc(num_locals * sizeof(Value));
  return frame;
}