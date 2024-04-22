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