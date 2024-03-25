#ifndef STACK_H
#define STACK_H

#include <stdlib.h>
#include <string.h>
#include <value.h>

#define GLOBALS_SIZE 10
#define MAX_STACK_SIZE 8192
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

extern void stack_push(Stack *stack, Value value);
extern Value stack_pop(Stack *stack);
Value stack_peek(Stack *stack);
void stack_push_n(Stack *stack, Value *values, size_t n);
Value *stack_pop_n(Stack *stack, size_t n);

#endif  // STACK_H