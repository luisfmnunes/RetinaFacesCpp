#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include<fstream>
#include<opencv2/opencv.hpp>
#include<onnx/onnxruntime_cxx_api.h>
#include<Grid.h>
#include<Config.h>

void inputPreProcessing(cv::Mat &im, float &det_scale, bool reshape = true, int img_size = 640);
Grid<float> outputPostProcessing(std::vector<Grid<float>> tensors, Config_Data cfg, bool debug = false, float det_scale = 1, cv::Size image_size= cv::Size(640,640), float conf_th = 0.7, int top_k=5000, int keep_top_k=750);
template<typename T> Grid<T> anchors_grid(Config_Data cfg, cv::Size image_size = cv::Size(640,640));


#endif