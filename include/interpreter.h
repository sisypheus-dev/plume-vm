#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <module.h>

Value call_function(Deserialized *mod, Value callee, int32_t argc, Value* argv);
Value run_interpreter(Deserialized *deserialized, int32_t ipc, bool does_return, int32_t current_callstack);

#endif  // INTERPRETER_H