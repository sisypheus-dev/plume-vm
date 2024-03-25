#ifndef BYTECODE_H
#define BYTECODE_H

#include <module.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {
  OP_LoadLocal,
  OP_StoreLocal,
  OP_LoadConstant,
  OP_Null,
  OP_LoadGlobal,
  OP_StoreGlobal,
  OP_Return,
  OP_Compare,
  OP_And,
  OP_Or,
  OP_LoadNative,
  OP_MakeList,
  OP_ListGet,
  OP_Call,
  OP_Jump,
  OP_JumpIf,
  OP_JumpIfRel,
  OP_TypeOf,
  OP_ConstructorName,
  OP_Phi,
  OP_MakeLambda
} Opcode;

typedef struct {
  Opcode opcode;
  int64_t operand1;
  int64_t operand2;
} Instruction;

typedef struct {
  Instruction *instructions;
  int64_t instruction_count;
} Bytecode;

typedef struct {
  Bytecode bytecode;
  Module *module;
} Deserialized;

#endif  // BYTECODE_H