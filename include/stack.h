#ifndef STACK_H
#define STACK_H

#include <stdlib.h>
#include <string.h>
#include <value.h>

#define GLOBALS_SIZE 1024
#define MAX_STACK_SIZE GLOBALS_SIZE * 32
#define VALUE_STACK_SIZE MAX_STACK_SIZE - GLOBALS_SIZE
#define BASE_POINTER GLOBALS_SIZE

typedef struct {
  Value values[MAX_STACK_SIZE];
  uint16_t stack_pointer;
} Stack;

Stack *stack_new();
void stack_free(Stack *stack);

#define DOES_OVERFLOW(stack, n) stack->stack_pointer + n >= MAX_STACK_SIZE
#define DOES_UNDERFLOW(stack, n) stack->stack_pointer - n < BASE_POINTER
#define stack_push(stack, value) \
  stack->values[stack->stack_pointer++] = value

#define stack_pop(stack) \
  stack->values[--stack->stack_pointer]

#define stack_pop_n(stack, n) \
  &stack->values[stack->stack_pointer -= n]

Value stack_peek(Stack *stack);
void stack_push_n(Stack *stack, Value *values, size_t n);

#endif  // STACK_H