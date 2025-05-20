#pragma once

#include "common.hpp"

template <typename T>
struct ArrayView {
  T const * data = NULL;
  size_t count = 0;
  ArrayView(T const * const data, size_t count) : data(data), count(count) {}
  ArrayView() = default;

  const T* get_ref(size_t index) const {
    if (index >= count) {
      panic_and_abort("Index out of range (ArrayView)");
    }

    return &data[index];
  }

  T get(size_t index) const {
    if (index >= count) {
      panic_and_abort("Index out of range (ArrayView)");
    }

    return data[index];
  }

  const T* last() {
    if (!count) return NULL;
    return &data[count-1];
  }

  T const * begin() const {
    return data;
  }
  T const * end() const {
    return data + count;
  }
};

template<typename T>
struct DArray {
  T* data;
  size_t size;
  size_t capacity;

  DArray() : size(0), capacity(8) {
    data = new T[8];
  }

  T get(size_t index) const {
    if (index >= size) {
      panic_and_abort("Index out of range");
    }

    return data[index];
  }

  DArray(size_t init_cap) {
    data = new T[init_cap];
    size = 0;
    capacity = init_cap;
  }

  T operator[](size_t index) const {
    return data[index];
  }

  T* get_ref(size_t index) const {
    if (index >= size) {
      panic_and_abort("DArray, index out of range");
    }

    return &data[index];
  }

  void add(const T& element) {
    ensure_capacity(size);

    data[size] = element;
    size++;
  }

  void free() {
    delete[] data;
  }

  T* last() const {
    return data + size - 1;
  }

  void ensure_capacity(size_t p_capacity) {
    if (capacity <= p_capacity) {
      T* ndata = new T[capacity * 2];
      for (size_t i = 0; i < size; i++) {
        ndata[i] = data[i];
      }

      delete[] data;
      data = ndata;
      capacity *= 2;
    }
  }

  T pop() {
    if (size == 0) {
      panic_and_abort("Trying to pop an element from an empty array");
    }

    size--;
    return data[size];
  }

  T* begin() {
    return data;
  }

  T* end() {
    return data + size;
  }

  int find(T value, bool (*compare)(T a, T b)) const {
    for (int i = 0; i < size; i++) {
      if (compare(value, data[i])) {
        return i;
      }
    }

    return -1;
  }
};

template<typename T>
struct Array {
  T* data = NULL;
  int size = 0;

  T get(int index) { return data[index]; }

  Array() = default;
  Array(T* data, int size) : data(data), size(size) {}
  Array(DArray<T> darray) : data(darray.data), size(size) {}
};
