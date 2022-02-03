#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include <map>
#include <ImageProcessing.h>
#include <onnx/onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

class RetinaModel{

    private: 
        Ort::Env* env;
        Ort::Session* session;
        Ort::SessionOptions options;

        std::string model_path;

        int _init(Ort::Env* env = NULL, const std::string model_path = "", GraphOptimizationLevel opt_level = ORT_ENABLE_ALL, int threads = 0);

    public:

        int input_count;
        int output_count;
        std::vector<std::string> input_names;
        std::vector<std::string> output_names;

        // input dims
        std::map<std::string,std::vector<int64_t>> input_shapes;
        // output dims
        std::map<std::string,std::vector<int64_t>> output_shapes;

        RetinaModel();
        RetinaModel(const std::string model_path);
        RetinaModel(Ort::Env* &env, const std::string model_path);
        ~RetinaModel();

        int init();
        int init(Ort::Env* &env);
        int init(const std::string model_path, GraphOptimizationLevel opt_level = ORT_ENABLE_ALL, int threads = 0);
        int init(Ort::Env* &env, const std::string model_path, GraphOptimizationLevel opt_level = ORT_ENABLE_ALL, int threads = 0);

        std::string getModelPath();
        void setModelPath(std::string model_path);

        Ort::Env* getModelEnv();
        void setModelEnv(Ort::Env* &env);

        void setOptions(GraphOptimizationLevel opt_level, int threads = 0);
        int getInference(cv::Mat &image, Grid<float> &output, bool resize = true, size_t img_size = 640);

        bool isModelInitialized();

};

inline std::ostream& operator<<(std::ostream& os, const std::vector<int64_t> &v){
    int count = 0;
    os << "[ ";
    for(auto item : v){
        os << item;
        if(++count < v.size())
            os << ", ";
    }
    os << " ]";
    
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const RetinaModel& model){
    if(!model.input_count)
        os << "ONNX Model presents no inputs, model is probably not initialized" << std::endl;
    else{
        os << "Loaded Model Parameters [" << std::endl
        << "\t Input Count: " << model.input_count << std::endl
        << "\t Output Count: " << model.output_count << std::endl
        << "\t Inputs and Shapes: [" << std::endl;

        for(auto item : model.input_shapes)
            os << "\t\t" << item.first << ": " << item.second << std::endl;

        os << "\t ]" << std::endl << "\t Outputs and Shapes: [";
        if(model.output_shapes.size()) os << std::endl;

        for(auto item : model.output_shapes)
            os << "\t\t" << item.first << ": " << item.second << std::endl;
        
        os << "\t ]" << std::endl;
        os << "]";
    }

    return os;
}

#endif