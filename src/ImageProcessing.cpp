#include <ImageProcessing.h>

void hwc_to_chw(cv::InputArray src, cv::OutputArray dst){
    const int src_h = src.rows();
    const int src_w = src.cols();
    const int src_c = src.channels();

    cv::Mat hw_c = src.getMat().reshape(1, src_h * src_w);

    const std::array<int,3>dims = {src_c, src_h, src_w};
    dst.create(3, &dims[0], CV_MAKETYPE(src.depth(), 1));
    cv::Mat dst_1d = dst.getMat().reshape(1, {src_c, src_h, src_w});

    cv::transpose(hw_c, dst_1d);
}

void chw_to_hwc(cv::InputArray src, cv::OutputArray dst){
    const auto& src_size = src.getMat().size;
    const int src_c = src_size[0];
    const int src_h = src_size[1];
    const int src_w = src_size[2];

    auto c_hw = src.getMat().reshape(0, {src_c, src_h * src_w});

    dst.create(src_h, src_w, CV_MAKE_TYPE(src.depth(), src_c));
    cv::Mat dst_1d = dst.getMat().reshape(src_c, {src_h, src_w});

    cv::transpose(c_hw, dst_1d);
}

void inputPreProcessing(cv::Mat &im, float &det_scale, bool reshape, int img_size){

    im -= cv::Scalar(103, 117, 123);
    im.convertTo(im, CV_32FC3);

    if(reshape){
        cv::Mat det_im = cv::Mat::zeros(cv::Size(img_size,img_size), CV_32FC3);
        cv::Mat resized_img;

        int new_height, new_width;
        float im_ratio = float(im.rows)/float(im.cols);
        if(im_ratio > 1){
            new_height = img_size;
            new_width = int(new_height / im_ratio);
        } else {
            new_width = img_size;
            new_height = int(new_width * im_ratio);
        }

        // std::cout << "Ratio: " << im_ratio << ", New Height: " << new_height << ", New Width: " << new_width << std::endl;

        det_scale = float(new_height / im.rows);
        cv::resize(im, resized_img, cv::Size(new_width,new_height));
        resized_img.copyTo(det_im(cv::Rect(0,0,new_width,new_height)));

        im = det_im;
    }

}

void outputPostProcessing(std::vector<Grid<float>>tensors, Config_Data cfg, cv::Size image_size, float conf_th, int top_k, int keep_top_k){
    Grid<float> anchors = anchors_grid<float>(cfg, image_size);
    std::cout << anchors << std::endl;
}

template<typename T> Grid<T> anchors_grid(Config_Data cfg, cv::Size image_size){
    std::vector<T> anchors;
    for(size_t i = 0; i < 3; i++){
        std::vector<int> min_sz = cfg.min_sizes[i];
        
        int fy = image_size.height/cfg.steps[i];
        int fx = image_size.width/cfg.steps[i];

        for(size_t x = 0; x < fx; x++){
            for(size_t y = 0; y < fy; y++){
                for(size_t s = 0; s < min_sz.size(); s++){
                    float s_kx = min_sz[s]/image_size.width;
                    float s_ky = min_sz[s]/image_size.height;
                    float dense_cx = (x + 0.5) * cfg.steps[i]/image_size.width;
                    float dense_cy = (y + 0.5) * cfg.steps[i]/image_size.height;
                    
                    anchors.push_back(dense_cx);
                    anchors.push_back(dense_cy);
                    anchors.push_back(s_kx);
                    anchors.push_back(s_ky);

                }
            }
        }

        // anchors.emplace_back(anchor_data.size()/4,4);
        // anchors.back().setData(anchor_data.data(), anchor_data.size());
    }

    Grid<T> final_grid(anchors.size()/4, 4);
    final_grid.setData(anchors.data(), anchors.size());

    return final_grid;
}

template<typename T> static std::vector<Grid<T>> decode(Grid<T> data, Grid<T> anchors, Config_Data cfg){

}