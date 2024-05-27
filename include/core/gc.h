/*
 * gc - A simple mark and sweep garbage collector for C.
 */

#ifndef __GC_H__
#define __GC_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct Object {
  bool marked;
  struct Object* next;
  struct Object** references;
  size_t size;
} Object;

typedef struct {
  Object* objects;
  Object* roots;
  size_t object_count;
  size_t max_objects;
} GC;

static GC gc = {NULL, NULL, 0, 2048};

static inline void mark(GC *gc, Object* obj);
static inline void sweep(GC* gc);

static inline void gc_collect(GC* gc) {
  for (Object* root = gc->roots; root != NULL; root = root->next) {
    mark(gc, root);
  }
  sweep(gc);
}

static inline Object* new_object(GC* gc, size_t size, int ref_count) {
  if (gc->object_count >= gc->max_objects) {
    gc_collect(gc);
  }

  Object* obj = (Object*)malloc(sizeof(Object));
  obj->marked = false;
  obj->next = gc->objects;
  obj->size = size;
  obj->references =
      ref_count > 0 ? (Object**)malloc(sizeof(Object*) * ref_count) : NULL;
  gc->objects = obj;
  gc->object_count++;
  return obj;
}

static inline void mark(GC* gc, Object* obj) {
  if (obj == NULL || obj->marked) return;
  obj->marked = true;
  for (size_t i = 0; i < obj->size / sizeof(Object*); i++) {
    mark(gc, obj->references[i]);
  }
}

static inline void sweep(GC* gc) {
  Object** obj = &gc->objects;
  while (*obj) {
    if (!(*obj)->marked) {
      Object* unreferenced = *obj;
      *obj = unreferenced->next;
      free(unreferenced->references);
      free(unreferenced);
      gc->object_count--;
    } else {
      (*obj)->marked = false;
      obj = &(*obj)->next;
    }
  }
}

static void* gc_malloc(GC* gc, size_t size) {
  Object* obj = new_object(gc, size, 0);
  return (void*)(obj + 1);
}

static inline void* gc_realloc(GC* gc, void* ptr, size_t size) {
  if (!ptr) return gc_malloc(gc, size);

  Object* obj = (Object*)ptr - 1;
  Object* new_obj = new_object(gc, size, 0);
  size_t copy_size = obj->size < size ? obj->size : size;
  memcpy(new_obj + 1, obj + 1, copy_size);
  return (void*)(new_obj + 1);
}

// Replace standard calloc with GC calloc
static inline void* gc_calloc(GC* gc, size_t num, size_t size) {
  size_t total_size = num * size;
  void* ptr = gc_malloc(gc, total_size);
  memset(ptr, 0, total_size);
  return ptr;
}

static inline char* gc_strdup (GC* gc, const char* s)
{
    size_t len = strlen(s) + 1;
    void *new = gc_malloc(gc, len);

    if (new == NULL) {
        return NULL;
    }
    return (char*) memcpy(new, s, len);
}


#endif /* !__GC_H__ */
