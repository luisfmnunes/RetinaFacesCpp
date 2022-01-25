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

void outputPostProcessing(std::vector<Ort::Value > tensors, int image_size=640, float conf_th = 0.02, int top_k=5000, int keep_top_k=750){
    
}
