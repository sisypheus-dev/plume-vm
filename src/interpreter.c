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

Value compare_eq(Value a, Value b) {
  ValueType a_type = get_type(a);
  ASSERT_FMT(a_type == get_type(b), "Cannot compare values of different types: %s and %s", type_of(a), type_of(b));

  switch (a_type) {
    case TYPE_INTEGER:
      return MAKE_INTEGER(a == b);
    case TYPE_FLOAT:
      return MAKE_INTEGER(GET_FLOAT(a) == GET_FLOAT(b));
    case TYPE_STRING: {
      HeapValue* a_ptr = GET_PTR(a);
      HeapValue* b_ptr = GET_PTR(b);

      size_t a_len = strlen(a_ptr->as_string);
      size_t b_len = strlen(b_ptr->as_string);

      if (a_len != b_len) return MAKE_INTEGER(0);

      return MAKE_INTEGER(strcmp(a_ptr->as_string, b_ptr->as_string) == 0);
    }
    case TYPE_FUNCTION: case TYPE_FUNCENV: case TYPE_MUTABLE: {
      return MAKE_INTEGER(a == b);
    }
    case TYPE_LIST: {
      HeapValue* a_ptr = GET_PTR(a);
      HeapValue* b_ptr = GET_PTR(b);

      if (a_ptr->length != b_ptr->length) return MAKE_INTEGER(0);

      for (uint32_t i = 0; i < a_ptr->length; i++) {
        if (!compare_eq(a_ptr->as_ptr[i], b_ptr->as_ptr[i])) return MAKE_INTEGER(0);
      }

      return MAKE_INTEGER(1);
    }
    case TYPE_SPECIAL: return MAKE_INTEGER(1);
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

Value compare_gt(Value a, Value b) {
  ASSERT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers");
  return MAKE_INTEGER(GET_INT(a) > GET_INT(b));
}

ComparisonFun comparison_table[] = { NULL, compare_gt, compare_eq, NULL, NULL, compare_and, compare_or };

void op_call(Module *module, int32_t *pc, Value callee, int32_t argc) {
  ASSERT_FMT(module->callstack < MAX_FRAMES, "Call stack overflow, reached %d", module->callstack);

  int16_t ipc = (int16_t) (callee & MASK_PAYLOAD_INT);
  int16_t local_space = (int16_t) ((callee >> 16) & MASK_PAYLOAD_INT);
  int16_t old_sp = module->stack->stack_pointer - argc;

  module->stack->stack_pointer += local_space - argc;

  int32_t new_pc = *pc + 4;

  stack_push(module->stack, MAKE_FUNCENV(new_pc, old_sp, module->base_pointer));
  
  module->base_pointer = module->stack->stack_pointer - 1;
  module->callstack++;

  *pc = ipc;
}

void op_native_call(Module *module, int32_t *pc, Value callee, int32_t argc) {
  char* fun = GET_NATIVE(callee);

  Value libIdx = stack_pop(module->stack);
  ASSERT_FMT(get_type(libIdx) == TYPE_INTEGER,
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

typedef void (*InterpreterFunc)(Module*, int32_t*, Value, int32_t);

InterpreterFunc interpreter_table[] = { op_native_call, op_call };

void run_interpreter(Deserialized des) {
  Module* module = des.module;
  Constants constants = module->constants;
  int32_t* bytecode = des.instrs;
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
    &&case_jump_else_rel, &&case_type_of, UNKNOWN, UNKNOWN,
    &&case_make_lambda, &&case_get_index, 
    &&case_special, &&case_jump_rel, &&case_slice, &&case_list_length,
    &&case_halt, &&case_update, &&case_make_mutable, &&case_unmut, 
    &&case_add, &&case_sub, &&case_return_const, &&case_add_const, 
    &&case_sub_const, &&case_jump_else_rel_cmp, &&case_ijump_else_rel_cmp, 
    &&case_jump_else_rel_cmp_constant, 
    &&case_ijump_else_rel_cmp_constant, &&case_call_global,
    &&case_call_local, &&case_make_and_store_lambda, &&case_mul,
    &&case_mul_const, &&case_return_unit };

  goto *jmp_table[op];

  case_load_local: {
    int32_t locals = module->base_pointer;

    Value value = module->stack->values[locals + i1];
    stack_push(module->stack, value);
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_store_local: {
    int32_t locals = module->base_pointer;
    module->stack->values[locals + i1] = stack_pop(module->stack);
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_load_constant: {
    Value value = constants[i1];
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

    module->stack->stack_pointer = fr.stack_pointer;
    module->base_pointer = fr.base_ptr;
    stack_push(module->stack, ret);

    pc = fr.instruction_pointer;
    goto *jmp_table[op];
  }
  
  case_compare: {
    Value a = stack_pop(module->stack);
    Value b = stack_pop(module->stack);

    stack_push(module->stack, comparison_table[i1](b, a));
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
    Value name = constants[i1];
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
    uint32_t idx = GET_INT(i1);
    ASSERT_FMT(get_type(list) == TYPE_LIST, "Invalid list type at IPC %d", pc / 4);
    HeapValue* l = GET_PTR(list);
    ASSERT(idx < l->length, "Index out of bounds");
    stack_push(module->stack, l->as_ptr[idx]);
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }
  
  case_call: {
    Value callee = stack_pop(module->stack);

    ASSERT(IS_FUN(callee) || IS_PTR(callee), "Invalid callee type");
  
    interpreter_table[(callee & MASK_SIGNATURE) == SIGNATURE_FUNCTION](module, &pc, callee, i1);

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
  
  case_type_of: {
    Value value = stack_pop(module->stack);
    stack_push(module->stack, MAKE_STRING(type_of(value), strlen(type_of(value))));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_make_lambda: {
    int32_t new_pc = pc + 4;
    Value lambda = MAKE_FUNCTION(new_pc, i2);

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
    uint32_t idx = GET_INT(index);
  
    ASSERT(idx < l->length, "Index out of bounds");
    stack_push(module->stack, l->as_ptr[idx]);
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
    ASSERT_FMT(get_type(list) == TYPE_LIST, "Invalid list type at IPC %d", pc / 4);
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

    stack_push(module->stack, MAKE_INTEGER(b - a));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_return_const: {
    Frame fr = pop_frame(module);
    module->stack->stack_pointer = fr.stack_pointer;
    module->base_pointer = fr.base_ptr;

    stack_push(module->stack, constants[i1]);

    pc = fr.instruction_pointer;

    goto *jmp_table[op];
  }

  case_add_const: {
    Value a = stack_pop(module->stack);
    Value b = constants[i1];

    ASSERT_FMT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers, got %s and %s", type_of(a), type_of(b));

    stack_push(module->stack, MAKE_INTEGER(a + b));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_sub_const: {
    Value a = stack_pop(module->stack);
    Value b = constants[i1];

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

  case_ijump_else_rel_cmp: {
    Value a = stack_pop(module->stack);
    Value b = stack_pop(module->stack);

    void* icomparison_table[] = { 
      UNKNOWN, UNKNOWN, &&icmp_eq, UNKNOWN, 
      UNKNOWN, &&icmp_and, &&icmp_or };

    uint32_t res;

    goto *icomparison_table[i1];

    icmp_eq: { res = GET_INT(a) == GET_INT(b); goto next; }
    icmp_and: { res = GET_INT(a) & GET_INT(b); goto next; }
    icmp_or: { res = GET_INT(a) | GET_INT(b); goto next; }

    next: {
      INCREASE_IP_BY(pc, (uint32_t) res == 0 ? i2 : 1);
      goto *jmp_table[op];
    }
  }

  case_jump_else_rel_cmp_constant: {
    Value a = stack_pop(module->stack);
    Value b = constants[i3];

    ASSERT(get_type(a) == get_type(b), "Expected integers");
    
    Value cmp = compare_eq(a, b);
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
    Value b = constants[i3];

    ASSERT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers");

    void* icomparison_table[] = { 
      &&icmp_cst_lt, &&icmp_cst_gt, &&icmp_cst_eq, &&icmp_cst_neq, 
      &&icmp_cst_lte, &&icmp_cst_gte, &&icmp_cst_and, &&icmp_cst_or };

    uint32_t res;

    goto *icomparison_table[i2];

    icmp_cst_lt: { res = GET_INT(a) < GET_INT(b); goto next_cst; }
    icmp_cst_gt: { res = GET_INT(a) > GET_INT(b); goto next_cst; }
    icmp_cst_eq: { res = GET_INT(a) == GET_INT(b); goto next_cst; }
    icmp_cst_neq: { res = GET_INT(a) != GET_INT(b); goto next_cst; }
    icmp_cst_gte: { res = GET_INT(a) >= GET_INT(b); goto next_cst; }
    icmp_cst_lte: { res = GET_INT(a) <= GET_INT(b); goto next_cst; }
    icmp_cst_and: { res = GET_INT(a) & GET_INT(b); goto next_cst; }
    icmp_cst_or: { res = GET_INT(a) | GET_INT(b); goto next_cst; }

    next_cst: {
      INCREASE_IP_BY(pc, (uint32_t) res == 0 ? i1 : 1);
      goto *jmp_table[op];
    }
  }

  case_call_global: {
    Value callee = module->stack->values[i1];

    ASSERT(IS_FUN(callee) || IS_PTR(callee), "Invalid callee type");
  
    interpreter_table[(callee & MASK_SIGNATURE) == SIGNATURE_FUNCTION](module, &pc, callee, i2);

    goto *jmp_table[op];
  }

  case_call_local: {
    int32_t locals = module->base_pointer;

    Value callee = module->stack->values[locals + i1];

    ASSERT(IS_FUN(callee) || IS_PTR(callee), "Invalid callee type");
  
    interpreter_table[(callee & MASK_SIGNATURE) == SIGNATURE_FUNCTION](module, &pc, callee, i2);

    goto *jmp_table[op];
  }

  case_make_and_store_lambda: {
    int32_t new_pc = pc + 4;
    Value lambda = MAKE_FUNCTION(new_pc, i3);

    module->stack->values[i1] = lambda;

    INCREASE_IP_BY(pc, i2 + 1);
    goto *jmp_table[op];
  }

  case_mul: {
    Value a = stack_pop(module->stack);
    Value b = stack_pop(module->stack);

    ASSERT_FMT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers, got %s and %s", type_of(a), type_of(b));

    stack_push(module->stack, MAKE_INTEGER(a * b));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_mul_const: {
    Value a = stack_pop(module->stack);
    Value b = constants[i1];

    ASSERT_FMT(get_type(a) == TYPE_INTEGER && get_type(b) == TYPE_INTEGER, "Expected integers, got %s and %s", type_of(a), type_of(b));

    stack_push(module->stack, MAKE_INTEGER(a * b));
    INCREASE_IP(pc);
    goto *jmp_table[op];
  }

  case_return_unit: {
    Frame fr = pop_frame(module);
    module->stack->stack_pointer = fr.stack_pointer;
    module->base_pointer = fr.base_ptr;

    Value* values = malloc(sizeof(Value) * 3);
    values[0] = MAKE_SPECIAL();
    values[1] = MAKE_STRING("unit", 4);
    values[2] = MAKE_STRING("unit", 4);

    stack_push(module->stack, MAKE_LIST(values, 3));

    pc = fr.instruction_pointer;

    goto *jmp_table[op];
  }

  case_unknown: {
    THROW_FMT("Unknown opcode: %d", op);
    return;
  }
}

