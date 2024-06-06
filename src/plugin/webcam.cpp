/*
   ____ _            _            _             _
  / ___| | ___   ___| | __  _ __ | |_   _  __ _(_)_ __
 | |   | |/ _ \ / __| |/ / | '_ \| | | | |/ _` | | '_ \
 | |___| | (_) | (__|   <  | |_) | | |_| | (_| | | | | |
  \____|_|\___/ \___|_|\_\ | .__/|_|\__,_|\__, |_|_| |_|
                           |_|            |___/
Produces current time and date
*/

#include "../source.hpp"
#include <chrono>
#include <nlohmann/json.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/photo.hpp>
#include <pugg/Kernel.h>
#include <sstream>
#include <thread>

#ifndef PLUGIN_NAME
#define PLUGIN_NAME "clock"
#endif

using namespace std;
using namespace cv;
using namespace std::chrono;
using json = nlohmann::json;

template <typename T>
inline T mapVal(T x, T a, T b, T c, T d)
{
    x = ::max(::min(x, b), a);
    return c + (d-c) * (x-a) / (b-a);
}


// Plugin class. This shall be the only part that needs to be modified,
// implementing the actual functionality
class Webcam : public Source<json> {
public:
  static void colorizeFlow(const Mat &u, const Mat &v, Mat &dst) {
    double uMin, uMax;
    cv::minMaxLoc(u, &uMin, &uMax, 0, 0);
    double vMin, vMax;
    cv::minMaxLoc(v, &vMin, &vMax, 0, 0);
    uMin = ::abs(uMin);
    uMax = ::abs(uMax);
    vMin = ::abs(vMin);
    vMax = ::abs(vMax);
    float dMax =
        static_cast<float>(::max(::max(uMin, uMax), ::max(vMin, vMax)));

    dst.create(u.size(), CV_8UC3);
    for (int y = 0; y < u.rows; ++y) {
      for (int x = 0; x < u.cols; ++x) {
        dst.at<uchar>(y, 3 * x) = 0;
        dst.at<uchar>(y, 3 * x + 1) =
            (uchar)mapVal(-v.at<float>(y, x), -dMax, dMax, 0.f, 255.f);
        dst.at<uchar>(y, 3 * x + 2) =
            (uchar)mapVal(u.at<float>(y, x), -dMax, dMax, 0.f, 255.f);
      }
    }
  }

  Webcam() {
    _error = "none";
    _blob_format = "jpg";
  }

  ~Webcam() {
    _cap.release();
    destroyAllWindows();
  }

  string kind() override { return PLUGIN_NAME; }

  static string get_ISO8601(
      const system_clock::time_point &time = chrono::system_clock::now()) {
    time_t tt = system_clock::to_time_t(time);
    tm *tt2 = localtime(&tt);

    // Get milliseconds hack
    auto timeTruncated = system_clock::from_time_t(tt);
    int ms =
        std::chrono::duration_cast<milliseconds>(time - timeTruncated).count();

    return (
               stringstream()
               << put_time(tt2, "%FT%T")               // "2023-03-30T19:49:53"
               << "." << setw(3) << setfill('0') << ms // ".005"
               << put_time(tt2, "%z") // "+0200" (time zone offset, optional)
               )
        .str();
  }

  return_type get_output(json *out,
                         std::vector<unsigned char> *blob = nullptr) override {
    return_type result = return_type::success;
    Mat hist, overlay, flipped;
    while (true) {
      _cap >> _frame;
      if (_frame.empty()) {
        _error = "Error: Unable to capture frame";
        return return_type::error;
      }
      resize(_frame, _frame, Size(), _scale, _scale);
      hist.release();
      overlay = Mat::zeros(_frame.size(), _frame.type());
      
      // prepare histogram
      hist.release();
      cvtColor(_frame, _gray, COLOR_BGR2GRAY);
      calcHist(&_gray,
               1,          // number of images
               0,          // channels
               Mat(),      // mask
               hist,       // histogram
               1,          // dimensionality
               &_hist_size, // number of bins
               0);
      normalize(hist, hist, 0, 255, NORM_MINMAX, CV_32F);

      // draw histogram
      double ratio = 5.0;
      int inset = 5;
      int hist_w = _frame.cols / ratio, hist_h = _frame.rows / ratio;
      double bin_w = (double)hist_w / _hist_size;
      int off_x = _frame.cols - hist_w - inset;
      int off_y = -inset;
      // draw histogram
      for (int i = 1; i < _hist_size; i++) {
        line(overlay,
             Point(bin_w * (i - 1) + off_x,
                   _frame.rows - cvRound(hist.at<float>(i - 1) / 255 * hist_h) +
                       off_y),
             Point(bin_w * i + off_x,
                   _frame.rows - cvRound(hist.at<float>(i) / 255 * hist_h) +
                       off_y),
             Scalar(200, 200, 200), 1, LINE_AA, 0);
      }
      // draw axes
      line(overlay, Point(off_x, _frame.rows - inset),
           Point(off_x, _frame.rows - inset - hist_h), Scalar(180, 180, 180), 1,
           LINE_AA, 0);
      line(overlay, Point(off_x, _frame.rows - inset),
           Point(off_x + hist_w, _frame.rows - inset), Scalar(180, 180, 180), 1,
           LINE_AA, 0);

      // timestamp image
      putText(overlay, get_ISO8601(), Point{5, _frame.rows - 5}, 0, 0.5,
              Scalar(200, 200, 200), 1, 8, false);

      // Optical flow
      if (_params.contains("optical_flow") &&
          _params["optical_flow"].is_boolean() && _params["optical_flow"]) {
        if (_previous.size() == _frame.size()) {
          calcOpticalFlowFarneback(_gray, _previous, _flowxy, 0.5, 3, 3, 5, 5,
                                  1.5, 0);
          split(_flowxy, _planes);
          colorizeFlow(_planes[0], _planes[1], _flowxy);
          imshow("flow", _flowxy);
        }
        _gray.copyTo(_previous);
      }

      // Stylize and display
      stylization(_frame, _frame);
      if (_flip) {
        flip(_frame, flipped, 1);
        imshow("frame", flipped + overlay);
      } else {
        imshow("frame", _frame + overlay);
      }

      // GUI
      char k = waitKey(1000.0 / 25);
      if (' ' == k) {
        Mat out_image = _frame + overlay;
        imwrite(_image_name, out_image);
        if (blob) {
          imencode("." + _blob_format, out_image, *blob);
        }
        break;
      } else if ('q' == k) {
        result = return_type::critical;
        _error = "Quit requested";
        break;
      }
    }

    // Prepare output
    if (return_type::success == result) {
      (*out)["frame_size"] = {cvRound(_cap.get(CAP_PROP_FRAME_WIDTH)),
                              cvRound(_cap.get(CAP_PROP_FRAME_HEIGHT))};
      (*out)["image_size"] = {_frame.cols, _frame.rows};
      (*out)["histogram"] = json::array();
      vector<uint16_t> hist_data;
      if (hist.isContinuous()) {
        hist_data.assign((float *)hist.datastart, (float *)hist.dataend);
        (*out)["histogram"] = hist_data;
      } else {
        for (int i = 0; i < _hist_size; i++) {
          (*out)["histogram"].push_back(cvRound(hist.at<float>(i)));
        }
      }
    }

    return result;
  }

  void set_params(void *params) override {
    _params = *(json *)params;
    if (_params.contains("device")) {
      _device = _params["device"].template get<int>();
    }
    _cap.open(_device);
    if (!_cap.isOpened()) {
      throw runtime_error("Error: Unable to open webcam");
    }
    if (_params.contains("hist_size") && _params["hist_size"].is_number()) {
      _hist_size = _params["hist_size"];
    }
    if (_params.contains("image_name") && _params["image_name"].is_string()) {
      _image_name = _params["image_name"];
    }
    if (_params.contains("scale") && _params["scale"].is_number()) {
      _scale = _params["scale"];
    }
    if (_params.contains("flip") && _params["flip"].is_boolean()) {
      _flip = _params["flip"];
    }
  }

  map<string, string> info() override {
    map<string, string> m{};
    m["device"] = to_string(_device);
    m["image_name"] = _image_name;
    m["hist_size"] = to_string(_hist_size);
    m["image_name"] = _image_name;
    m["scale"] = to_string(_scale);
    m["flip"] = _flip ? "yes" : "no";
    return m;
  }

private:
  json _data, _params;
  int _device = 0;
  int _hist_size = 20;
  float _scale = 0.3;
  string _image_name = "image.jpg";
  Mat _frame, _gray, _previous, _flowxy;
  Mat _planes[2];
  VideoCapture _cap;
  bool _flip = false;
};

/*
  ____  _             _             _      _
 |  _ \| |_   _  __ _(_)_ __     __| |_ __(_)_   _____ _ __
 | |_) | | | | |/ _` | | '_ \   / _` | '__| \ \ / / _ \ '__|
 |  __/| | |_| | (_| | | | | | | (_| | |  | |\ V /  __/ |
 |_|   |_|\__,_|\__, |_|_| |_|  \__,_|_|  |_| \_/ \___|_|
                |___/
This is the plugin driver, it should not need to be modified
*/

class WebcamDriver : public SourceDriver<json> {
public:
  WebcamDriver() : SourceDriver(PLUGIN_NAME, Webcam::version) {}
  Source<json> *create() { return new Webcam(); }
};

extern "C" EXPORTIT void register_pugg_plugin(pugg::Kernel *kernel) {
  kernel->add_driver(new WebcamDriver());
}

/*
                  _
  _ __ ___   __ _(_)_ __
 | '_ ` _ \ / _` | | '_ \
 | | | | | | (_| | | | | |
 |_| |_| |_|\__,_|_|_| |_|

For testing purposes, when directly executing the plugin
*/
int main(int argc, char const *argv[]) {
  Webcam wc;
  json output;
  vector<unsigned char> blob;
  wc.set_params(new json({
    {"device", 0}, 
    {"image_name", "image.jpg"}, 
    {"hist_size", 50},
    {"scale", 1/3.0},
    {"flip", true}
  }));

  cout << "Params: " << endl;
  for (auto &[k, v] : wc.info()) {
    cout << k << ": " << v << endl;
  }

  cout << "Press space to capture image, q to quit" << endl;

  while (wc.get_output(&output, &blob) == return_type::success) {
    cout << "Webcam plugin output: " << output.dump() << endl;
    cout << "Image size: " << blob.size() << " bytes" << endl;
  }

  return 0;
}
