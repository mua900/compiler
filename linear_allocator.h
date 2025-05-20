#pragma once

#include "common.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uintptr_t* data;  // to match the word size
  size_t used;
  size_t size;
} Linear_Allocator;

Linear_Allocator make_allocator(size_t size);
void* allocate(Linear_Allocator* la, size_t size);
void deallocate(Linear_Allocator* la, size_t size);

#ifdef LA_IMPLEMENTATION

Linear_Allocator make_allocator(size_t size) {
  uintptr_t* data = (uintptr_t*)malloc(size);
  if (!data) {
    fprintf(stderr, "Malloc failed trying to create a linear allocator buffer with size %zu\n", size);
    exit(1);
  }

  return (Linear_Allocator) {data, 0, size};
}

// @xxx takes up a precious name in the global namespace
void* allocate(Linear_Allocator* la, size_t size) {
  size_t alloc_size = next_multiple_of_wordsize(size);
  if (la->size < la->used + alloc_size) {
    uintptr_t* tmp = (uintptr_t*)malloc(la->size * 1.5);  // 1.5 arbitrary
    memcpy(tmp, la->data, la->used);
    free(la->data);
    la->data = tmp;
  }

  void* mem = ((uintptr_t*)la->data) + la->used;
  la->used += size;

  return mem;
}

void deallocate(Linear_Allocator* la, size_t size) {
  size_t dealloc_size = MIN(size, la->used);

  la->used -= dealloc_size;
}

#endif

#undef MIN
#ifdef __cplusplus
}
#endif
