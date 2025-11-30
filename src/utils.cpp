#include "utils.h"
#include <iostream>
#include <string>
using namespace std;

#ifndef UTILS_H
#define UTILS_H

template <typename T>
class DynamicArray {
private:
    T* data;
    int capacity;
    int length;

    void resize();

public:
    DynamicArray();
    ~DynamicArray();

    void push_back(const T& value);
    int size() const;
    T& operator[](int index);
};

#endif
