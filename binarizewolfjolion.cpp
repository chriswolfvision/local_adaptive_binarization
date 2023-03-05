/**************************************************************
 * Binarization with several methods
 * (0) Niblacks method
 * (1) Sauvola & Co.
 *     ICDAR 1997, pp 147-152
 * (2) by myself - Christian Wolf
 *     Research notebook 19.4.2001, page 129
 * (3) by myself - Christian Wolf
 *     20.4.2007
 *
 * See also:
 * Research notebook 24.4.2001, page 132 (Calculation of s)
 **************************************************************/

#include <cmath>
#include <getopt.h>
#include <iostream>
#include <opencv2/opencv.hpp>

#define BINARIZEWOLF_VERSION "2.4 (August 1st, 2014)"
#define DEFAULT_K 0.5
#define MAX_WINX 100
#define DEFAULT_WINX 40
#define DEFAULT_WINY 40

enum ThreshMethod {
  NIBLACK = 0,
  SAUVOLA,
  WOLFJOLION,
};

/**********************************************************
 * Usage
 **********************************************************/

static void usage(char *com) {
    std::cerr << "Usage: " << com
              << " [ -x <winx> -y <winy> -k <parameter> -m <method> ] <inputimage> <outputimage>\n"
              << "\n"
              << "method: n           Niblack (1986)         needs white text on black background\n"
              << "        s           Sauvola et al. (1997)  needs black text on white background\n"
              << "        w (deafult) Wolf et al. (2001)     needs black text on white background\n"
              << "\n"
              << "Example:\n"
              << "       " << com << " -m w in.pgm out.pgm\n"
              << "       " << com << " in.pgm out.pgm\n"
              << "       " << com << " -m s -x 50 -y 50 -k 0.6 in.pgm out.pgm\n";
}

// *************************************************************
// glide a window across the image and
// create two maps: mean and standard deviation.
// *************************************************************
//
// Version patched by Thibault Yohan (using opencv integral images)

double calcLocalStats(cv::Mat &im, cv::Mat &map_m, cv::Mat &map_s, int winx, int winy) {
  cv::Mat im_sum;
  cv::Mat im_sum_sq;
  cv::integral(im, im_sum, im_sum_sq, CV_64F);

  int wxh = winx / 2;
  int wyh = winy / 2;
  int x_firstth = wxh;
  int y_firstth = wyh;
  int y_lastth = im.rows - wyh - 1;
  double winarea = winx * winy;

  double max_s = 0;
  for (int j = y_firstth; j <= y_lastth; j++) {
    // for sum array iterator pointer
    double *sum_top_left     = im_sum.ptr<double>(j - wyh);
    double *sum_top_right    = sum_top_left + winx;
    double *sum_bottom_left  = im_sum.ptr<double>(j - wyh + winy);
    double *sum_bottom_right = sum_bottom_left + winx;

    // for sum_sq array iterator pointer
    double *sum_eq_top_left     = im_sum_sq.ptr<double>(j - wyh);
    double *sum_eq_top_right    = sum_eq_top_left + winx;
    double *sum_eq_bottom_left  = im_sum_sq.ptr<double>(j - wyh + winy);
    double *sum_eq_bottom_right = sum_eq_bottom_left + winx;

    double sum    = (*sum_bottom_right    + *sum_top_left) -
                    (*sum_top_right       + *sum_bottom_left);
    double sum_sq = (*sum_eq_bottom_right + *sum_eq_top_left) -
                    (*sum_eq_top_right    + *sum_eq_bottom_left);

    double m = sum / winarea;
    double s = std::sqrt((sum_sq - m * sum) / winarea);
    if (s > max_s) {
      max_s = s;
    }

    float *map_m_data = map_m.ptr<float>(j) + x_firstth;
    float *map_s_data = map_s.ptr<float>(j) + x_firstth;
    *map_m_data++ = m;
    *map_s_data++ = s;

    // Shift the window, add and remove	new/old values to the histogram
    for (int i = 1; i <= im.cols - winx; i++) {
      sum_top_left++;
      sum_top_right++;
      sum_bottom_left++;
      sum_bottom_right++;

      sum_eq_top_left++;
      sum_eq_top_right++;
      sum_eq_bottom_left++;
      sum_eq_bottom_right++;

      sum    = (*sum_bottom_right    + *sum_top_left) -
               (*sum_top_right       + *sum_bottom_left);
      sum_sq = (*sum_eq_bottom_right + *sum_eq_top_left) -
               (*sum_eq_top_right    + *sum_eq_bottom_left);

      m = sum / winarea;
      s = std::sqrt((sum_sq - m * sum) / winarea);
      if (s > max_s) {
        max_s = s;
      }

      *map_m_data++ = m;
      *map_s_data++ = s;
    }
  }

  return max_s;
}

/**********************************************************
 * The binarization routine
 **********************************************************/

void NiblackSauvolaWolfJolion(cv::Mat im, cv::Mat output, ThreshMethod method,
                              int winx, int winy, double k, double dR = 128) {

  int wxh = winx / 2;
  int wyh = winy / 2;
  int x_firstth = wxh;
  int y_firstth = wyh;
  int x_lastth = im.cols - wxh - 1;
  int y_lastth = im.rows - wyh - 1;

  // Create local statistics and store them in a double matrices
  cv::Mat map_m = cv::Mat::zeros(im.rows, im.cols, CV_32F);
  cv::Mat map_s = cv::Mat::zeros(im.rows, im.cols, CV_32F);
  double max_s = calcLocalStats(im, map_m, map_s, winx, winy);

  double min_I;
  double max_I;
  cv::minMaxLoc(im, &min_I, &max_I);

  cv::Mat thsurf(im.rows, im.cols, CV_32F);

  double th = 0.0;

  // Create the threshold surface, including border processing
  // ----------------------------------------------------
  for (int j = y_firstth; j <= y_lastth; j++) {

    float *th_surf_ptr = thsurf.ptr<float>(j) + wxh;
    float *map_m_ptr    = map_m.ptr<float>(j) + wxh;
    float *map_s_ptr    = map_s.ptr<float>(j) + wxh;

    // NORMAL, NON-BORDER AREA IN THE MIDDLE OF THE WINDOW:
    for (int i = 0; i <= im.cols - winx; i++) {
      double m = *map_m_ptr++;
      double s = *map_s_ptr++;

      // Calculate the threshold
      switch (method) {

      case NIBLACK:
        th = m + k * s;
        break;

      case SAUVOLA:
        th = m * (1 + k * (s / dR - 1));
        break;

      case WOLFJOLION:
        th = m + k * (s / max_s - 1) * (m - min_I);
        break;

      default:
        std::cerr << "Unknown thresholding method: " << method << std::endl;
        std::exit(EXIT_FAILURE);
      }

      *th_surf_ptr++ = th;

      if (i == 0) {
        // LEFT BORDER
        float *th_surf_ptr = thsurf.ptr<float>(j);
        for (int i = 0; i <= x_firstth; ++i)
          *th_surf_ptr++ = th;

        // LEFT-UPPER CORNER
        if (j == y_firstth) {
          for (int u = 0; u < y_firstth; ++u) {
            float *th_surf_ptr = thsurf.ptr<float>(u);
            for (int i = 0; i <= x_firstth; ++i)
              *th_surf_ptr++ = th;
          }
        }

        // LEFT-LOWER CORNER
        if (j == y_lastth) {
          for (int u = y_lastth + 1; u < im.rows; ++u) {
            float *th_surf_ptr = thsurf.ptr<float>(u);
            for (int i = 0; i <= x_firstth; ++i)
              *th_surf_ptr++ = th;
          }
        }
      }

      // LEFT-UPPER BORDER
      if (j == y_firstth)
        for (int u = 0; u < y_firstth; ++u)
          thsurf.at<float>(u, i+wxh) = th;

      // LEFT-LOWER BORDER
      if (j == y_lastth)
        for (int u = y_lastth + 1; u < im.rows; ++u)
          thsurf.at<float>(u, i+wxh) = th;
    }

    // RIGHT BORDER
    th_surf_ptr = thsurf.ptr<float>(j) + x_lastth;
    for (int i = x_lastth; i < im.cols; ++i)
      *th_surf_ptr++ = th;

    // RIGHT-UPPER CORNER
    if (j == y_firstth) {
      for (int u = 0; u < y_firstth; ++u) {
        th_surf_ptr = thsurf.ptr<float>(u) + x_lastth;
        for (int i = x_lastth; i < im.cols; ++i)
          *th_surf_ptr++ = th;
      }
    }

    // RIGHT-LOWER CORNER
    if (j == y_lastth) {
      for (int u = y_lastth + 1; u < im.rows; ++u) {
        th_surf_ptr = thsurf.ptr<float>(u) + x_lastth;
        for (int i = x_lastth; i < im.cols; ++i)
          *th_surf_ptr++ = th;
      }
    }
  }
  std::cerr << "surface created\n";

  for (int y = 0; y < im.rows; ++y) {
    unsigned char *im_data = im.ptr<unsigned char>(y);
    float *th_surf_data = thsurf.ptr<float>(y);
    unsigned char *output_data = output.ptr<unsigned char>(y);
    for (int x = 0; x < im.cols; ++x) {
      *output_data = *im_data >= *th_surf_data ? 255 : 0;
      im_data++;
      th_surf_data++;
      output_data++;
    }
  }
}

ThreshMethod parseMethodString(char *methodString) {
  char method = methodString[0];
  switch (method) {
    case 'n':
      std::cerr << "Niblack (1986)\n";
      return NIBLACK;
    case 's':
      std::cerr << "Sauvola et al. (1997)\n";
      return SAUVOLA;
    case 'w':
      std::cerr << "Wolf and Jolion (2001)\n";
      return WOLFJOLION;
    default:
      std::cerr << "\nInvalid method: '" << method << "'!";
      std::exit(EXIT_FAILURE);
  }
}

/**********************************************************
 * The main function
 **********************************************************/

int main(int argc, char **argv) {
  int winx = 0;
  int winy = 0;
  float optK = DEFAULT_K;
  ThreshMethod methodCode = WOLFJOLION;
  char *inputname;
  char *outputname;

  std::cerr << "===========================================================\n"
            << "Christian Wolf, LIRIS Laboratory, Lyon, France.\n"
            << "christian.wolf@liris.cnrs.fr\n"
            << "Version " << BINARIZEWOLF_VERSION << std::endl
            << "===========================================================\n";

  // Argument processing
  char c;
  while ((c = getopt(argc, argv, "x:y:k:m:")) != EOF) {
    switch (c) {
      case 'x':
        winx = std::atoi(optarg);
        break;
      case 'y':
        winy = std::atoi(optarg);
        break;
      case 'k':
        optK = std::atof(optarg);
        break;
      case 'm':
        methodCode = parseMethodString(optarg);
        break;
      case '?':
        usage(*argv);
        std::cerr << "\nProblem parsing the options!\n\n";
        std::exit(EXIT_FAILURE);
    }
  }

  switch (argc - optind) {
    case 2:
      inputname = argv[optind];
      outputname = argv[optind + 1];
      break;
    default:
      usage(*argv);
      std::exit(EXIT_FAILURE);
  }

  std::cerr << "Adaptive binarization\n"
            << "Threshold calculation: ";

  std::cerr << "parameter k=" << optK << std::endl;

  // Load the image in grayscale mode
  cv::Mat input = imread(inputname, cv::ImreadModes::IMREAD_GRAYSCALE);

  if ((input.rows <= 0) || (input.cols <= 0)) {
    std::cerr << "*** ERROR: Couldn't read input image " << inputname << std::endl;
    std::exit(EXIT_FAILURE);
  }

  // Treat the window size
  if (winx <= 0 || winy <= 0) {
    std::cerr << "Input size: " << input.cols << "x" << input.rows << std::endl;
    winy = (2 * input.rows - 1) / 3;
    winx = input.cols - 1 < winy ? input.cols - 1 : winy;
    // if the window is too big, then we asume that the image
    // is not a single text box, but a document page: set
    // the window size to a fixed constant.
    if (winx > MAX_WINX) {
      winx = DEFAULT_WINX;
      winy = DEFAULT_WINY;
    }
    std::cerr << "Setting window size to [" << winx << "," << winy << "].\n";
  }

  // Threshold
  cv::Mat output(input.rows, input.cols, CV_8U);
  NiblackSauvolaWolfJolion(input, output, methodCode, winx, winy, optK);

  // Write the tresholded file
  std::cerr << "Writing binarized image to file '" << outputname << "'.\n";
  imwrite(outputname, output);

  return EXIT_SUCCESS;
}
