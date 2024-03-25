#include <core/error.h>
#include <stack.h>
#include <stdio.h>
#include <stdlib.h>

Stack* stack_new() {
  Stack* stack = malloc(sizeof(Stack));
  stack->stack_pointer = BASE_POINTER;
  return stack;
}

void stack_free(Stack* stack) {
  free(stack->values);
  free(stack);
}

inline void stack_push(Stack* stack, Value value) {
  ASSERT(!(DOES_OVERFLOW(stack, 1)), "Stack overflow on stack push");
  stack->values[stack->stack_pointer++] = value;
}

inline Value stack_pop(Stack* stack) {
  ASSERT(!(DOES_UNDERFLOW(stack, 1)), "Stack underflow");
  return stack->values[--stack->stack_pointer];
}

Value stack_peek(Stack* stack) {
  ASSERT(!(DOES_UNDERFLOW(stack, 1)), "Stack underflow");
  return stack->values[stack->stack_pointer - 1];
}

void stack_push_n(Stack* stack, Value* values, size_t n) {
  ASSERT(!(DOES_OVERFLOW(stack, n)), "Stack overflow on stack push_n");
  memcpy(&stack->values[stack->stack_pointer], values, n * sizeof(Value));
}

Value* stack_pop_n(Stack* stack, size_t n) {
  ASSERT(!(DOES_UNDERFLOW(stack, n)), "Stack underflow");
  return &stack->values[stack->stack_pointer -= n];
}
