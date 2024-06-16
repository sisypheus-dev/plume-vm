#include <core/error.h>
#include <stack.h>
#include <stdio.h>
#include <stdlib.h>

Stack* stack_new() {
  Stack* stack = GC_malloc(sizeof(Stack));
  stack->stack_pointer = BASE_POINTER;
  return stack;
}

void stack_free(Stack* stack) {
  free(stack->values);
  free(stack);
}
