#include <assert.h>
#include <bytecode.h>
#include <callstack.h>
#include <core/debug.h>
#include <core/error.h>
#include <core/library.h>
#include <interpreter.h>
#include <module.h>
#include <stack.h>
#include <stdio.h>
#include <value.h>

#define INCREASE_IP_BY(mod, x) mod->instruction_pointer += ((x) * 4)
#define INCREASE_IP(mod) INCREASE_IP_BY(mod, 1)

Value unmut(Value mut) {
  ASSERT(mut.type == VALUE_MUTABLE, "Cannot unmut non-mutable value");
  Value* v = mut.mutable_value;
  return *v;
}

int halt = 0;

void execute(Module* module, Opcode op, int64_t i1, int64_t i2, int64_t i3) {
  switch (op) {
    case OP_LoadConstant: {
      Value value = module->constants[i1];
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
      Value name = module->constants[i1];
      ASSERT(name.type == VALUE_STRING, "Invalid native function name type");
      stack_push(module->stack, MAKE_INTEGER(i2));
      stack_push(module->stack, MAKE_INTEGER(i3));
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

        int64_t argc = i1;

        if (module->natives[lib_name].functions[lib_idx] == NULL) {
          void* lib = module->handles[lib_name];
          ASSERT_FMT(lib != NULL, "Library with function %s not loaded", fun);
          Native nfun = get_proc_address(lib, fun);
          ASSERT_FMT(nfun != NULL, "Native function %s not found", fun);
          module->natives[lib_name].functions[lib_idx] = nfun;

          Value* args = stack_pop_n(module->stack, argc);
          Value ret = nfun(argc, module, args);
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
        ASSERT(module->callstack < MAX_FRAMES, "Call stack overflow");

        ValueList list = callee.list_value;
        ASSERT(list.length == 2, "Invalid lambda shape");

        Value ipc = list.values[0];
        Value local_space = list.values[1];

        ASSERT(ipc.type == VALUE_ADDRESS, "Invalid ipc type");
        ASSERT(local_space.type == VALUE_INT, "Invalid local space type");

        size_t sp = create_frame(module, module->instruction_pointer + 4, i1);
        
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
      list.values[0] = MAKE_ADDRESS(module->instruction_pointer + 4);
      list.values[1] = MAKE_INTEGER(i2);

      Value lambda = MAKE_LIST(list);
      stack_push(module->stack, lambda);
      INCREASE_IP_BY(module, i1 + 1);
      break;
    }

    case OP_LoadLocal: {
      Value value = module->stack->values[module->base_pointer + 3 + i1];
      stack_push(module->stack, value);
      INCREASE_IP(module);
      break;
    }

    case OP_StoreLocal: {
      module->stack->values[module->base_pointer + 3 + i1] = stack_pop(module->stack);
      INCREASE_IP(module);
      break;
    }

    case OP_Add: {
      Value x = stack_pop(module->stack);
      Value y = stack_pop(module->stack);

      ASSERT(x.type == y.type && x.type == VALUE_INT, "Cannot add different types");

      stack_push(module->stack, MAKE_INTEGER(x.int_value + y.int_value));
      INCREASE_IP(module);
      break;
    }

    case OP_Sub: {
      Value x = stack_pop(module->stack);
      Value y = stack_pop(module->stack);

      ASSERT(x.type == y.type && x.type == VALUE_INT, "Cannot subtract different types");

      stack_push(module->stack, MAKE_INTEGER(y.int_value - x.int_value));
      INCREASE_IP(module);
      break;
    }

    case OP_StoreGlobal: {
      module->stack->values[i1] = stack_pop(module->stack);
      
      INCREASE_IP(module);
      break;
    }

    case OP_LoadGlobal: {
      Value value = module->stack->values[i1];
      stack_push(module->stack, value);
      INCREASE_IP(module);
      break;
    }

    case OP_Return: {
      Frame fr = pop_frame(module);
      Value ret = stack_pop(module->stack);

      module->instruction_pointer = fr.instruction_pointer;
      module->stack->stack_pointer = fr.stack_pointer;
      module->base_pointer = fr.base_ptr;
      stack_push(module->stack, ret);

      module->callstack--;

      break;
    }

    case OP_JumpIfRel: {
      Value value = stack_pop(module->stack);
      ASSERT(value.type == VALUE_INT, "Invalid value type")
      if (value.int_value == 0) {
        INCREASE_IP_BY(module, i1);
      } else {
        INCREASE_IP(module);
      }
      break;
    }

    case OP_AddConst: {
      Value value = stack_pop(module->stack);
      ASSERT(value.type == VALUE_INT, "Invalid value type");

      Value x = module->constants[i1];
      ASSERT(x.type == VALUE_INT, "Invalid constant type");

      stack_push(module->stack, MAKE_INTEGER(value.int_value + x.int_value));
      INCREASE_IP(module);
      break;
    }

    case OP_SubConst: {
      Value value = stack_pop(module->stack);
      ASSERT(value.type == VALUE_INT, "Invalid value type");

      Value x = module->constants[i1];
      ASSERT(x.type == VALUE_INT, "Invalid constant type");

      stack_push(module->stack, MAKE_INTEGER(value.int_value - x.int_value));
      INCREASE_IP(module);
      break;
    }

    case OP_JumpRel: {
      INCREASE_IP_BY(module, i1);
      break;
    }

    case OP_Compare: {
      Value y = stack_pop(module->stack);
      Value x = stack_pop(module->stack);

      ASSERT(x.type == y.type, "Cannot compare different types");
      if (i1 == EqualTo) {
        stack_push(module->stack, equal(x, y));
      } else if (i1 == GreaterThan) {
        ASSERT(x.type == VALUE_INT || x.type == VALUE_FLOAT,
               "Invalid type for comparison");
        if (x.type == VALUE_INT) {
          stack_push(module->stack, MAKE_INTEGER(x.int_value > y.int_value));
        } else {
          stack_push(module->stack,
                     MAKE_INTEGER(x.float_value > y.float_value));
        }
      } else if (i1 == LessThanOrEqualTo) {
        ASSERT(x.type == y.type && x.type == VALUE_INT, "Invalid type for comparison");
        stack_push(module->stack, MAKE_INTEGER(x.int_value <= y.int_value));
      } else {
        THROW_FMT("Unknown comparison type: %lld", i1);
      }
      INCREASE_IP(module);
      break;
    }

    case OP_ReturnConst: {
      Frame fr = pop_frame(module);
      Value ret = module->constants[i1];

      module->instruction_pointer = fr.instruction_pointer;
      module->stack->stack_pointer = fr.stack_pointer;
      module->base_pointer = fr.base_ptr;

      stack_push(module->stack, ret);

      module->callstack--;
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
      ASSERT(list.type == VALUE_LIST, "Invalid list type");
      ValueList l = list.list_value;
      ASSERT(i1 < l.length, "Index out of bounds");
      stack_push(module->stack, l.values[i1]);
      INCREASE_IP(module);
      break;
    }

    case OP_MakeList: {
      ValueList list;
      list.length = i1;
      list.values = malloc(sizeof(Value) * i1);
      memcpy(list.values, stack_pop_n(module->stack, i1),
             i1 * sizeof(Value));
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
      int64_t idx = i1;
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

    case OP_MakeMutable: {
      Value value = stack_pop(module->stack);
      Value* v = malloc(sizeof(Value));
      memcpy(v, &value, sizeof(Value));
      Value mutable = MAKE_MUTABLE(v);
      stack_push(module->stack, mutable);
      INCREASE_IP(module);
      break;
    }

    case OP_Update: {
      Value var = stack_pop(module->stack);
      ASSERT(var.type == VALUE_MUTABLE, "Invalid mutable type");

      Value value = stack_pop(module->stack);
      memcpy(var.mutable_value, &value, sizeof(Value));
      INCREASE_IP(module);
      break;
    }

    case OP_UnMut: {
      Value value = stack_pop(module->stack);
      ASSERT(value.type == VALUE_MUTABLE, "Invalid mutable type");
      stack_push(module->stack, unmut(value));
      INCREASE_IP(module);
      break;
    }

    case OP_Halt: {
      module->instruction_pointer = -1;
      halt = 1;
      break;
    }

    default: {
      module->instruction_pointer = -1;
      THROW_FMT("Unknown opcode: %d", op);
      break;
    }
  }
}

void run_interpreter(Deserialized des) {
  Module* module = des.module;
  module->instruction_pointer = 0;

  while (halt != 1 && module->instruction_pointer >= 0) {
    Opcode opcode = des.instrs[module->instruction_pointer];
    int64_t i1 = des.instrs[module->instruction_pointer + 1];
    int64_t i2 = des.instrs[module->instruction_pointer + 2];
    int64_t i3 = des.instrs[module->instruction_pointer + 3];

    execute(module, opcode, i1, i2, i3);
  }
}
