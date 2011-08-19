/* Author: Johannes Schmid and Ingemar Rask, 2006, johnny@grob.org */
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <algorithm>

//#include <GL/gl.h>
//#include <GL/glu.h>
//#include "glext.h"
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>

#include <X11/Intrinsic.h>    /* Display, Window */
#include <GL/glx.h>           /* GLXContext */
#include <iostream>

bool bNoTex = false;

#include "viewer.h"
extern "C" {
#include "trackball.h"
}
#include "fluid.h"
#include "imageloader.h"

// load a 256x256 RGB .RAW file as a texture
using std::cout;
using std::endl;
GLuint LoadTextureRAW( const char * filename, int width, int height, int wrap = 1)
{
    GLuint texture;
    //int width, height;
    void * data;
    FILE * file;

    // open texture data
    file = fopen( filename, "rb" );
    if ( file == NULL ) { printf("Failed to open texture file! \n "); return 0; }

    // allocate buffer
    //width = 256;
    //height = 256;
    size_t nBytes = sizeof(unsigned char) * width * height * 3;
    cout<<"malloc and fread number of bytes: "<<nBytes<<endl;
    data = malloc( nBytes );

    // read texture data
    fread( data, sizeof(unsigned char) * width * height * 3, 1, file );
    cout<<"read succeeded!"<<endl;
    fclose( file );
    cout<<"file close succeeded!"<<endl;

    // allocate a texture name
    glGenTextures( 1, &texture );

    // select our current texture
    glBindTexture( GL_TEXTURE_2D, texture );

    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                     GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    // if wrap is true, the texture wraps over at the edges (repeat)
    //       ... false, the texture ends at the edges (clamp)
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                     wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                     wrap ? GL_REPEAT : GL_CLAMP );

    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width, height,
                       GL_RGB, GL_UNSIGNED_BYTE, data );
    cout<<"build 2D mipmaps succeeded!"<<endl;

    // free buffer
    free( data );
    cout<<"texture: "<<texture<<endl;
    return texture;
}

GLuint LoadTextureBMP( Image* img, int wrap = 1)
{
    GLuint texture;
    void * data;

    int height = img->height;
    int width  = img->width;

    // allocate buffer
    size_t nBytes = sizeof(unsigned char) * width * height * 3;
    cout<<"malloc and fread number of bytes: "<<nBytes<<endl;
    data = malloc( nBytes );

    // read texture data
    memcpy( data, img->pixels, nBytes );
    cout<<"copy from img to void* data succeeded!"<<endl;

    // allocate a texture name
    glGenTextures( 1, &texture );

    // select our current texture
    glBindTexture( GL_TEXTURE_2D, texture );

    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                     GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    // if wrap is true, the texture wraps over at the edges (repeat)
    //       ... false, the texture ends at the edges (clamp)
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                     wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                     wrap ? GL_REPEAT : GL_CLAMP );

    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width, height,
                       GL_RGB, GL_UNSIGNED_BYTE, data );
    cout<<"build 2D mipmaps succeeded!"<<endl;

    // free buffer
    free( data );
    cout<<"texture: "<<texture<<endl;
    return texture;
}


bool canRunFragmentProgram(const char* programString);	// from FragmentUtils.cpp

	// cube vertices
GLfloat cv[][3] = {
	{1.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}, {-1.0f, -1.0f, 1.0f}, {1.0f, -1.0f, 1.0f},
	{1.0f, 1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f}
};

	// edges have the form edges[n][0][xyz] + t*edges[n][1][xyz]
float edges[12][2][3] = {
	{{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
	{{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
	{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
	{{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},

	{{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	{{-1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	{{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
	{{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},

	{{-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
	{{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
	{{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
	{{-1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}}
};

Viewer::Viewer( std::vector<std::string> & texNames )
{
	trackball(_quat, 0.0, 0.0, 0.0, 0.0);
	trackball(_lquat, 0.0, 0.0, 0.0, 0.0);
	_dist = 5.0f;
	_fp = NULL;
	_texture_data = NULL;

	_draw_cube = true;
	_draw_slice_outline = false;
	_dispstring = NULL;

	init_GL();
	init_font();

	/*
	_light_dir[0] = -1.0f;
	_light_dir[1] = 0.50f;
	_light_dir[2] = 0.0f;
	*/
	_light_dir[0] = 0.0f;
	_light_dir[1] = 1.0f;
	_light_dir[2] = -1.0f;
  gen_ray_templ(Fluid::N()+2);

        /*this->texture1 = LoadTextureRAW( "texture1.raw",256,256 );
        this->texture2 = LoadTextureRAW( "texture2.raw",128,128 ); */
        if (texNames[0].size() > 2) {
        this->image_textures.clear();
            for( size_t k = 0; k < texNames.size(); k++ ) { // assign the textures
                char* cstrName = (char*) malloc( sizeof(char) * texNames[k].size() + 1 );
                memcpy( cstrName, texNames[k].c_str(), sizeof(char) *(texNames[k].size()));
                cstrName[texNames[k].size()] = 0;
                std::cout<<"attempting to load file: "<<cstrName<< std::endl;
                Image* img = loadBMP( cstrName ); //new Image( cstrName, 256, 256 );
                std::cout<<"load BMP done. "<<endl;
                image_textures.push_back(img);
                free( cstrName );
                if( k == 0 )
                    texture1 = LoadTextureBMP( image_textures[k] );
                else if( k == 1 )
                    texture2 = LoadTextureBMP( image_textures[k] );
            }
        } else { bNoTex = true; }

        theta = 0.0;
}

Viewer::~Viewer()
{
	if (_texture_data)
		free(_texture_data);
	if (_fp)
		fclose(_fp);
}

void Viewer::init_GL(void)
{
	glEnable(GL_TEXTURE_3D);
	glDisable(GL_DEPTH_TEST);
	glCullFace(GL_FRONT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	glGenTextures(1, &_txt[0]);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_3D, _txt[0]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,GL_CLAMP);

}

bool Viewer::init_font(void)
{
	Display* display;
	XFontStruct* font_info;
	int first;
	int last;

	_font_base = glGenLists(256);

	/* Need an X Display before calling any Xlib routines. */
	display = XOpenDisplay(0);
	if (display == 0) {
		printf("Error: XOpenDisplay() failed\n");
		return false;
	}
	else {

		/* Load the font. */
		font_info = XLoadQueryFont(display, "fixed");
		if (!font_info) {
			printf("Error:  XLoadQueryFont() failed\n");
			return false;
		}
		else {
			/* Tell GLX which font & glyphs to use. */
			first = font_info->min_char_or_byte2;
			last  = font_info->max_char_or_byte2;
			glXUseXFont(font_info->fid, first, last-first+1, _font_base+first);
		}
		XCloseDisplay(display);
		display = 0;
	}

	return true;
}

void Viewer::print_string(char* s)
{
	if (s && strlen(s)) {
		glPushAttrib(GL_LIST_BIT);
		glListBase(_font_base);
		glCallLists(strlen(s), GL_UNSIGNED_BYTE, (GLubyte *)s);
		glPopAttrib();
	}
}


void Viewer::draw(void)
{
	int i;

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, -_dist,  0, 0, 0,  0, 1, 0);

	float m[4][4];
	build_rotmatrix(m, _quat);
	glMultMatrixf(&m[0][0]);

  if (_draw_cube)
		draw_cube();
	draw_slices(m, _draw_slice_outline);

	if (_dispstring != NULL) {
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixd(_ortho_m);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glDisable(GL_TEXTURE_3D);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glRasterPos2i(-_sx/2 + 10, _sy/2 - 15);

		print_string(_dispstring);

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixd(_persp_m);
		glMatrixMode(GL_MODELVIEW);
	}
}


void Viewer::draw_cube(void)
{
    if( bNoTex )
        return;

	glDisable(GL_TEXTURE_3D);
	glDisable(GL_FRAGMENT_PROGRAM_ARB);

        glEnable(GL_TEXTURE_2D);
        glPushMatrix();
        glTranslatef( -1.5, 0.0, 1.0 );
        glRotatef( theta, 0.0f, 0.0f, 1.0f );
        glColor3f( 1.0, 1.0, 1.0 );
        glBindTexture( GL_TEXTURE_2D, texture1 );
        glBegin( GL_QUADS );
        glTexCoord2d(0.0,0.0); glVertex3f(-1.0,-1.0,0.0);//size of background image
        glTexCoord2d(1.0,0.0); glVertex3f(+1.0,-1.0,0.0);
        glTexCoord2d(1.0,1.0); glVertex3f(+1.0,+1.0,0.0);
        glTexCoord2d(0.0,1.0); glVertex3f(-1.0,+1.0,0.0);
        glEnd();
        glPopMatrix();

        glPushMatrix();
        glTranslatef( 0.0, -1.0, 0.0 );
        glRotatef( theta, 0.0f, 1.0f, 0.0f );
        glColor3f( 1.0, 1.0, 1.0 );//, 0.5 );
        glBindTexture( GL_TEXTURE_2D, texture2 );
        glBegin( GL_QUADS );
        glTexCoord2d(0.0,0.0); glVertex3f(-1.5,0.0,-1.5);
        glTexCoord2d(1.0,0.0); glVertex3f(+1.5,0.0,-1.5);
        glTexCoord2d(1.0,1.0); glVertex3f(+1.5,0.0,+1.5);
        glTexCoord2d(0.0,1.0); glVertex3f(-1.5,0.0,+1.5);
        glEnd();
        glPopMatrix();
        glDisable(GL_TEXTURE_2D);

        theta += 1.0f; //increment the rotation

}

class Convexcomp
{
private:
	const Vec3 &p0, &up;
public:
	Convexcomp(const Vec3& p0, const Vec3& up) : p0(p0), up(up) {}

	bool operator()(const Vec3& a, const Vec3& b) const
	{
		Vec3 va = a-p0, vb = b-p0;
		return dot(up, cross(va, vb)) >= 0;
	}
};

void Viewer::draw_slices(float m[][4], bool frame)
{
	int i;

	Vec3 viewdir(m[0][2], m[1][2], m[2][2]);
	viewdir.Normalize();
		// find cube vertex that is closest to the viewer
	for (i=0; i<8; i++) {
		float x = cv[i][0] + viewdir[0];
		float y = cv[i][1] + viewdir[1];
		float z = cv[i][2] + viewdir[2];
		if ((x>=-1.0f)&&(x<=1.0f)
			&&(y>=-1.0f)&&(y<=1.0f)
			&&(z>=-1.0f)&&(z<=1.0f))
		{
			break;
		}
	}
	assert(i != 8);

		// our slices are defined by the plane equation a*x + b*y +c*z + d = 0
		// (a,b,c), the plane normal, are given by viewdir
		// d is the parameter along the view direction. the first d is given by
		// inserting previously found vertex into the plane equation
	float d0 = -(viewdir[0]*cv[i][0] + viewdir[1]*cv[i][1] + viewdir[2]*cv[i][2]);
	float dd = 2*d0/64.0f;
	int n = 0;
	for (float d = -d0; d < d0; d += dd) {
					// intersect_edges returns the intersection points of all cube edges with
			// the given plane that lie within the cube
		std::vector<Vec3> pt = intersect_edges(viewdir[0], viewdir[1], viewdir[2], d);

		if (pt.size() > 2) {
				// sort points to get a convex polygon
			std::sort(pt.begin()+1, pt.end(), Convexcomp(pt[0], viewdir));

			glEnable(GL_TEXTURE_3D);
			glBegin(GL_POLYGON);
			for (i=0; i<pt.size(); i++){
			  glColor3f(1.0, 1.0, 1.0);
			  glTexCoord3d((pt[i][0]+1.0)/2.0, (-pt[i][1]+1)/2.0, (pt[i][2]+1.0)/2.0);
			  glVertex3f(pt[i][0], pt[i][1], pt[i][2]);
			}
			glEnd();

			if (frame)
			{
				glDisable(GL_TEXTURE_3D);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glBegin(GL_POLYGON);
				for (i=0; i<pt.size(); i++) {
					glColor3f(0.0, 0.0, 1.0);
					glVertex3f(pt[i][0], pt[i][1], pt[i][2]);
				}
				glEnd();
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
		n++;
	}

//	printf("Sliced: %d\n", n);
}

std::vector<Vec3> Viewer::intersect_edges(float a, float b, float c, float d)
{
	int i;
	float t;
	Vec3 p;
	std::vector<Vec3> res;

	for (i=0; i<12; i++) {
		t = -(a*edges[i][0][0] + b*edges[i][0][1] + c*edges[i][0][2] + d)
			/ (a*edges[i][1][0] + b*edges[i][1][1] + c*edges[i][1][2]);
		if ((t>0)&&(t<2)) {
			p[0] = edges[i][0][0] + edges[i][1][0]*t;
			p[1] = edges[i][0][1] + edges[i][1][1]*t;
			p[2] = edges[i][0][2] + edges[i][1][2]*t;
			res.push_back(p);
		}
	}

	return res;
}

void Viewer::viewport(int w, int h)
{
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	GLdouble size = (GLdouble)((w >= h) ? w : h) / 2.0, aspect;
	if (w <= h) {
		aspect = (GLdouble)h/(GLdouble)w;
		glOrtho(-size, size, -size*aspect, size*aspect, -100000.0, 100000.0);
	}
	else {
		aspect = (GLdouble)w/(GLdouble)h;
		glOrtho(-size*aspect, size*aspect, -size, size, -100000.0, 100000.0);
	}
	glScaled(aspect, aspect, 1.0);
	glGetDoublev(GL_PROJECTION_MATRIX, _ortho_m);

	glLoadIdentity();
	gluPerspective(45.0 /* fov */,
		(GLdouble) w/h /* aspect ratio */,
		0.0 /* zNear */,
		10.0 /* zFar */
	);
	glGetDoublev(GL_PROJECTION_MATRIX, _persp_m);

	_sx = w;
	_sy = h;
}

void Viewer::anchor(int x, int y)
{
	_ax = x;
	_ay = y;
}

void Viewer::rotate(int x, int y)
{
    float spin_quat[4];

        float sx2 = _sx*1.0f, sy2 = _sy*0.5f;

	trackball(spin_quat,
		(_ax - sx2) / sx2,
		(_ay - sy2) / sy2,
		(x - sx2) / sx2,
		(y - sy2) / sy2);
	add_quats(spin_quat, _quat, _quat);
	_ax = x;
	_ay = y;
}

void Viewer::rotate_light(int x, int y)
{
    float spin_quat[4];

	float sx2 = _sx*0.5f, sy2 = _sy*0.5f;

	trackball(spin_quat,
		(_ax - sx2) / sx2,
		(_ay - sy2) / sy2,
		(x - sx2) / sx2,
		(y - sy2) / sy2);
	add_quats(spin_quat, _lquat, _lquat);
	_ax = x;
	_ay = y;

	float m[4][4];
	build_rotmatrix(m, _lquat);
	_light_dir[0] = m[0][2];
	_light_dir[1] = m[1][2];
	_light_dir[2] = m[2][2];
	gen_ray_templ(66);
	//printf("%.2f %.2f %.2f\n", _light_dir[0], _light_dir[1], _light_dir[2]);

}

void Viewer::dolly(int y)
{
	_dist += ((float)y - _ay)/((float)_sy) * _dist;
	_ay = y;
}

bool Viewer::open(char* filename)
{
	if (_fp)
		fclose(_fp);

	_fp = fopen(filename, "rb");
        if (!_fp) {
            printf("_fp is null, returning false in Viewer::open \n");
            return false;
        }

	fread(&_N, sizeof(int), 1, _fp);
	fread(&_N, sizeof(int), 1, _fp);
	printf("Resolution: %dx%dx%d\n", _N, _N, _N);
	if (_texture_data)
		free(_texture_data);
	_texture_data = (unsigned char*) malloc(_N*_N*_N*4);

	fread(&_nframes, sizeof(int), 1, _fp);
	printf("Number of frames: %d\n", _nframes);
	_cur_frame = 0;
        std::cout<<"value of fp: "<<_fp<<std::endl;
        return true;
}

void Viewer::load_frame(void)
{
	float* d = (float*) malloc(_N*_N*_N*sizeof(float));
	unsigned char* l = (unsigned char*) malloc(_N*_N*_N);

	if (++_cur_frame == _nframes) {
		fseek(_fp, 12, SEEK_SET);
		_cur_frame = 0;
	}

	fread(d, sizeof(float), _N*_N*_N, _fp);

	memset(l,0,_N*_N*_N);
	cast_light(_N, d, l);

#define ALPHA_THRESH 0.2f
	for (int i=0; i<_N*_N*_N; i++) {
		unsigned char c = l[i];
		_texture_data[(i<<2)] = c;
		_texture_data[(i<<2)+1] = c;
		_texture_data[(i<<2)+2] = c;
		_texture_data[(i<<2)+3] = (d[i]>ALPHA_THRESH) ? 255 : (unsigned char) (d[i]*255.0f*1.0f/ALPHA_THRESH);
	}
#undef ALPHA_THRESH

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, _N, _N, _N, 0, GL_RGBA, GL_UNSIGNED_BYTE, _texture_data);

	free(d);
	free(l);
}

void Viewer::frame_from_sim(Fluid* fluid)
{
	if (_texture_data == NULL)
    _texture_data = (unsigned char*) malloc(fluid->Size()*4);

  unsigned char* l = (unsigned char*) malloc(fluid->Size());
  int npts = Fluid::N()+2;
  memset(l,0,fluid->Size());
  cast_light(npts, fluid->d, l);

  int num_grid_pts = fluid->Size();
  for (int i=0; i<num_grid_pts; i++) {
		unsigned char c = l[i];
		_texture_data[(i<<2)] = c;
		_texture_data[(i<<2)+1] = c;
		_texture_data[(i<<2)+2] = c;
		_texture_data[(i<<2)+3] = (fluid->d[i]>0.1f) ? 255 : (unsigned char) (fluid->d[i]*2550.0f);
	}

	free(l);

	glActiveTextureARB(GL_TEXTURE0_ARB);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, npts, npts, npts, 0, GL_RGBA, GL_UNSIGNED_BYTE, _texture_data);
}

#define ALMOST_EQUAL(a, b) ((fabs(a-b)<0.00001f)?true:false)
void Viewer::gen_ray_templ(int edgelen)
{
	_ray_templ[0][0] = _ray_templ[0][2] = _ray_templ[0][2] = 0;
	float fx = 0.0f, fy = 0.0f, fz = 0.0f;
	int x = 0, y = 0, z = 0;
	float lx = _light_dir[0] + 0.000001f, ly = _light_dir[1] + 0.000001f, lz = _light_dir[2] + 0.000001f;
	int xinc = (lx > 0) ? 1 : -1;
	int yinc = (ly > 0) ? 1 : -1;
	int zinc = (lz > 0) ? 1 : -1;
	float tx, ty, tz;
	int i = 1;
	int len = 0;
	int maxlen = 3*edgelen*edgelen;
	while (len <= maxlen)
	{
		// fx + t*lx = (x+1)   ->   t = (x+1-fx)/lx
		tx = (x+xinc-fx)/lx;
		ty = (y+yinc-fy)/ly;
		tz = (z+zinc-fz)/lz;

		if ((tx<=ty)&&(tx<=tz)) {
			_ray_templ[i][0] = _ray_templ[i-1][0] + xinc;
			x =+ xinc;
			fx = x;

			if (ALMOST_EQUAL(ty,tx)) {
				_ray_templ[i][1] = _ray_templ[i-1][1] + yinc;
				y += yinc;
				fy = y;
			} else {
				_ray_templ[i][1] = _ray_templ[i-1][1];
				fy += tx*ly;
			}

			if (ALMOST_EQUAL(tz,tx)) {
				_ray_templ[i][2] = _ray_templ[i-1][2] + zinc;
				z += zinc;
				fz = z;
			} else {
				_ray_templ[i][2] = _ray_templ[i-1][2];
				fz += tx*lz;
			}
		} else if ((ty<tx)&&(ty<=tz)) {
			_ray_templ[i][0] = _ray_templ[i-1][0];
			fx += ty*lx;

			_ray_templ[i][1] = _ray_templ[i-1][1] + yinc;
			y += yinc;
			fy = y;

			if (ALMOST_EQUAL(tz,ty)) {
				_ray_templ[i][2] = _ray_templ[i-1][2] + zinc;
				z += zinc;
				fz = z;
			} else {
				_ray_templ[i][2] = _ray_templ[i-1][2];
				fz += ty*lz;
			}
		} else {
			assert((tz<tx)&&(tz<ty));
			_ray_templ[i][0] = _ray_templ[i-1][0];
			fx += tz*lx;
			_ray_templ[i][1] = _ray_templ[i-1][1];
			fy += tz*ly;
			_ray_templ[i][2] = _ray_templ[i-1][2] + zinc;
			z += zinc;
			fz = z;
		}

		len = _ray_templ[i][0]*_ray_templ[i][0]
			+ _ray_templ[i][1]*_ray_templ[i][1]
			+ _ray_templ[i][2]*_ray_templ[i][2];
		i++;
	}
}

#define DECAY 0.06f
void Viewer::cast_light(int n /*edgelen*/, float* dens, unsigned char* intensity)
{
	int i,j;
	int sx = (_light_dir[0]>0) ? 0 : n-1;
	int sy = (_light_dir[1]>0) ? 0 : n-1;
	int sz = (_light_dir[2]>0) ? 0 : n-1;

	float decay = 1.0f/(n*DECAY);

	for (i=0; i<n; i++)
		for (j=0; j<n; j++) {
			if (!ALMOST_EQUAL(_light_dir[0], 0))
				light_ray(sx,i,j,n,decay,dens,intensity);
			if (!ALMOST_EQUAL(_light_dir[1], 0))
				light_ray(i,sy,j,n,decay,dens,intensity);
			if (!ALMOST_EQUAL(_light_dir[2], 0))
				light_ray(i,j,sz,n,decay,dens,intensity);
		}
}


#define AMBIENT 3
inline void Viewer::light_ray(int x, int y, int z, int n, float decay, float* dens, unsigned char* intensity)
{
	int xx = x, yy = y, zz = z, i = 0;
	int offset;

	int l = 200;
	float d;

	do {
		offset = ((zz*n) + yy)*n + xx;
		if (intensity[offset] > 0)
			intensity[offset] = (unsigned char) ((intensity[offset] + l)*0.5f);
		else
			intensity[offset] = (unsigned char) l;
		d = dens[offset]*255.0f;
		if (l > AMBIENT) {
			l -= d*decay;
			if (l < AMBIENT)
				l = AMBIENT;
		}

		i++;
		xx = x + _ray_templ[i][0];
		yy = y + _ray_templ[i][1];
		zz = z + _ray_templ[i][2];
	} while ((xx>=0)&&(xx<n)&&(yy>=0)&&(yy<n)&&(zz>=0)&&(zz<n));
}

