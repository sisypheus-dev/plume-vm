#include <assert.h>
#include <bytecode.h>
#include <callstack.h>
#include <core/debug.h>
#include <core/error.h>
#include <dlfcn.h>
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
    case VALUE_SPECIAL:
      printf("<special>");
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
      printf("<native %s>", value.native_value);
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
  //               module->instruction_pointer);

  switch (instr.opcode) {
    case OP_LoadConstant: {
      Value value = module->constants[instr.operand1];
      stack_push(module->stack, value);
      INCREASE_IP(module);
      break;
    }

    case OP_Special: {
      stack_push(module->stack, MAKE_SPECIAL());
      INCREASE_IP(module);
      break;
    }

    case OP_ConstructorName: {
      Value v = stack_pop(module->stack);
      // native_print(v);
      // printf(" at IPC %d\n", module->instruction_pointer);
      char* name = constructor_name(v);
      stack_push(module->stack, MAKE_STRING(name));
      INCREASE_IP(module);
      break;
    }

    case OP_TypeOf: {
      Value value = stack_pop(module->stack);
      char* type = type_of(value);
      stack_push(module->stack, MAKE_STRING(type));
      INCREASE_IP(module);
      break;
    }
    case OP_LoadNative: {
      Value name = module->constants[instr.operand1];
      ASSERT(name.type == VALUE_STRING, "Invalid native function name type");
      stack_push(module->stack, MAKE_INTEGER(instr.operand2));
      stack_push(module->stack, MAKE_INTEGER(instr.operand3));
      stack_push(module->stack, MAKE_NATIVE(name.string_value));
      INCREASE_IP(module);
      break;
    }
    case OP_Call: {
      Value callee = stack_pop(module->stack);

      ASSERT(callee.type == VALUE_NATIVE || callee.type == VALUE_LIST,
             "Invalid callee type");

      if (callee.type == VALUE_NATIVE) {
        char* fun = callee.native_value;

        Value libIdx = stack_pop(module->stack);
        ASSERT_FMT(libIdx.type == VALUE_INT,
                   "Invalid library (for function %s) index", fun);
        int lib_idx = libIdx.int_value;
        Value fun_name = stack_pop(module->stack);
        ASSERT_FMT(fun_name.type == VALUE_INT,
                   "Invalid library (for function %s)", fun);
        int lib_name = fun_name.int_value;
        ASSERT_FMT(module->natives[lib_name].functions != NULL,
                   "Library not loaded (for function %s)", fun);

        int64_t argc = instr.operand1;

        if (module->natives[lib_name].functions[lib_idx] == NULL) {
          void* lib = module->handles[lib_name];
          ASSERT_FMT(lib != NULL, "Library with function %s not loaded", fun);
          Native nfun = dlsym(lib, fun);
          ASSERT_FMT(nfun != NULL, "Native function %s not found", fun);
          module->natives[lib_name].functions[lib_idx] = nfun;

          // printf("Calling %s with %lld args\n", fun, argc);
          Value* args = stack_pop_n(module->stack, argc);
          Value ret = nfun(argc, module, args);
          // DEBUG_STACK(module->stack);
          stack_push(module->stack, ret);
        } else {
          Native nfun = module->natives[lib_name].functions[lib_idx];
          ASSERT_FMT(nfun != NULL, "Native function %s not found", fun);

          Value* args = stack_pop_n(module->stack, argc);
          Value ret = nfun(argc, module, args);

          stack_push(module->stack, ret);
        }

        INCREASE_IP(module);
      } else if (callee.type == VALUE_LIST) {
        ASSERT(module->call_stack->frame_pointer < MAX_FRAMES,
               "Call stack overflow");

        ValueList list = callee.list_value;
        ASSERT(list.length == 2, "Invalid lambda shape");

        Value ipc = list.values[0];
        Value local_space = list.values[1];

        ASSERT(ipc.type == VALUE_ADDRESS, "Invalid ipc type");
        ASSERT(local_space.type == VALUE_INT, "Invalid local space type");

        uint16_t sp = module->stack->stack_pointer - instr.operand1;
        Frame fr = frame_new(module->instruction_pointer + 1, sp,
                             local_space.int_value);
        Value* locals = stack_pop_n(module->stack, instr.operand1);

        memcpy(fr.locals, locals, instr.operand1 * sizeof(Value));

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
      // printf("%d < %d\n", module->stack->stack_pointer, MAX_STACK_SIZE);
      module->stack->values[instr.operand1] = stack_pop(module->stack);
      // native_print(module->stack->values[instr.operand1]);
      // printf(" at %d\n", instr.operand1);
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
      module->stack->stack_pointer = fr.stack_pointer;
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

    case OP_JumpRel: {
      INCREASE_IP_BY(module, instr.operand1);
      break;
    }

    case OP_Compare: {
      Value y = stack_pop(module->stack);
      Value x = stack_pop(module->stack);
      ASSERT(x.type == y.type, "Cannot compare different types");
      if (instr.operand1 == EqualTo) {
        stack_push(module->stack, equal(x, y));
      } else if (instr.operand1 == GreaterThan) {
        ASSERT(x.type == VALUE_INT || x.type == VALUE_FLOAT,
               "Invalid type for comparison");
        if (x.type == VALUE_INT) {
          stack_push(module->stack, MAKE_INTEGER(x.int_value > y.int_value));
        } else {
          stack_push(module->stack,
                     MAKE_INTEGER(x.float_value > y.float_value));
        }
      } else {
        THROW_FMT("Unknown comparison type: %lld", instr.operand1);
      }
      INCREASE_IP(module);
      break;
    }

    case OP_GetIndex: {
      Value index = stack_pop(module->stack);
      Value list = stack_pop(module->stack);
      ASSERT(index.type == VALUE_INT, "Invalid index type");
      ASSERT(list.type == VALUE_LIST, "Invalid list type");
      ValueList l = list.list_value;
      ASSERT(index.int_value < l.length, "Index out of bounds");
      stack_push(module->stack, l.values[index.int_value]);
      INCREASE_IP(module);
      break;
    }

    case OP_And: {
      Value x = stack_pop(module->stack);
      Value y = stack_pop(module->stack);
      ASSERT(x.type == VALUE_INT, "Invalid x type");
      ASSERT(y.type == VALUE_INT, "Invalid y type");
      stack_push(module->stack, MAKE_INTEGER(x.int_value && y.int_value));
      INCREASE_IP(module);
      break;
    }

    case OP_ListGet: {
      Value list = stack_pop(module->stack);
      // DEBUG_STACK(module->stack);
      // native_print(list);
      // printf("\n");
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
      memcpy(list.values, stack_pop_n(module->stack, instr.operand1),
             instr.operand1 * sizeof(Value));
      stack_push(module->stack, MAKE_LIST(list));
      INCREASE_IP(module);
      break;
    }

    case OP_ListLength: {
      Value list = stack_pop(module->stack);
      ASSERT(list.type == VALUE_LIST, "Invalid list type");
      ValueList l = list.list_value;
      stack_push(module->stack, MAKE_INTEGER(l.length));
      INCREASE_IP(module);
      break;
    }

    case OP_Slice: {
      int64_t idx = instr.operand1;
      Value list = stack_pop(module->stack);
      ASSERT(list.type == VALUE_LIST, "Invalid list type");
      ValueList l = list.list_value;
      ValueList new_list;

      new_list.length = l.length - idx;
      new_list.values = malloc(sizeof(Value) * new_list.length);

      memcpy(new_list.values, &l.values[idx], (l.length - idx) * sizeof(Value));
      stack_push(module->stack, MAKE_LIST(new_list));
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
