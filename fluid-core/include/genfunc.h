/* Author: Johannes Schmid, 2006, johnny@grob.org */
#ifndef _GENFUNC_H
#define _GENFUNC_H

extern float* genparams;

float* randfloats(int n);

float genfunc(int x, int y, int sx, int sy, float t, float* p);

#endif
