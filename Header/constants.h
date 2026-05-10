#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <iostream>
#include <fstream>
#include <ctime>

using namespace std;

inline int lenArr(const char* arr) {
    int size = 0;
    while (arr[size] != '\0') size++;
    return size;
}

#endif