#include <iostream>
#include <opencv2/opencv.hpp>
#include <ImageProcessing.h>
#include <RetinaModel.h>

using namespace std;

void drawBbox(cv::Mat &image, Grid<float> grid,float scale=1.0f){
    for(int i = 0; i < grid.rows(); i++){
        cv::Rect bbox( cv::Point(grid(0,i), grid(1,i)), cv::Point(grid(2,i),grid(3,i)) );
        cv::rectangle(image, bbox, cv::Scalar(255,0,255), int(scale));
        cv::putText(image, to_string(grid(4,i)),cv::Point(grid(0,i), grid(1,i)+12*scale),cv::FONT_HERSHEY_DUPLEX , 1.0, cv::Scalar(255,255,255));
    }
}

void drawLandmarks(cv::Mat &image, Grid<float> grid){
    // grid[:,5:15] are the landmarks idx
    for(int i = 0; i < grid.rows(); i++){
        cv::circle(image, cv::Point(grid(5,i), grid(6,i)), 2, cv::Scalar(0, 0, 255), -1);
        cv::circle(image, cv::Point(grid(7,i), grid(8,i)), 2, cv::Scalar(0, 255, 255), -1);
        cv::circle(image, cv::Point(grid(9,i), grid(10,i)), 2, cv::Scalar(255, 0, 255), -1);
        cv::circle(image, cv::Point(grid(11,i), grid(12,i)), 2, cv::Scalar(0, 255, 0), -1);
        cv::circle(image, cv::Point(grid(13,i), grid(14,i)), 2, cv::Scalar(255, 0, 0), -1);
    }
}

int main(int argc, char** argv) {
    RetinaModel model("model/retinaface_dynamic.onnx");
    string image_path;
    float scale = 0.0;
    cv::Mat im;

    model.init();

    if(argc < 2){
        cerr << "Insufficient Arguments" << endl;
        return EXIT_FAILURE;
    }
    
    image_path = argv[1];
    im = cv::imread(image_path);

    cout << model << endl;

    Grid<float> outputs;
    model.getInference(im, outputs);

    cout << "End Process: Outputs = " << outputs.rows() << endl;

    if(outputs.rows()){
        drawBbox(im, outputs, scale);
        drawLandmarks(im, outputs);
    }
    cv::imshow("result", im);
    cv::waitKey(0);

    return EXIT_SUCCESS;
}
