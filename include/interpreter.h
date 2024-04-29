#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <module.h>

void run_interpreter(Deserialized deserialized);

extern Constants constants;
extern DLL* handles;

extern int32_t plm_argc;
extern Value* plm_argv;

#endif  // INTERPRETER_H