/* Refactored by Peter Karasev, original Author: Johannes Schmid, 2006, johnny@grob.org */

// c-style headers
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// c++ stl
#include <iostream>
#include <string>
#include <list>

// opencv dev branch
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>

// 'getopt' for argument parsing
#include "getopt_pp_standalone.h"

#ifdef WIN32
#include <windows.h>
#endif

using namespace std;

#include <SDL.h>
#include <SDL_thread.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <sstream>

//#include "SDLfluid.h"
#include "fluid.h"
#include "viewer.h"
#include "genfunc.h"

SDL_Surface* screen = NULL;
SDL_Thread* simthread = NULL;

Fluid* fluid = NULL;
Viewer* viewer = NULL;

char infostring[256];
int frames = 0, simframes = 0;
float fps = 0.0f, sfps = 0.0f;
GLuint texture;

float t = 0;
float* gfparams;
std::string strName1;
std::string strName2;

enum Mode { SIMULATE, PLAY };
Mode mode;
int iDrawTexFlipSign = 1;
bool bSaveOutput = false;

bool paused = true, quitting = false, redraw = false, update = false, wasupdate = false;;

int EventLoop(FILE* fp);


using std::cout;
using std::endl;
void GetBuffer(int w, int h, int idx, CvMat* result)
{
        int thesize = sizeof(char) * w * h * 3;
        char* pixels = (char*)malloc( thesize );
        //int initWindowID = glutGetWindow();
        //if( initWindowID != idx )
        //        cout<<"warning, mismatch of window indices...\n";

        glReadPixels( 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels );
        int k = 0;
        for( int i = h-1; i >= 0; i-- ) {
                for( int j = 0; j < w; j++ ) {
                        uchar r = (uchar) pixels[k];
                        uchar g = (uchar) pixels[k+1];
                        uchar b = (uchar) pixels[k+2];
                        CvScalar scalar = cvScalar(b,g,r);
                        cvSet2D(result,i,j,scalar);
                        k+=3;
                }
        }
        free( pixels );
}

CvMat* GetBuffer(int w, int h, int num)
{
    CvMat* img = cvCreateMat(h,w,CV_8UC3);
    GetBuffer( w, h, num, img );
    return img;
}

void SaveBufferToImage( int w, int h, const std::string & strFilename ) {

    //cout<<"Attempting to grab frame and save ..."<<strFilename<<endl;
    CvMat* img = GetBuffer( w, h,0);
    cvSaveImage( strFilename.c_str(), img );
    cvReleaseMat( &img );
    glFinish();
}

void error(const char* str)
{
#ifdef WIN32
	MessageBox(NULL, str, "Error", MB_OK | MB_ICONSTOP);
#else
	printf("Error: %s/n", str);
#endif
}

bool init(void)
{
	char str[256];

	/* SDL */
	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0 ) {
        sprintf(str, "Unable to init SDL: %s\n", SDL_GetError());
		error(str);
        return false;
    }
    atexit(SDL_Quit);

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    screen = SDL_SetVideoMode(512, 512, 32, SDL_OPENGL | SDL_RESIZABLE);
	if ( screen == NULL ) {
        sprintf(str, "Unable to set video: %s\n", SDL_GetError());
		error(str);
        return false;
	}

	/* Fluid */

	fluid = new Fluid();
	fluid->diffusion = 0.00001f;
	fluid->viscosity = 0.000000f;
	fluid->buoyancy  = 4.0f;
	fluid->vc_eps    = 5.0f;

	/* Viewer */

        std::vector<std::string> strNames(2);
        strNames[0] = strName1;
        strNames[1] = strName2;

        viewer = new Viewer(strNames);
	viewer->viewport(screen->w, screen->h);

	gfparams = randfloats(256);

	return true;
}

int simulate(void* bla)
{
	assert(fluid != NULL);

	float dt = 0.1;
	float f;

	while (!quitting)
	{
		if (!paused && !update) {
	  	const int xpos = 60;
			const int ypos = 50;
			for (int i=0; i<8; i++)
			{
				float d = (rand()%1000)/1000.0f;
				for (int j=0; j<8; j++)
				{
					f = genfunc(i,j,8,8,t,gfparams);
					fluid->d[_I(xpos,i+ypos,j+28)] = d;
					//fluid->u[_I(xpos,i+ypos,j+28)] = -(1.0f + 2.0f*f);
					fluid->u[_I(xpos,i+ypos,j+28)] = -3.0f;
				}
			}

			fluid->step(dt);
			update = true;
			t += dt;
			simframes++;
		}
	}

	return 0;
}

SDL_UserEvent nextframe = { SDL_USEREVENT, 0, NULL, NULL};
Uint32 timer_proc(Uint32 interval, void* bla)
{
	/*if (!paused)
		SDL_PushEvent((SDL_Event*) &nextframe);*/
	wasupdate = update;
	update = true;
	return interval;
}

Uint32 showfps(Uint32 interval, void* bla)
{
	static int lastf = 0, lastsf = 0;

	sprintf(infostring, "FPS: %d, sFPS: %d, simframes: %d",
		frames - lastf,
		simframes - lastsf,
		simframes);

	lastf = frames;
	lastsf = simframes;

	return interval;
}

int EventLoop(FILE* fp)
{
    SDL_Event event;
	unsigned int ts;

	paused = false;

    while (1) {
		//ts = SDL_GetTicks();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYUP:
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE:
							quitting = true;
							return 0;
              /*case SDLK_c: // No: this flag is now used for yes/no drawing of background
							viewer->_draw_cube = !viewer->_draw_cube;
              break;*/
						case SDLK_i:
							if (viewer->_dispstring != NULL)
								viewer->_dispstring = NULL;
							else
								viewer->_dispstring = infostring;
							break;
						case SDLK_s:
							viewer->_draw_slice_outline = !viewer->_draw_slice_outline;
							break;
						case SDLK_SPACE:
							paused = !paused;
							break;
            default:
              cout << "key pressed: " << event.key.keysym.sym << endl;
              break;
					}

					break;

				case SDL_QUIT:
					quitting = true;
					return 0;

				case SDL_VIDEORESIZE:
					screen = SDL_SetVideoMode(event.resize.w, event.resize.h, 32, SDL_OPENGL | SDL_RESIZABLE);
					viewer->viewport(screen->w, screen->h);
					redraw = true;
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch (event.button.button)
					{
						case SDL_BUTTON_WHEELUP:
							viewer->anchor(0,0);
							viewer->dolly(-screen->h/10);
							redraw = true;
							break;
						case SDL_BUTTON_WHEELDOWN:
							viewer->anchor(0,0);
							viewer->dolly(screen->h/10);
							redraw = true;
							break;
						default:
							viewer->anchor(event.button.x, event.button.y);
							break;
					}
					break;
				case SDL_MOUSEMOTION:
					switch (event.motion.state) {
						case SDL_BUTTON(SDL_BUTTON_LEFT):
							viewer->rotate(event.motion.x, event.motion.y);
							redraw = true;
							break;
						case SDL_BUTTON(SDL_BUTTON_RIGHT):
							viewer->rotate_light(event.motion.x, event.motion.y);
							redraw = true;
							break;
					}
					break;
				/*case SDL_USEREVENT:
					viewer->load_frame();
					redraw = true;
					break;*/
			}
		}

    if (update)
		{
			if (mode == SIMULATE) {
				viewer->frame_from_sim(fluid);
				if (fp)
					fluid->store(fp);
			} else
				viewer->load_frame();
			redraw = true;
			update = wasupdate;
			wasupdate = false;
		}

		if (redraw)
    { /* Call main draw loop */
      viewer->draw();
      std::ostringstream stm;
      stm << "smoke-out/";
      stm << frames;
      std::string strName = stm.str();
      iDrawTexFlipSign *= -1;
      while( strName.size() < 8 )
          strName.insert( strName.begin(), '0' );
      string tmp = "smoke-out/";
      tmp.append( strName );
      strName = tmp;
      std::string strName_ = "";
      if( !bSaveOutput || -1 == iDrawTexFlipSign ) { // yes textures!
        strName.append(".png");
        viewer->_draw_cube = false;
      } else { // for no textures just the fluid stuff
        strName.append(".jpg");
        viewer->_draw_cube = true;
        redraw = false;
        }
      strName_.append(strName);
      if( bSaveOutput ) {
        SaveBufferToImage( 512, 512, strName_);
      }
			SDL_GL_SwapBuffers();
      if( 1 == iDrawTexFlipSign ) { frames++; }

      }
    }

	return 1;
}

using namespace GetOpt;
int main(int argc, char* argv[])
{
#ifdef WIN32
    error("Calibration data missing. Set your desk on fire, program will auto-calibrate at this time.\n");
    exit(1);
#endif

    GetOpt_pp ops(argc, argv);
    std::string str1 = "image005.bmp";
    std::string str2 = "image006.bmp";
    ops >> Option('a', "texture1", str1);
    ops >> Option('b', "texture2", str2);
    ops >> OptionPresent( 's', "saveImages", bSaveOutput );

    strName1 = str1;
    strName2 = str2;

    cout<<"save image data, is -s flag present? "<< bSaveOutput <<endl;
    cout<<"strName1:"<<strName1<<endl;
    cout<<"strName2: "<<strName2<<endl;

	char str[256];
	FILE *fp = NULL;
	int h[3];
	SDL_TimerID playtimer, fpstimer;
  // allocate a texture name
  glGenTextures( 1, &texture );


	if (!init())
		return 1;

	if (argc>1) {
                if (!strcmp(argv[1],"-l") /*&& (argc==3)*/ ) {
			mode = PLAY;
			if (!viewer->open(argv[2]))	{
                                printf("viewer->open failed \n");
				error("Unable to load data file\n");
				exit(1);
			}
		} else if (!strcmp(argv[1], "-w") && (argc==3)) {
			mode = SIMULATE;
			fp = fopen(argv[2], "wb");
			fwrite(h, sizeof(int), 3, fp);
		} else {
			printf("Invalid command line argument. Usage: fluid [-l|-w <filename>]\n");
			exit(1);
		}
	} else {
		mode = SIMULATE;
	}

	if (mode == SIMULATE)
		simthread = SDL_CreateThread(simulate, NULL);

	if (mode == PLAY)
		playtimer = SDL_AddTimer(1000/16, timer_proc, NULL);

	fpstimer = SDL_AddTimer(1000, showfps, NULL);
	EventLoop(fp);
	SDL_RemoveTimer(fpstimer);
	if (mode==PLAY)
		SDL_RemoveTimer(playtimer);

	if (mode == SIMULATE) {
		quitting = true;
		SDL_WaitThread(simthread, NULL);
	}

	if (fp && (mode == SIMULATE))	{
		int pos;
		h[0] = h[1] = N+2;
		h[2] = simframes;
		pos = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		fwrite(h, sizeof(int), 3, fp);
		fclose(fp);
		printf("%d frames written to file %s, %d kiB\n", simframes, argv[2], pos>>10);
	}
}
