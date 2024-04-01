#ifndef LIBRARY_H
#define LIBRARY_H

#if defined(_WIN32)
#include <windows.h>
typedef HMODULE DLL;
#else
typedef void* DLL;
#endif

DLL load_library(const char* path);
void* get_proc_address(DLL library, const char* name);
void free_library(DLL library);

#endif