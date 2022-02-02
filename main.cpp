#include <iostream>
#include <opencv2/opencv.hpp>
#include <ImageProcessing.h>
#include <RetinaModel.h>

using namespace std;

int main(int argc, char** argv) {
    RetinaModel model("model/retinaface_dynamic.onnx");
    string image_path;
    float scale = 0.0;
    cv::Mat im;
    
    if(argc < 2){
        cerr << "Insufficient Arguments" << endl;
        return EXIT_FAILURE;
    }
    
    image_path = argv[1];
    im = cv::imread(image_path);

    // cv::imshow("Read Image", im);
    // inputPreProcessing(im, scale);

    // cv::imshow("PreProcessing", im);
    // cv::waitKey(0);

    model.init();

    cout << model << endl;

    vector<Ort::Value> outputs;
    model.getInference(im, outputs);

    cout << "End Process: Outputs = " << outputs.size() << endl;

    return EXIT_SUCCESS;
}
