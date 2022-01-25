#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include<opencv2/opencv.hpp>
#include<onnxruntime_cxx_api.h>
#include<Config.h>

void inputPreProcessing(cv::Mat &im, float &det_scale, bool reshape = true, int img_size = 640);
void outputPostProcessing(std::vector<Ort::Value> tensors, int image_size=640, float conf_th = 0.02, int top_k=5000, int keep_top_k=750);


#endif