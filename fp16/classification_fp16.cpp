#include <caffe/caffe.hpp>
#ifdef USE_OPENCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#endif  // USE_OPENCV
#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if defined(USE_OPENCV) && defined(HAS_HALF_SUPPORT)
using namespace caffe;  // NOLINT(build/namespaces)
using std::string;
using caffe::Timer;

#define Dtype half

/* Pair (label, confidence) representing a prediction. */
typedef std::pair<string, Dtype> Prediction;

class Classifier {
 public:
  Classifier(const string& model_file,
             const string& trained_file,
             const string& mean_file,
             const string& label_file);

  std::vector<Prediction> Classify(const cv::Mat& img, int_tp N = 5);

 private:
  void SetMean(const string& mean_file);

  std::vector<Dtype> Predict(const cv::Mat& img);

  void WrapInputLayer(std::vector<Dtype *> &input_channels);

  void Preprocess(const cv::Mat& img,
                  std::vector<Dtype *> input_channels);
 
 private:
  shared_ptr<Net<Dtype> > net_;
  cv::Size input_geometry_;
  int_tp num_channels_;
  cv::Mat mean_;
  std::vector<string> labels_;
};

// Get all available GPU devices
static void get_gpus(vector<int>* gpus) {
    int count = 0;
#ifndef CPU_ONLY
    count = Caffe::EnumerateDevices(true);
#else
    NO_GPU;
#endif
    for (int i = 0; i < count; ++i) {
      gpus->push_back(i);
    }
}

Classifier::Classifier(const string& model_file,
                       const string& trained_file,
                       const string& mean_file,
                       const string& label_file) {
  // Set device id and mode
  vector<int> gpus;
  get_gpus(&gpus);
  if (gpus.size() != 0) {
#ifndef CPU_ONLY
    std::cout << "Use GPU with device ID " << gpus[0] << std::endl;
    //Caffe::SetDevices(gpus);
    Caffe::set_mode(Caffe::GPU);
    Caffe::SetDevice(gpus[0]);
#endif  // !CPU_ONLY
  } else {
    std::cout << "Use CPU" << std::endl;
    Caffe::set_mode(Caffe::CPU);
  }

  /* Load the network. */
  net_.reset(new Net<Dtype>(model_file, TEST, Caffe::GetDefaultDevice()));
  net_->CopyTrainedLayersFrom(trained_file);

  CHECK_EQ(net_->num_inputs(), 1) << "Network should have exactly one input.";
  CHECK_EQ(net_->num_outputs(), 1) << "Network should have exactly one output.";

  Blob<Dtype>* input_layer = net_->input_blobs()[0];
  num_channels_ = input_layer->channels();
  CHECK(num_channels_ == 3 || num_channels_ == 1)
    << "Input layer should have 1 or 3 channels.";
  input_geometry_ = cv::Size(input_layer->width(), input_layer->height());

  /* Load the binaryproto mean file. */
  SetMean(mean_file);

  /* Load labels. */
  std::ifstream labels(label_file.c_str());
  CHECK(labels) << "Unable to open labels file " << label_file;
  string line;
  while (std::getline(labels, line))
    labels_.push_back(string(line));

  Blob<Dtype>* output_layer = net_->output_blobs()[0];
  CHECK_EQ(labels_.size(), output_layer->shape(1))
    << "Number of labels is different from the output layer dimension.";
}

static bool PairCompare(const std::pair<Dtype, int_tp>& lhs,
                        const std::pair<Dtype, int_tp>& rhs) {
  return lhs.first > rhs.first;
}

/* Return the indices of the top N values of vector v. */
static std::vector<int_tp> Argmax(const std::vector<Dtype>& v, int_tp N) {
  std::vector<std::pair<Dtype, int_tp> > pairs;
  for (size_t i = 0; i < v.size(); ++i) {
    pairs.push_back(std::make_pair(v[i], static_cast<int_tp>(i)));
  }
  std::partial_sort(pairs.begin(), pairs.begin() + N, pairs.end(), PairCompare);

  std::vector<int_tp> result;
  for (int_tp i = 0; i < N; ++i)
    result.push_back(pairs[i].second);
  return result;
}

/* Return the top N predictions. */
std::vector<Prediction> Classifier::Classify(const cv::Mat& img, int_tp N) {
  std::vector<Dtype> output = Predict(img);

  N = std::min<int_tp>(labels_.size(), N);
  std::vector<int_tp> maxN = Argmax(output, N);
  std::vector<Prediction> predictions;
  for (int_tp i = 0; i < N; ++i) {
    int_tp idx = maxN[i];
    predictions.push_back(std::make_pair(labels_[idx], output[idx]));
  }

  return predictions;
}

/* Load the mean file in binaryproto format. */
void Classifier::SetMean(const string& mean_file) {
  BlobProto blob_proto;
  ReadProtoFromBinaryFileOrDie(mean_file.c_str(), &blob_proto);

  /* Convert from BlobProto to Blob<float> */
  Blob<float> mean_blob;
  mean_blob.FromProto(blob_proto);
  CHECK_EQ(mean_blob.shape(1), num_channels_)
    << "Number of channels of mean file doesn't match input layer.";

  /* The format of the mean file is planar 32-bit float BGR or grayscale. */
  std::vector<cv::Mat> channels;
  float* data = mean_blob.mutable_cpu_data();
  for (int_tp i = 0; i < num_channels_; ++i) {
    /* Extract an individual channel. */
    cv::Mat channel(mean_blob.height(), mean_blob.width(), CV_32FC1, data);
    channels.push_back(channel);
    data += mean_blob.height() * mean_blob.width();
  }

  /* Merge the separate channels into a single image. */
  cv::Mat mean;
  cv::merge(channels, mean);

  /* Compute the global mean pixel value and create a mean image
   * filled with this value. */
  cv::Scalar channel_mean = cv::mean(mean);
  mean_ = cv::Mat(input_geometry_, mean.type(), channel_mean);
}

std::vector<Dtype> Classifier::Predict(const cv::Mat& img) {
  Blob<Dtype>* input_layer = net_->input_blobs()[0];
  input_layer->Reshape(1, num_channels_,
                       input_geometry_.height, input_geometry_.width);
  /* Forward dimension change to all layers. */
  net_->Reshape();

  std::vector<Dtype *> input_channels;
  WrapInputLayer(input_channels);

  Preprocess(img, input_channels);

  net_->Forward();

  /* Copy the output layer to a std::vector */
  Blob<Dtype>* output_layer = net_->output_blobs()[0];
  const Dtype* begin = output_layer->cpu_data();
  const Dtype* end = begin + output_layer->shape(1);
  std::cout << begin[0];
  return std::vector<Dtype>(begin, end);
}

/* Wrap the input layer of the network in separate cv::Mat objects
 * (one per channel). This way we save one memcpy operation and we
 * don't need to rely on cudaMemcpy2D. The last preprocessing
 * operation will write the separate channels directly to the input
 * layer. */
void Classifier::WrapInputLayer(std::vector<Dtype *> &input_channels) {
  Blob<Dtype>* input_layer = net_->input_blobs()[0];

  int_tp width = input_layer->width();
  int_tp height = input_layer->height();
  Dtype* input_data = input_layer->mutable_cpu_data();
  for (int i = 0; i < input_layer->channels(); ++i) {
    input_channels.push_back(input_data);
    input_data += width * height;
  }
}

void Classifier::Preprocess(const cv::Mat& img,
                          std::vector<Dtype *> input_channels) {
  /* Convert the input image to the input image format of the network. */
  cv::Mat sample;
  if (img.channels()== 3 && num_channels_ == 1)
    cv::cvtColor(img, sample, cv::COLOR_BGR2GRAY);
  else if (img.channels() == 4 && num_channels_ == 1)
    cv::cvtColor(img, sample, cv::COLOR_BGRA2GRAY);
  else if (img.channels() == 4 && num_channels_ == 3)
    cv::cvtColor(img, sample, cv::COLOR_BGRA2BGR);
  else if (img.channels() == 1 && num_channels_ == 3)
    cv::cvtColor(img, sample, cv::COLOR_GRAY2BGR);
  else
    sample = img;

  cv::Mat sample_resized;
  if (sample.size() != input_geometry_)
    cv::resize(sample, sample_resized, input_geometry_);
  else
    sample_resized = sample;

  cv::Mat sample_Dtype;
  if (num_channels_ == 3)
    sample_resized.convertTo(sample_Dtype, CV_32FC3);
  else
    sample_resized.convertTo(sample_Dtype, CV_32FC1);

  cv::Mat sample_normalized;
  cv::subtract(sample_Dtype, mean_, sample_normalized);

  /* This operation will write the separate BGR planes directly to the
   * input layer of the network because it is wrapped by the cv::Mat
   * objects in input_channels. */
  //cv::split(sample_normalized, *input_channels);
  for( int i = 0; i < input_geometry_.height; i++) {
    for( int j = 0; j < input_geometry_.width; j++) {
      int pos = i * input_geometry_.width + j;
      if (num_channels_ == 3) {
        cv::Vec3f pixel = sample_normalized.at<cv::Vec3f>(i, j);
        input_channels[0][pos] = pixel.val[0];
        input_channels[1][pos] = pixel.val[1];
        input_channels[2][pos] = pixel.val[2];  
      } else {
        cv::Scalar pixel = sample_normalized.at<float>(i, j);
        input_channels[0][pos] = pixel.val[0];
      }
    }
  }
std::cout <<  input_channels[0][0] << " , " <<  input_channels[1][100] << std::endl;
}

int main(int argc, char** argv) {
  if (argc != 6) {
    std::cerr << "Usage: " << argv[0]
              << " deploy.prototxt network.caffemodel"
              << " mean.binaryproto labels.txt img.jpg" << std::endl;
    return 1;
  }

  ::google::InitGoogleLogging(argv[0]);

  string model_file   = argv[1];
  string trained_file = argv[2];
  string mean_file    = argv[3];
  string label_file   = argv[4];
  Classifier classifier(model_file, trained_file, mean_file, label_file);

  string file = argv[5];

  std::cout << "---------- Prediction for "
            << file << " ----------" << std::endl;

  cv::Mat img = cv::imread(file, -1);
  CHECK(!img.empty()) << "Unable to decode image " << file;
  Timer detect_timer;
  double timeUsed;
	
  detect_timer.Start();
  std::vector<Prediction> predictions = classifier.Classify(img);
  timeUsed = detect_timer.MilliSeconds();
  std::cout << "the first detect time=" << timeUsed <<"ms\n";  
  
  detect_timer.Start();
  predictions = classifier.Classify(img);
  detect_timer.Stop();
  timeUsed = detect_timer.MilliSeconds();
  std::cout << "the second detect time=" << timeUsed <<"ms\n";
  
  /* Print the top N predictions. */
  for (uint_tp i = 0; i < predictions.size(); ++i) {
    Prediction p = predictions[i];
    std::cout << std::fixed << std::setprecision(4) << p.second << " - \""
              << p.first << "\"" << std::endl;
  }
}
#else
int_tp main(int_tp argc, char** argv) {
  LOG(FATAL) << "This example requires OpenCV; compile with USE_OPENCV.";
}
#endif  // USE_OPENCV
