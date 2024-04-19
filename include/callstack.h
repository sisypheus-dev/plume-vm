#ifndef CALLSTACK_H
#define CALLSTACK_H

#include <stdlib.h>
#include <value.h>
#include <module.h>

#define MAX_FRAMES 1024

typedef struct {
  size_t instruction_pointer;
  size_t stack_pointer;
  size_t base_ptr;
} Frame;

typedef struct {
  size_t frame_pointer;

  Frame frames[MAX_FRAMES];
} CallStack;

CallStack *callstack_new();
void callstack_free(CallStack *callstack);
size_t create_frame(Module *mod, size_t pc, size_t num_locals);
Frame pop_frame(Module *mod);

extern Frame frame_new(size_t instruction_pointer, size_t stack_pointer,
                       size_t num_locals);

#define CALLSTACK_PUSH(callstack, frame) \
  callstack->frames[callstack->frame_pointer++] = frame
#define CALLSTACK_POP(callstack) callstack->frames[--callstack->frame_pointer]
#define CALLSTACK_PEEK(callstack) \
  callstack->frames[callstack->frame_pointer - 1]

#endif  // CALLSTACK_H