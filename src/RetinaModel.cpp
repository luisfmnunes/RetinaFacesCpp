#include <RetinaModel.h>

RetinaModel::RetinaModel(){}

RetinaModel::RetinaModel(const std::string model_path) : model_path(model_path){}

RetinaModel::RetinaModel(Ort::Env* &env, const std::string model_path) : env(env), model_path(model_path){}

RetinaModel::~RetinaModel(){
    if(this->env) delete this->env;
    if(this->session) delete this->session;
}

static int getNameIndex(const std::string name, const std::vector<std::string> vec){
    for(int i = 0; i < vec.size(); i++)
        if(name==vec[i]) return i;
    return -1;
}

int RetinaModel::_init(Ort::Env* env, const std::string model_path, GraphOptimizationLevel opt_level, int threads){
    
    Ort::AllocatorWithDefaultOptions allocator;
    this->options.SetGraphOptimizationLevel(opt_level);
    this->options.SetInterOpNumThreads(threads);
    this->options.SetIntraOpNumThreads(threads);

    if(env == NULL){
        if(this->env==NULL)
            this->env = new Ort::Env();
        if (!this->env) return EXIT_FAILURE;
    } else {
        this->env = env;
    }

    if(model_path.empty()){
        if(!this->model_path.empty()){
            this->session = new Ort::Session(*(this->env), this->model_path.c_str(), this->options);
            if(!this->session) return EXIT_FAILURE;
        }
    } else {
        this->session = new Ort::Session(*(this->env), model_path.c_str(), this->options);
        if(!this->session) return EXIT_FAILURE;
    }

    this->input_count = session->GetInputCount();
    this->output_count = session->GetOutputCount();

    for(int i = 0; i < this->input_count; i++)
        this->input_names.push_back(this->session->GetInputName(i, allocator));
    for(int i = 0; i < this->output_count; i++)
        this->output_names.push_back(this->session->GetOutputName(i, allocator));

    if(this->input_names.size() != this->input_count || this->output_names.size() != this->output_count) return EXIT_FAILURE;

    for(auto name : this->input_names){
        int idx = getNameIndex(name, this->input_names);
        if(idx<0) continue;
        this->input_shapes.emplace(name, this->session->GetInputTypeInfo(idx).GetTensorTypeAndShapeInfo().GetShape());
    }

    for(auto name : this->output_names){
        int idx = getNameIndex(name, this->output_names);
        if(idx<0) continue;
        this->output_shapes.emplace(name, this->session->GetOutputTypeInfo(idx).GetTensorTypeAndShapeInfo().GetShape());
    }

    return EXIT_SUCCESS;
}

int RetinaModel::init(){
    return _init();
}

int RetinaModel::init(Ort::Env* &env){
    return _init(env);
}

int RetinaModel::init(const std::string model_path, GraphOptimizationLevel opt_level, int threads){
    return _init(this->env, model_path, opt_level, threads);
}

int RetinaModel::init(Ort::Env* &env, const std::string model_path, GraphOptimizationLevel opt_level, int threads){
    return _init(env, model_path, opt_level, threads);
}

std::string RetinaModel::getModelPath(){ return this->model_path; }
void RetinaModel::setModelPath(std::string model_path){ this->model_path = model_path; }

void RetinaModel::setOptions(GraphOptimizationLevel opt_level, int threads){
    this->options.SetGraphOptimizationLevel(opt_level);
    this->options.SetInterOpNumThreads(threads);
    this->options.SetIntraOpNumThreads(threads); 
}

int RetinaModel::getInference(cv::Mat &image, std::vector<Ort::Value> &output, bool resize, size_t img_size){
    float det_scale = 0.0f;
    cv::Mat input(image);
    Ort::AllocatorWithDefaultOptions allocator;
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    inputPreProcessing(input, det_scale, resize, img_size);

    std::vector<int64_t> dims(this->input_shapes[this->input_names[0]]);
    dims[0] = 1;
    dims[2] = input.rows;
    dims[3] = input.cols;

    std::vector<const char*> input_nm(this->input_count);
    for(int i = 0; i < this->input_count; i++)
        input_nm[i] = this->session->GetInputName(i, allocator);

    size_t data_size = 1.0f;
    for (auto dim : dims) data_size *= dim;

    std::vector<float> input_data(data_size);
    input_data.assign((float*)input.datastart, (float*)input.dataend);

    std::cout << "Creating Input Tensor" << std::endl;
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_data.data(), input_data.size(), dims.data(), dims.size());
    if(!input_tensor.IsTensor())
        return EXIT_FAILURE;

    std::vector<const char*> output_names(this->output_count);
    for (int i = 0; i < this->output_count; i++) output_names[i] = this->session->GetOutputName(i, allocator);
    // for (auto name : output_names) std::cout << name << std::endl;

    std::cout << "Running Inference" << std::endl;
    auto output_tensor = this->session->Run(Ort::RunOptions{nullptr}, input_nm.data(), &input_tensor, 1, output_names.data(), output_names.size());
    // assert(output_tensor.front().IsTensor());
    while(!output.empty())
        output.pop_back();

    std::vector<Grid<float>> out_tensors;
    for(int i=0; i < output_tensor.size(); i++){
        if(!output_tensor[i].IsTensor())
            return EXIT_FAILURE;
        std::vector<int64_t> tensor_dim = output_tensor[i].GetTensorTypeAndShapeInfo().GetShape();
        std::cout << output_names[i] << ": " << tensor_dim << std::endl;
        
        out_tensors.emplace_back(tensor_dim[1], tensor_dim[2]);
        float* data = output_tensor[i].GetTensorMutableData<float>();
        size_t data_size = tensor_dim[1]*tensor_dim[2];
        out_tensors.back().setData(data, data_size);
    }

    Config_Data cfg = get_R50_config();
    outputPostProcessing(out_tensors, cfg);

    return EXIT_SUCCESS;
}

bool RetinaModel::isModelInitialized(){
    return(this->session != NULL);
}