/* Author: Johannes Schmid, 2006, johnny@grob.org */
#include <stdlib.h>
#include <math.h>
#include <time.h>

float* genparams;

float* randfloats(int n)
{
	int i;
	float* v = (float*) malloc(n*sizeof(float));

	srand(time(NULL));

	for (i=0; i<n; i++)
	{
		v[i] = (rand()%1000)/1000.0f;
/*		if (i<10)
			printf("%.3f ", v[i]);*/
	}
//	printf("\n");
	return v;
}

#define PI2 3.141592654*2
float genfunc(int x, int y, int sx, int sy, float t, float* p)
{
	float f, fx, fy;
	int i, pi = 0;

	f=0;
	for (i=0; i<12; i++)
	{
		f += (1.0f +
				sin(((float)x)/sx*PI2*(p[pi++]+1.0f)+p[pi++]*PI2 + p[pi++]*t)*
				sin(((float)y)/sy*PI2*(p[pi++]+1.0f)+p[pi++]*PI2 + p[pi++]*t))
			* (1.0f + sin((p[pi++]+0.5f)*t + p[pi++]*PI2)) * 0.25f;
	}
	f *= 1.0f/i;

	fx = (x < sx*0.9f) ? 1.0f : 1.0f-(x-sx*0.9f)/(sx*0.2f);
	if (x < sx*0.1f)
		fx = 0.5f + x/(sx*0.2f);

	fy = (y < sy*0.9f) ? 1.0f : 1.0f-(y-sy*0.9f)/(sy*0.2f);
	if (y < sy*0.1f)
		fy = 0.5f + y/(sy*0.2f);


	return f * fx * fy;
}
