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
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>

// 'getopt' for argument parsing
#include "getopt_pp_standalone.h"

using namespace std;
#include <SDL.h>
#include <SDL_thread.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <sstream>

#include "fluid.h"
#include "fviewer.h"
#include "genfunc.h"

using boost::lexical_cast;
namespace po = boost::program_options;

namespace {

enum { max_length = 1024 };

struct Options {
  int frameWidth;
  int frameHeight;
  string save_imgs;
  string load_data_file;
  string save_data_file;
  string show_info;
  double cam_to_fire_dist;
};

int options(int ac, char ** av, Options& opts) {
  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help", "Produce help message.")
      ("width,W", po::value<int>(&opts.frameWidth)->default_value(512),"frame width")
      ("height,H", po::value<int>(&opts.frameHeight)->default_value(512),"frame height")
      ("saveimgs,s", po::value<string>(&opts.save_imgs)->default_value(""),"save images , non-empty for 'yes' ")
      ("load,l", po::value<string>(&opts.load_data_file)->default_value(""),"load data file physics")
      ("write,w", po::value<string>(&opts.save_data_file)->default_value(""),"save data file physics")
      ("distance,d", po::value<double>(&opts.cam_to_fire_dist)->default_value(5.0),"range to fire origin")
      ("infodisplay,i", po::value<string>(&opts.show_info)->default_value(""),"show fps, #frames info at top");

  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
  }

  return 0;

}

Options opts;

} // end app-local namespace


int iDrawTexFlipSign = 1;

SDL_Surface* screen = NULL;
SDL_Thread* simthread = NULL;

Fluid* fluid = NULL;
FViewer* viewer = NULL;

char infostring[256];
int frames = 0, simframes = 0;
float fps = 0.0f, sfps = 0.0f;
GLuint texture;

float t = 0;
float* gfparams;

enum Mode { SIMULATE, PLAY };
Mode mode;
bool bSaveOutput = false;

bool paused = true, quitting = false, redraw = false, update = false, wasupdate = false;

int EventLoop(FILE* fp);


using std::cout;
using std::endl;
void GetBuffer(int w, int h, int idx, CvMat* result)
{
  int thesize = sizeof(char) * w * h * 3;
  char* pixels = (char*)malloc( thesize );
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

  // cout<<"Attempting to grab frame and save ..."<<strFilename<<endl;
  CvMat* img = GetBuffer( w, h,0);
  cvSaveImage( strFilename.c_str(), img );
  cvReleaseMat( &img );
  glFinish();
}

void error(const char* str)
{
  printf("Error: %s/n", str);

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
  SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL,10);

  screen = SDL_SetVideoMode(opts.frameWidth, opts.frameHeight, 32, SDL_OPENGL | !SDL_RESIZABLE);
  if ( screen == NULL ) {
    sprintf(str, "Unable to set video: %s\n", SDL_GetError());
    error(str);
    return false;
  }

  /* Fluid */

  fluid = new Fluid();
  fluid->diffusion = -0.0000001f;
  fluid->viscosity =  0.00000001f; // 0
  fluid->buoyancy  =  2.0f;   // 1.5
  fluid->cooling   =  1.0f;   // 1.0
  fluid->vc_eps    =  4.0f;   // 4.0


  /* Viewer */
  std::vector<std::string> strNames(0);
  viewer = new FViewer( strNames, opts.cam_to_fire_dist);

  viewer->viewport(screen->w, screen->h);

  gfparams = randfloats(256);

  return true;
}

int simulate(void* )
{
  assert(fluid != NULL);

  float dt = 0.25 * 0.1; // TODO: Param
  float f;

  while (!quitting)
  {
    if (!paused && !update)
    {
      cout << "updating ... t = " << t << endl;
      int idxStart    = (Fluid::N()+2)/8;
      int idxStop     = Fluid::N()+2-idxStart;
      double xyCenter = Fluid::N()/2.0;
      double booster  = (rand()%13==0)*31 + (rand()%5==0)*11 + (rand()%3==0)*2;
      for (int i=idxStart; i<idxStop; i++) // TODO: Param, relative to N !?
      {
        for (int j=idxStart; j<idxStop; j++) // TODO: Param, relative to N !?
        {
          f = genfunc(i-idxStart,j-idxStart,
                      idxStop-idxStart,idxStop-idxStart,
                      t,gfparams);
          double vs  = 3*(sin(t*CV_PI*1.3)) * sin( 2*CV_PI*(i-xyCenter)/idxStop );
          double us  = 3*(cos(t*CV_PI*1.2)) * cos( 2*CV_PI*(j-xyCenter)/idxStop );;
          double tval= (((float)(rand()%100)/50.0f + f *5.0f));
          fluid->sT[_I(i,idxStop+4,j)] = tval*0.0;
          fluid->sT[_I(i,idxStop+3,j)] = tval*0.5;
          fluid->sT[_I(i,idxStop+2,j)] = tval;

          fluid->sd[_I(i,idxStop+4,j)] = 1.0f;
          fluid->su[_I(i,idxStop+4,j)] = 3*us * tval;
          fluid->sw[_I(i,idxStop+4,j)] = 3*vs * tval;

          fluid->sv[_I(i,idxStop+4,j)] = -0*fluid->v[_I(i,idxStop/2,j)]; // -UP

          // walls
          fluid->su[_I(idxStop/2,i,j)]   = -5*us ;
          fluid->sw[_I(idxStop/2,i,j)]   = -5*vs ;

          fluid->su[_I(idxStop,i,j)]   = us * 5;
          fluid->su[_I(idxStart,i,j)]  = us * 5;
          fluid->su[_I(i,j+2,idxStart)]  = us * 5 ;
          fluid->su[_I(i,j+2,idxStop)]   = us * 5 ;
          fluid->sw[_I(idxStop,i,j)]   = vs * 5 ;
          fluid->sw[_I(idxStart,i,j)]  = vs * 5 ;
          fluid->sw[_I(i,j+2,idxStart)]  = vs * 5;
          fluid->sw[_I(i,j+2,idxStop)]   = vs * 5;

          fluid->sv[_I(i,idxStop,j)] = -abs(us)*abs(vs)-booster-2;




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
  if( ! opts.show_info.empty() )
    viewer->_dispstring = infostring;

  while (1) {
    //ts = SDL_GetTicks();
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_KEYUP:
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
          quitting = true;
          return 0;
          /*case SDLK_c:
       viewer->_draw_cube = !viewer->_draw_cube;
              break; */ // no! use this to trigger texture drawing
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
      stm << frames;
      std::string strName = stm.str();
      iDrawTexFlipSign *= -1;
      while( strName.size() < 8 )
        strName.insert( strName.begin(), '0' );
      string tmp = "fire-out/";
      tmp.append( strName );
      strName = tmp;
      std::string strName_ = "";
      strName.append(".png");
      strName_.append(strName);
      if( bSaveOutput ) {
        SaveBufferToImage( opts.frameWidth, opts.frameHeight, strName_);
      }
      SDL_GL_SwapBuffers();
      frames++;

    }
  }

  return 1;
}

struct FireDemoParams
{ // TODO: all demo params go in here
  FireDemoParams() : timer_msec(5.0) { }

  int init_from_args(int argc, char* argv[] ) {
    return 0;
  }

  double timer_msec;


};

using namespace GetOpt;
int main(int argc, char* argv[])
{
#ifdef WIN32
  error("Calibration data missing. Set your desk on fire, program will auto-calibrate at this time.\n");
  exit(1);
#endif

  FireDemoParams params;
  params.init_from_args(argc,argv);

  GetOpt_pp ops(argc, argv);

  if(options(argc,argv,opts)) {
    return 1;
  }

  bSaveOutput = ! (opts.save_imgs.empty() );

  char str[256];
  FILE *fp = NULL;
  int h[3];
  SDL_TimerID playtimer, fpstimer;
  // allocate a texture name
  glGenTextures( 1, &texture );


  if (!init())
    return 1;

  if (argc>1)
  {
    if (!strcmp(argv[1],"-l") ) {
      mode = PLAY;
      if (!viewer->open(argv[2]))	{
        printf("viewer->open failed \n");
        error("Unable to load data file\n");
        exit(1);
      }
    } else if (!strcmp(argv[1], "-w") ) {
      mode = SIMULATE;
      fp = fopen(argv[2], "wb");
      fwrite(h, sizeof(int), 3, fp);
    } else {
      printf("Invalid command line argument. Usage: fluid [-l|-w <filename>]\n");
      exit(1);
    }
  } else
  {
    cout << "No args as input, simulating without save!" << endl;
    mode = SIMULATE;
  }

  if (mode == SIMULATE)
    simthread = SDL_CreateThread(simulate, NULL);

  if (mode == PLAY)
    playtimer = SDL_AddTimer(params.timer_msec, timer_proc, NULL);

  fpstimer = SDL_AddTimer(1000, showfps, NULL);
  EventLoop(fp);
  SDL_RemoveTimer(fpstimer);
  if (mode==PLAY)
    SDL_RemoveTimer(playtimer);

  if (mode == SIMULATE) {
    quitting = true;
    SDL_WaitThread(simthread, NULL);
  }

  if (fp && (mode == SIMULATE))
  { // if ran with -w  foo.dat arg, save it!
    int pos;
    h[0] = h[1] = Fluid::N()+2;
    h[2] = simframes;
    pos = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fwrite(h, sizeof(int), 3, fp);
    fclose(fp);
    printf("%d frames written to file %s, %d kiB\n", simframes, argv[2], pos>>10);
  }
}
