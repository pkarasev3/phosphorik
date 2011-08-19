/* Author: Johannes Schmid, 2006, johnny@grob.org */
/* 2011 Updatess: Peter Karasev, karasevpa@gmail.com */

#ifndef __FLUID_H
#define __FLUID_H

#include <stdio.h>

/** Indexing lookup. e.g., used as:
    vorticity_x[ _I(i+1,j,k) ] to get right-of-i,j,k
    smaller number is log2(N()), larger is 2 times that */
#define _I(x,y,z) (((z)<<12)+((y)<<6)+x)

#define SWAPFPTR(x,y) {float *t=x;x=y;y=t;}

class Fluid
{
public:
  static int N() {
    return 62; // #define N 62				// must be N^2-2
  }

  struct SimParams;
  SimParams* simParms;

public:
  void init_with_args( int argc, char* argv[] );

  float buffers[10][64*64*64];
	float *d, *d0;			// density
	float *u, *u0;			// velocity in x direction
	float *v, *v0;			// velocity in y direction
	float *w, *w0;			// velocity in z direction
  float *T, *T0;			// temperature

  int Size() ;
protected:

	// simulation methods
		// beware: i changed stam's implementation from a destiation, source ordering
		// of the parameters to source, destiation
	void add_source(float* src, float *dst, float dt);
	void add_buoyancy(float dt);
	void set_bnd(int b, float* x);
	void diffuse(int b, float* x0, float* x, float diff, float dt);

  /** Smoke-like appearance: constant T while advecting */
  void advect(int b, float* x0, float* x, float* uu, float* vv, float* ww, float dt);

  /** Fire-like appearance: cool down while advecting */
  void advect_cool(int b, float* x0, float* x, float* y0, float* y, float* uu, float* vv, float* ww, float dt);

  void project(void);
	void vorticity_confinement(float dt);

	void vel_step(float dt);
	void dens_step(float dt);
	void dens_temp_step(float dt);

	// utility methods
	void clear_buffer(float* x);
	void clear_sources(void);

public:
  float sd[262144], su[262144], sv[262144], sw[262144], sT[262144];	// sources for density and velocities
  float diffusion, viscosity, buoyancy, cooling, vc_eps;

	Fluid(void);
	~Fluid(void);

	void step(float dt);

	void store(FILE *fp);
};

#endif

