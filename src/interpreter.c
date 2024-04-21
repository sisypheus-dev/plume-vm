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

#define INCREASE_IP_BY(pc, x) (pc += ((x) * 4))
#define INCREASE_IP(pc) INCREASE_IP_BY(pc, 1)

int halt = 0;

typedef Value (*ComparisonFun)(Value, Value);

static inline Value compare_eq_int(Value a, Value b) {
  return MAKE_INTEGER(GET_INT(a) == GET_INT(b));
}

Value compare_eq(Value a, Value b) {
  ValueType a_type = get_type(a);
  ValueType b_type = get_type(b);
  ASSERT_FMT(a_type == b_type, "Cannot compare values of different types: %s and %s", type_of(a), type_of(b));

  switch (a_type) {
    case TYPE_INTEGER:
      return MAKE_INTEGER(a == b);
    case TYPE_FLOAT:
      return MAKE_INTEGER(GET_FLOAT(a) == GET_FLOAT(b));
    case TYPE_STRING: {
      HeapValue* a_ptr = GET_PTR(a);
      HeapValue* b_ptr = GET_PTR(b);

      if (a_ptr->length != b_ptr->length) return MAKE_INTEGER(0);

      return MAKE_INTEGER(strcmp(a_ptr->as_string, b_ptr->as_string) == 0);
    }
    default: 
      THROW_FMT("Cannot compare values of type %s", type_of(a));
  }
}

Value compare_and(Value a, Value b) {
  ASSERT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers");
  return MAKE_INTEGER(GET_INT(a) && GET_INT(b));
}

Value compare_or(Value a, Value b) {
  ASSERT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers");
  return MAKE_INTEGER(GET_INT(a) || GET_INT(b));
}

ComparisonFun comparison_table[] = { NULL, NULL, compare_eq, NULL, NULL, compare_and, compare_or };
ComparisonFun icomparison_table[] = { NULL, NULL, compare_eq_int, NULL, NULL, compare_and, compare_or };

void op_call(Module *module, int32_t *pc, Value callee, size_t argc) {
  ASSERT(module->callstack < MAX_FRAMES, "Call stack overflow");

  HeapValue* list = GET_PTR(callee);
  ASSERT(list->length == 2, "Invalid lambda shape");

  Value ipc = list->as_ptr[0];
  Value local_space = list->as_ptr[1];

  ValueType ipc_type = get_type(ipc);
  ValueType local_space_type = get_type(local_space);

  ASSERT(ipc_type == TYPE_INTEGER, "Invalid ipc type");
  ASSERT(local_space_type == TYPE_INTEGER, "Invalid local space type");

  int32_t new_pc = *pc + 4;
  size_t sp = create_frame(module, new_pc, argc);

  *pc = GET_INT(ipc);
}

void op_native_call(Module *module, int32_t *pc, Value callee, size_t argc) {
  char* fun = GET_NATIVE(callee);

  Value libIdx = stack_pop(module->stack);
  ValueType libIdx_type = get_type(libIdx);
  ASSERT_FMT(libIdx_type == TYPE_INTEGER,
              "Invalid library (for function %s) index", fun);
  int32_t lib_idx = GET_INT(libIdx);
  Value fun_name = stack_pop(module->stack);
  ASSERT_FMT(get_type(fun_name) == TYPE_INTEGER,
              "Invalid library (for function %s)", fun);
  int32_t lib_name = GET_INT(fun_name);

  ASSERT_FMT(module->natives[lib_name].functions != NULL,
              "Library not loaded (for function %s)", fun);

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

  *pc += 4;
}

typedef void (*InterpreterFunc)(Module*, int32_t*, Value, size_t);

InterpreterFunc interpreter_table[] = { NULL, NULL, op_native_call, op_call };

void run_interpreter(Deserialized des) {
  Module* module = des.module;
  int32_t* bytecode = des.instrs;
  int counter = 0;
  int32_t pc = 0;

  #define op bytecode[pc]
  #define i1 bytecode[pc + 1]
  #define i2 bytecode[pc + 2]
  #define i3 bytecode[pc + 3]

  #define UNKNOWN &&case_unknown

  void* jmp_table[] = { 
    &&case_load_local, &&case_store_local, &&case_load_constant, 
    &&case_load_global, &&case_store_global, &&case_return, 
    &&case_compare, &&case_and, &&case_or, &&case_load_native, 
    &&case_make_list, &&case_list_get, &&case_call, 
    &&case_jump_else_rel, UNKNOWN, UNKNOWN, UNKNOWN,
    &&case_make_lambda, &&case_get_index, 
    &&case_special, &&case_jump_rel, &&case_slice, &&case_list_length,
    &&case_halt, &&case_update, &&case_make_mutable, &&case_unmut, 
    &&case_add, &&case_sub, &&case_return_const, &&case_add_const, 
    &&case_sub_const, &&case_jump_else_rel_cmp, UNKNOWN, UNKNOWN, 
    &&case_ijump_else_rel_cmp_constant };

  goto *jmp_table[op];

  case_load_local: {
    Value value = module->stack->values[module->base_pointer + 3 + i1];
    stack_push(module->stack, value);
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_store_local: {
    module->stack->values[module->base_pointer + 3 + i1] = stack_pop(module->stack);
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_load_constant: {
    Value value = module->constants[i1];
    stack_push(module->stack, value);
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_load_global: {
    Value value = module->stack->values[i1];
    stack_push(module->stack, value);
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }
  
  case_store_global: {
    module->stack->values[i1] = stack_pop(module->stack);
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }
  
  case_return: {
    Frame fr = pop_frame(module);
    Value ret = stack_pop(module->stack);

    pc = fr.instruction_pointer;
    module->stack->stack_pointer = fr.stack_pointer;
    module->base_pointer = fr.base_ptr;
    stack_push(module->stack, ret);

    module->callstack--;
    goto *jmp_table[op];
  }
  
  case_compare: {
    Value a = stack_pop(module->stack);
    Value b = stack_pop(module->stack);

    stack_push(module->stack, comparison_table[i1](a, b));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }
  
  case_and: {
    Value a = stack_pop(module->stack);
    Value b = stack_pop(module->stack);

    ASSERT_FMT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers, got %s and %s", type_of(a), type_of(b));

    stack_push(module->stack, MAKE_INTEGER(a && b));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_or: {
    Value a = stack_pop(module->stack);
    Value b = stack_pop(module->stack);

    ASSERT_FMT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers, got %s and %s", type_of(a), type_of(b));

    stack_push(module->stack, MAKE_INTEGER(a || b));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_load_native: {
    Value name = module->constants[i1];
    ASSERT(get_type(name) == TYPE_STRING, "Invalid native function name type");
    stack_push(module->stack, MAKE_INTEGER(i2));
    stack_push(module->stack, MAKE_INTEGER(i3));
    stack_push(module->stack, name);
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }
  
  case_make_list: {
    Value* values = malloc(sizeof(Value) * i1);
    memcpy(values, stack_pop_n(module->stack, i1),
            i1 * sizeof(Value));
    stack_push(module->stack, MAKE_LIST(values, i1));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }
  
  case_list_get: {
    Value list = stack_pop(module->stack);
    ASSERT(get_type(list) == TYPE_LIST, "Invalid list type");
    HeapValue* l = GET_PTR(list);
    ASSERT(i1 < l->length, "Index out of bounds");
    stack_push(module->stack, l->as_ptr[i1]);
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }
  
  case_call: {
    Value callee = stack_pop(module->stack);

    ValueType call_ty = get_type(callee);
    ASSERT(call_ty == TYPE_STRING || call_ty == TYPE_LIST, "Invalid callee type");

    interpreter_table[call_ty](module, &pc, callee, i1);

    goto *jmp_table[op];
  }
  
  case_jump_else_rel: {
    Value value = stack_pop(module->stack);
    ASSERT(get_type(value) == TYPE_INTEGER, "Invalid value type")
    if (GET_INT(value) == 0) {
      INCREASE_IP_BY(pc, i1);
    } else {
      INCREASE_IP(pc);
    }
    goto *jmp_table[op];
  }
  
  case_make_lambda: {
    Value* values = malloc(sizeof(Value) * 2);
    int32_t new_pc = pc + 4;
    values[0] = MAKE_ADDRESS(new_pc);
    values[1] = MAKE_INTEGER(i2);

    Value lambda = MAKE_LIST(values, 2);
    stack_push(module->stack, lambda);
    INCREASE_IP_BY(pc, i1 + 1);

    goto *jmp_table[op];
  }
  
  case_get_index: {
    Value index = stack_pop(module->stack);
    Value list = stack_pop(module->stack);
    ASSERT(get_type(list) == TYPE_LIST, "Invalid list type");
    ASSERT(get_type(index) == TYPE_INTEGER, "Invalid index type");

    HeapValue* l = GET_PTR(list);
    ASSERT(index.as_int64 < l->length, "Index out of bounds");
    stack_push(module->stack, l->as_ptr[index]);
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_special: {
    stack_push(module->stack, MAKE_SPECIAL());
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_jump_rel: {
    INCREASE_IP_BY(pc, i1);
    goto *jmp_table[op];
  }
  
  case_slice: {
    Value list = stack_pop(module->stack);
    ASSERT(get_type(list) == TYPE_LIST, "Invalid list type");
    HeapValue* l = GET_PTR(list);
    HeapValue* new_list = malloc(sizeof(HeapValue));

    new_list->type = TYPE_LIST;
    new_list->length = l->length - i1;
    new_list->as_ptr = malloc(sizeof(Value) * new_list->length);

    memcpy(new_list->as_ptr, &l->as_ptr[i1], (l->length - i1) * sizeof(Value));
    stack_push(module->stack, MAKE_PTR(new_list));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_list_length: {
    Value list = stack_pop(module->stack);
    ASSERT(get_type(list) == TYPE_LIST, "Invalid list type");
    HeapValue* l = GET_PTR(list);
    stack_push(module->stack, MAKE_INTEGER(l->length));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_halt: {
    halt = 1;
    return;
  }

  case_update: {
    Value var = stack_pop(module->stack);
    ASSERT(get_type(var) == TYPE_MUTABLE, "Invalid mutable type");

    HeapValue* l = GET_PTR(var);

    Value value = stack_pop(module->stack);
    memcpy(l->as_ptr, &value, sizeof(Value));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_make_mutable: {
    Value value = stack_pop(module->stack);
    Value* v = malloc(sizeof(Value));
    memcpy(v, &value, sizeof(Value));
    HeapValue* l = malloc(sizeof(HeapValue));
    l->type = TYPE_MUTABLE;
    l->length = 1;
    l->as_ptr = v;
    Value mutable = MAKE_PTR(l);
    stack_push(module->stack, mutable);
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_unmut: {
    Value value = stack_pop(module->stack);
    ASSERT(get_type(value) == TYPE_MUTABLE, "Invalid mutable type");
    stack_push(module->stack, GET_MUTABLE(value));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }
    
  case_add: {
    Value a = stack_pop(module->stack);
    Value b = stack_pop(module->stack);
    
    ASSERT_FMT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers, got %s and %s", type_of(a), type_of(b));

    stack_push(module->stack, MAKE_INTEGER(a + b));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_sub: {
    Value a = stack_pop(module->stack);
    Value b = stack_pop(module->stack);

    ASSERT_FMT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers, got %s and %s", type_of(a), type_of(b));

    stack_push(module->stack, MAKE_INTEGER(a - b));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_return_const: {
    Frame fr = pop_frame(module);
    
    module->stack->stack_pointer = fr.stack_pointer;
    module->base_pointer = fr.base_ptr;

    stack_push(module->stack, module->constants[i1]);

    pc = fr.instruction_pointer;
    module->callstack--;

    goto *jmp_table[op];
  }

  case_add_const: {
    Value a = stack_pop(module->stack);
    Value b = module->constants[i1];

    ASSERT_FMT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers, got %s and %s", type_of(a), type_of(b));

    stack_push(module->stack, MAKE_INTEGER(a + b));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_sub_const: {
    Value a = stack_pop(module->stack);
    Value b = module->constants[i1];

    ASSERT_FMT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers, got %s and %s", type_of(a), type_of(b));
    stack_push(module->stack, MAKE_INTEGER(a - b));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_jump_else_rel_cmp: {
    Value a = stack_pop(module->stack);
    Value b = stack_pop(module->stack);

    Value cmp = comparison_table[i2](a, b);
    ASSERT(get_type(cmp) == TYPE_INTEGER, "Expected integer");

    if (GET_INT(cmp) == 0) {
      INCREASE_IP_BY(pc, i1);
    } else {
      INCREASE_IP(pc);
    }

    goto *jmp_table[op];
  }

  case_ijump_else_rel_cmp_constant: {
    Value a = stack_pop(module->stack);
    Value b = module->constants[i3];

    ASSERT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers");

    Value cmp = icomparison_table[i2](a, b);

    if (GET_INT(cmp) == 0) {
      INCREASE_IP_BY(pc, i1);
    } else {
      INCREASE_IP(pc);
    }

    goto *jmp_table[op];
  }

  case_unknown: {
    THROW_FMT("Unknown opcode: %d", op);
    return;
  }
}

