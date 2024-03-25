#include <assert.h>
#include <bytecode.h>
#include <callstack.h>
#include <core/debug.h>
#include <core/error.h>
#include <interpreter.h>
#include <module.h>
#include <stack.h>
#include <stdio.h>
#include <value.h>

#define INCREASE_IP_BY(mod, x) mod->instruction_pointer += x
#define INCREASE_IP(mod) INCREASE_IP_BY(mod, 1)

int halt = 0;

void native_print(Value value) {
  switch (value.type) {
    case VALUE_INT:
      printf("%lld", value.int_value);
      break;
    case VALUE_FLOAT:
      printf("%f", value.float_value);
      break;
    case VALUE_STRING:
      printf("%s", value.string_value);
      break;
    case VALUE_LIST: {
      ValueList list = value.list_value;
      printf("[");
      for (int i = 0; i < list.length; i++) {
        native_print(list.values[i]);
        if (i < list.length - 1) {
          printf(", ");
        }
      }
      printf("]");
      break;
    }
    case VALUE_NATIVE:
      printf("<native 0x%x>", value.native_value);
      break;
    case VALUE_ADDRESS:
      printf("0x%x", value.address_value);
      break;
  }
}

void print_frame(Frame fr) {
  printf("Frame: { IP: %ld, SP: %ld, Locals: [", fr.instruction_pointer,
         fr.stack_pointer);
  for (int i = 0; i < fr.num_locals; i++) {
    native_print(fr.locals[i]);
    if (i < fr.num_locals - 1) {
      printf(", ");
    }
  }
  printf("] }\n");
}

void execute(Module* module, Instruction instr) {
  // DEBUG_PRINTLN("Executing instruction %d at IPC %ld", instr.opcode,
  // module->instruction_pointer);

  switch (instr.opcode) {
    case OP_LoadConstant: {
      Value value = module->constants[instr.operand1];
      stack_push(module->stack, value);
      INCREASE_IP(module);
      break;
    }
    case OP_LoadNative: {
      stack_push(module->stack, MAKE_NATIVE(instr.operand1));
      INCREASE_IP(module);
      break;
    }
    case OP_Call: {
      Value callee = stack_pop(module->stack);

      ASSERT(callee.type == VALUE_NATIVE || callee.type == VALUE_LIST,
             "Invalid callee type");

      if (callee.type == VALUE_NATIVE) {
        if (callee.native_value == 0) {
          Value v = stack_pop(module->stack);
          native_print(v);
          printf("\n");
        } else if (callee.native_value == 1) {
          Value x = stack_pop(module->stack);
          Value y = stack_pop(module->stack);
          // printf("%lld - %lld\n", y.int_value, x.int_value);
          stack_push(module->stack, MAKE_INTEGER(y.int_value - x.int_value));
        } else if (callee.native_value == 2) {
          Value x = stack_pop(module->stack);
          Value y = stack_pop(module->stack);
          stack_push(module->stack, MAKE_INTEGER(y.int_value * x.int_value));
        } else if (callee.native_value == 3) {
          Value x = stack_pop(module->stack);
          Value y = stack_pop(module->stack);
          stack_push(module->stack, MAKE_INTEGER(y.int_value == x.int_value));
        } else if (callee.native_value == 4) {
          Value x = stack_pop(module->stack);
          Value y = stack_pop(module->stack);
          stack_push(module->stack, MAKE_INTEGER(y.int_value + x.int_value));
        } else {
          THROW_FMT("Unknown native function, received %d",
                    callee.native_value);
        }
        INCREASE_IP(module);
      } else if (callee.type == VALUE_LIST) {
        ASSERT(module->call_stack->frame_pointer < MAX_FRAMES,
               "Call stack overflow");

        ValueList list = callee.list_value;
        ASSERT(list.length == 2, "Invalid lambda list length");

        Value ipc = list.values[0];
        Value local_space = list.values[1];

        ASSERT(ipc.type == VALUE_ADDRESS, "Invalid ipc type");
        ASSERT(local_space.type == VALUE_INT, "Invalid local space type");

        uint16_t sp = module->stack->stack_pointer;
        Frame fr = frame_new(module->instruction_pointer + 1, sp,
                             local_space.int_value);
        Value* locals = stack_pop_n(module->stack, instr.operand1);

        for (int i = 0; i < instr.operand1; i++) {
          fr.locals[i] = locals[i];
        }

        CALLSTACK_PUSH(module->call_stack, fr);

        module->instruction_pointer = ipc.address_value;
      } else {
        THROW_FMT("Invalid callee type, received %d", callee.type);
      }
      break;
    }
    case OP_MakeLambda: {
      ValueList list;
      list.length = 2;
      list.values = malloc(sizeof(Value) * 2);
      list.values[0] = MAKE_ADDRESS(module->instruction_pointer + 1);
      list.values[1] = MAKE_INTEGER(instr.operand2);

      Value lambda = MAKE_LIST(list);
      stack_push(module->stack, lambda);
      INCREASE_IP_BY(module, instr.operand1 + 1);
      break;
    }

    case OP_LoadLocal: {
      Frame fr = CALLSTACK_PEEK(module->call_stack);
      Value value = fr.locals[instr.operand1];
      stack_push(module->stack, value);
      INCREASE_IP(module);
      break;
    }

    case OP_StoreLocal: {
      Frame fr = CALLSTACK_PEEK(module->call_stack);
      fr.locals[instr.operand1] = stack_pop(module->stack);
      INCREASE_IP(module);
      break;
    }

    case OP_StoreGlobal: {
      module->stack->values[instr.operand1] = stack_pop(module->stack);
      INCREASE_IP(module);
      break;
    }

    case OP_LoadGlobal: {
      Value value = module->stack->values[instr.operand1];
      stack_push(module->stack, value);
      INCREASE_IP(module);
      break;
    }

    case OP_Return: {
      Frame fr = CALLSTACK_POP(module->call_stack);
      Value ret = stack_pop(module->stack);

      module->instruction_pointer = fr.instruction_pointer;
      module->stack->stack_pointer = fr.stack_pointer - 1;
      stack_push(module->stack, ret);
      break;
    }

    case OP_JumpIfRel: {
      Value value = stack_pop(module->stack);
      ASSERT(value.type == VALUE_INT, "Invalid value type")
      // DEBUG_PRINTLN("Jumping if %lld is 1 to %lld", value.int_value,
      //               module->instruction_pointer + instr.operand1);
      if (value.int_value == 0) {
        INCREASE_IP_BY(module, instr.operand1);
      } else {
        INCREASE_IP(module);
      }
      break;
    }

    case OP_Compare: {
      Value x = stack_pop(module->stack);
      Value y = stack_pop(module->stack);
      if (instr.operand1 == 2) {
        stack_push(module->stack, MAKE_INTEGER(y.int_value == x.int_value));
      }
      INCREASE_IP(module);
      break;
    }

    case OP_ListGet: {
      Value list = stack_pop(module->stack);
      ASSERT(list.type == VALUE_LIST, "Invalid list type");
      ValueList l = list.list_value;
      ASSERT(instr.operand1 < l.length, "Index out of bounds");
      stack_push(module->stack, l.values[instr.operand1]);
      INCREASE_IP(module);
      break;
    }

    case OP_MakeList: {
      ValueList list;
      list.length = instr.operand1;
      list.values = malloc(sizeof(Value) * instr.operand1);
      for (int i = 0; i < instr.operand1; i++) {
        list.values[instr.operand1 - i - 1] = stack_pop(module->stack);
      }
      stack_push(module->stack, MAKE_LIST(list));
      INCREASE_IP(module);
      break;
    }

    default: {
      module->instruction_pointer = -1;
      THROW_FMT("Unknown opcode: %d", instr.opcode);
      break;
    }
  }
}

void run_interpreter(Deserialized des) {
  Module* module = des.module;
  Bytecode bytecode = des.bytecode;

  while (halt != 1 &&
         module->instruction_pointer < bytecode.instruction_count &&
         module->instruction_pointer >= 0) {
    Instruction instr = bytecode.instructions[module->instruction_pointer];
    execute(module, instr);
  }
}
