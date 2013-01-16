// c-style headers
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// c++ stl
#include <iostream>
#include <string>
#include <list>
#include <sys/types.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
// opencv dev branch
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

using namespace std;
#include <sstream>

using boost::lexical_cast;
namespace po = boost::program_options;

namespace {

struct Options {
    Options() {
    }
    string inputDir;
};

/**  given a "dir" as string and ending extension, put name of files
     into the vector string. vector is sorted lexicographically.
  */
void lsFilesOfType(const char * dir, const string& extension,
        vector<string>& files) {
    files.clear();
    DIR *dp;
    struct dirent *dirp;
    if ((dp = opendir(dir)) == NULL) {
        return;
    }

    while ((dirp = readdir(dp)) != NULL) {
        std::string name(dirp->d_name);
        size_t pos = name.find(extension);
        if (pos != std::string::npos) {
            files.push_back(name);
        }
    }
    closedir(dp);
    std::sort(files.begin(), files.end());
}

/**  given an ending extension, put prefix name of files
     into the vector string. vector is sorted lexicographically.
     e.g. dir = "/1941" contains smolensk.jpg, sevastopol.jpg, guderian.jpg
     and extension = ".jpg" gives files as [guderian, sevastopol, smolensk] */
void getFilePrefixes(const char * dir, const string& extension,
        vector<string>& files) {

    files.clear();
    DIR *dp;
    struct dirent *dirp;
    if ((dp = opendir(dir)) == NULL) {
        return;
    }

    while ((dirp = readdir(dp)) != NULL) {
        std::string name(dirp->d_name);
        size_t pos = name.find(extension);
        if (pos != std::string::npos) {
            files.push_back(name.substr(0, pos));
        }
    }
    closedir(dp);
    std::sort(files.begin(), files.end());
}

int options(int ac, char ** av, Options& opts) {
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "Produce help message.")
            ("directory,d", po::value<string>(&opts.inputDir)->default_value("./"),
                                    "directory where N images are. Will make N-1 flow sets.");

    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);
    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }
    return 0;

}

}


Options opts;
using namespace cv;
int main( int argc, char* argv[] ) {

  options( argc, argv, opts );

  vector<string> imgs,prefixes;
  lsFilesOfType( opts.inputDir.c_str(), "png", imgs );
  getFilePrefixes(opts.inputDir.c_str(),"png",prefixes);
  cv::Mat img0,velx,vely;

  boost::filesystem::create_directory("./outTwo");

  for( int k=0; k < (int) imgs.size(); k++ ) {
    cv::Mat img1 = imread( imgs[k] );
    img1.convertTo(img1,CV_32FC3);
    cv::cvtColor(img1,img1,CV_RGB2GRAY);
    img1.convertTo(img1,CV_8UC1);
    if( k==0 ) {
      cout << "got img 0..." << endl;
    } else {
      CvTermCriteria criteria;
      criteria.epsilon=1e-3;
      criteria.max_iter=300;
      criteria.type=CV_TERMCRIT_ITER;
      double lambda=0.25;

      //IplImage* i0 = cvCreateImage( cvSize(img1.cols,img1.rows), IPL_DEPTH_32F, 1 );
      //IplImage* i1 = cvCreateImage( cvSize(img1.cols,img1.rows), IPL_DEPTH_32F, 1 );
      CvMat* i0 = cvCreateMat( img1.rows, img1.cols,CV_8UC1);
      CvMat* i1 = cvCreateMat( img1.rows, img1.cols,CV_8UC1);

      CvMat* vx = cvCreateMat( img1.rows, img1.cols,CV_32FC1);
      CvMat* vy = cvCreateMat( img1.rows, img1.cols,CV_32FC1);

      insertImageCOI( img0, i0,0);
      insertImageCOI( img1, i1,0);
      cvCalcOpticalFlowHS( i0, i1, 0, vx, vy, lambda, criteria);
      velx = cvarrToMat(vx,true);
      vely = cvarrToMat(vy,true);

      double vmin,vmax;
      cv::minMaxLoc(velx,&vmin,&vmax);
      cout << "sanity check, min and max vx values: " << vmin << ", " << vmax << endl;
      velx = velx * 500 + 8192;
      vely = vely * 500 + 8192;
      velx.clone().convertTo(velx,CV_16UC1);
      vely.clone().convertTo(vely,CV_16UC1);
      //cv::resize(velx.clone(),velx, Size(img1.cols*4,img1.rows*4) );
      //cv::resize(vely.clone(),vely, Size(img1.cols*4,img1.rows*4) );

      //std::vector<int> params(2);
//      params[0]=CV_IMWRITE_JPEG_QUALITY;
//      params[1]=100;
      assert( velx.depth() == CV_16U );
      // 5. create list for storing per-frame string data
      list<string> state_info_csv_lines;

      imwrite( "./outTwo/" + prefixes[k] + "_debug_CVHS_vX.png",velx);//, params);
      imwrite( "./outTwo/" + prefixes[k] + "_debug_CVHS_vY.png",vely);//, params);


      //cvReleaseImage(&i0); cvReleaseImage(&i1);
      cvReleaseMat(&i0); cvReleaseMat(&i1);
      cvReleaseMat(&vx); cvReleaseMat(&vy);

    }
    img1.copyTo(img0);
    //cv::imshow( imgs[k], img); cv::waitKey(0);

  }

  return 0;
}
