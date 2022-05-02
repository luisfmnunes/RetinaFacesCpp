#include <RetinaModel.h>

RetinaModel::RetinaModel() : env(NULL), session(NULL) {}

RetinaModel::RetinaModel(bool debug) : env(NULL), session(NULL), debug(debug) {} 

RetinaModel::RetinaModel(const std::string model_path) : model_path(model_path), env(NULL), session(NULL){}

RetinaModel::RetinaModel(Ort::Env* &env, const std::string model_path) : env(env), model_path(model_path), session(NULL){}

RetinaModel::~RetinaModel() {}

static int getNameIndex(const std::string name, const std::vector<std::string> vec){
    for(int i = 0; i < vec.size(); i++)
        if(name==vec[i]) return i;
    return -1;
}

int RetinaModel::_init(Ort::Env* env, const std::string model_path, bool debug, GraphOptimizationLevel opt_level, int threads){
    
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::SessionOptions options;
    OrtThreadingOptions *thOpts;
    OrtStatusPtr status;
    status = Ort::GetApi().CreateThreadingOptions(&thOpts);
    status = Ort::GetApi().SetGlobalIntraOpNumThreads(thOpts, threads);
    status = Ort::GetApi().SetGlobalInterOpNumThreads(thOpts, threads);

    options.SetExecutionMode(ORT_SEQUENTIAL);
    options.DisablePerSessionThreads();
    options.SetGraphOptimizationLevel(opt_level);
    options.SetInterOpNumThreads(threads);
    options.SetIntraOpNumThreads(threads);

    this->debug = debug;

    if(env == NULL){
        if(this->env==NULL)
            this->env = new Ort::Env(thOpts, ORT_LOGGING_LEVEL_FATAL, "retina_env");
        if (!this->env) return EXIT_FAILURE;
    } else {
        this->env = env;
    }

    if(model_path.empty()){
        if(!this->model_path.empty()){
            this->session = new Ort::Session(*(this->env), this->model_path.c_str(), options);
            if(!this->session) return EXIT_FAILURE;
        }
    } else {
        this->session = new Ort::Session(*(this->env), model_path.c_str(), options);
        if(!this->session) return EXIT_FAILURE;
    }

    this->input_count = session->GetInputCount();
    this->output_count = session->GetOutputCount();

    for(int i = 0; i < this->input_count; i++){
        const char* input = this->session->GetInputName(i, allocator);
        this->input_names.emplace_back(input);
        allocator.Free((void*) input);
    }

    for(int i = 0; i < this->output_count; i++){
        const char* output = this->session->GetOutputName(i, allocator);
        this->output_names.emplace_back(output);
        allocator.Free((void*) output);
    }

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

    Ort::GetApi().ReleaseThreadingOptions(thOpts);

    return EXIT_SUCCESS;
}

int RetinaModel::init(){
    return _init();
}

int RetinaModel::init(Ort::Env* &env){
    return _init(env);
}

int RetinaModel::init(const std::string model_path, bool debug, GraphOptimizationLevel opt_level, int threads){
    return _init(this->env, model_path, debug, opt_level, threads);
}

int RetinaModel::init(Ort::Env* &env, const std::string model_path, bool debug, GraphOptimizationLevel opt_level, int threads){
    return _init(env, model_path, debug, opt_level, threads);
}

void RetinaModel::release(){
    if(this->env) delete this->env;
    if(this->session) delete this->session;
}

std::string RetinaModel::getModelPath(){ return this->model_path; }
void RetinaModel::setModelPath(std::string model_path){ this->model_path = model_path; }

// void RetinaModel::setOptions(GraphOptimizationLevel opt_level, int threads){
//     this->options.SetGraphOptimizationLevel(opt_level);
//     this->options.SetInterOpNumThreads(threads);
//     this->options.SetIntraOpNumThreads(threads); 
// }

int RetinaModel::getInference(cv::Mat &image, Grid<float> &output, bool resize, size_t img_size){
    float det_scale = 0.0f;
    cv::Mat input(image);

    if(!env || !session)
        return EXIT_FAILURE;

    Ort::AllocatorWithDefaultOptions allocator;
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    inputPreProcessing(input, det_scale, resize, img_size);

    std::vector<int64_t> dims(this->input_shapes[this->input_names[0]]);
    dims[0] = 1;
    dims[2] = input.rows;
    dims[3] = input.cols;

    // std::cout << dims << std::endl;

    std::vector<const char*> input_nm(this->input_count);
    for(int i = 0; i < this->input_count; i++)
        input_nm[i] = this->session->GetInputName(i, allocator);

    size_t data_size = 1;
    for (auto dim : dims) data_size *= dim;

    // same as reinterpreted_cast<const float*>
    std::vector<float> input_data(data_size);
    input_data.assign((float*)input.datastart, (float*)input.dataend); 

    if(this->debug) std::cout << "[RetinaFace Debug] Creating Input Tensor" << std::endl;
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_data.data(), input_data.size(), dims.data(), dims.size());
    if(!input_tensor.IsTensor())
        return EXIT_FAILURE;

    std::vector<const char*> output_names(this->output_count);
    for (int i = 0; i < this->output_count; i++) output_names[i] = this->session->GetOutputName(i, allocator);
    // for (auto name : output_names) std::cout << name << std::endl;

    if(this->debug) std::cout << "[RetinaFace Debug] Running Inference" << std::endl;
    auto output_tensor = this->session->Run(Ort::RunOptions{nullptr}, input_nm.data(), &input_tensor, 1, output_names.data(), output_names.size());
    assert(output_tensor.front().IsTensor());

    std::vector<Grid<float>> out_tensors;
    for(int i=0; i < output_tensor.size(); i++){
        if(!output_tensor[i].IsTensor())
            return EXIT_FAILURE;
        std::vector<int64_t> tensor_dim = output_tensor[i].GetTensorTypeAndShapeInfo().GetShape();
        // std::cout << output_names[i] << ": " << tensor_dim << std::endl;
        
        out_tensors.emplace_back(tensor_dim[2], tensor_dim[1]);
        float* data = output_tensor[i].GetTensorMutableData<float>();
        size_t data_size = tensor_dim[1]*tensor_dim[2];
        out_tensors.back().setData(data, data_size);
    }

    Config_Data cfg = get_R50_config();
    output = outputPostProcessing(out_tensors, cfg, this->debug, det_scale);

    //ADD THE ALLOCATOR.FREE to FREE THE INPUT AND OUTPUT NAMES!
    for(auto &name : input_nm) allocator.Free((void*) name);
    for(auto &name : output_names) allocator.Free((void*) name);

    return EXIT_SUCCESS;
}

bool RetinaModel::isModelInitialized(){
    return(this->session != NULL);
}