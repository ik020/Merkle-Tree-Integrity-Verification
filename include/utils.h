//#pragma once
//#include <iostream>
//using namespace std;
//
//// ---------------- Template Implementation ------------------
//
//template <typename T>
//DynamicArray<T>::DynamicArray() {
//    capacity = 4;
//    length = 0;
//    data = new T[capacity];
//}
//
//template <typename T>
//DynamicArray<T>::~DynamicArray() {
//    delete[] data;
//}
//
//template <typename T>
//void DynamicArray<T>::resize() {
//    capacity *= 2;
//    T* newData = new T[capacity];
//
//    for (int i = 0; i < length; i++)
//        newData[i] = data[i];
//
//    delete[] data;
//    data = newData;
//}
//
//template <typename T>
//void DynamicArray<T>::push_back(const T& value) {
//    if (length == capacity)
//        resize();
//
//    data[length++] = value;
//}
//
//template <typename T>
//int DynamicArray<T>::size() const {
//    return length;
//}
//
//template <typename T>
//T& DynamicArray<T>::operator[](int index) {
//    // No bounds checking for simplicity
//    return data[index];
//}
