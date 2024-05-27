#include <core/gc.h>

void gc_collect(GC* gc) {
  for (Object* root = gc->roots; root != NULL; root = root->next) {
    mark(gc, root);
  }
  sweep(gc);
}
