#ifndef UTILS_H
#define UTILS_H

#include <cstdio>

char* get_file_content(const char* filename);
int   calc_nlines(char* buffer);
int   get_file_size(FILE* file);
bool  is_equal(double a, double b);
void  ClearBuffer ();

#endif
