#include <ImageProcessing.h>

cv::Mat hwc_to_chw(const cv::Mat &image){

    cv::Mat chw_image(image.size(), image.type());

    for(int i = 0; i < image.channels(); i++)
        cv::extractChannel(
            image,
            cv::Mat(
                image.size().height,
                image.size().width,
                CV_32FC1,
                &(chw_image.at<float>(image.size().height*image.size().width*i))
            ),
            i
        );

    return chw_image;
}

static std::ostream& operator<<(std::ostream& os, const std::vector<int> &v){
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

// void chw_to_hwc(cv::InputArray src, cv::OutputArray dst){
//     const auto& src_size = src.getMat().size;
//     const int src_c = src_size[0];
//     const int src_h = src_size[1];
//     const int src_w = src_size[2];

//     auto c_hw = src.getMat().reshape(0, {src_c, src_h * src_w});

//     dst.create(src_h, src_w, CV_MAKE_TYPE(src.depth(), src_c));
//     cv::Mat dst_1d = dst.getMat().reshape(src_c, {src_h, src_w});

//     cv::transpose(c_hw, dst_1d);
// }

void inputPreProcessing(cv::Mat &im, float &det_scale, bool reshape, int img_size){

#if CV_MAJOR_VERSION > 3
    // not required if using opencv 4
    im.convertTo(im, CV_32FC3);
    im -= cv::Scalar(103, 117, 123);
#endif

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

        det_scale = (float)(new_height) / (float)(im.rows);
        cv::resize(im, resized_img, cv::Size(new_width,new_height));
        resized_img.copyTo(det_im(cv::Rect(0,0,new_width,new_height)));

#if CV_MAJOR_VERSION < 4
        im = hwc_to_chw(det_im);
#else
        // opencv 4 dnn solution
        im = cv::dnn::blobFromImage(det_im, 1.0, cv::Size(), cv::Scalar(103,117,123), false, false, CV_32FC3);
#endif
    }

}


template<typename T> static Grid<T> decode(Grid<T> data, Grid<T> anchors, Config_Data cfg){
    Grid<T> result;
    try{
        Grid<T> boxes = GridFunc::concatenateGrids(anchors.getSubset(2, 0)+data.getSubset(2, 0)*cfg.variance[0]*anchors.getReverseSubset(2, 0),anchors.getReverseSubset(2, 0) * data.getReverseSubset(2,0).getExponential(cfg.variance[1]));
        // std::cout << boxes.print() << std::endl;
        Grid<T> sub_set_x = boxes.getSubset(2, 0);
        sub_set_x -= (boxes.getReverseSubset(2,0)*0.5f);
        Grid<T> sub_set_y = boxes.getReverseSubset(2, 0) + sub_set_x;
    
        result = GridFunc::concatenateGrids(sub_set_x, sub_set_y);
    } catch(const char *msg){
        std::cout << msg << std::endl;
    }

    return result;
}

template<typename T> static Grid<T> decode_landmarks(Grid<T> land, Grid<T> anchors, Config_Data cfg){
    Grid<T> result;
    try{
        result = GridFunc::concatenateNGrids(std::vector<Grid<T>>({
            anchors.getSubset(2,0) + land.getSubset(2,0) * cfg.variance[0] * anchors.getReverseSubset(2,0),
            anchors.getSubset(2,0) + land.getIntervalSubset(gridPoint(2,0),gridPoint(4,-1)) * cfg.variance[0] * anchors.getReverseSubset(2,0),
            anchors.getSubset(2,0) + land.getIntervalSubset(gridPoint(4,0),gridPoint(6,-1)) * cfg.variance[0] * anchors.getReverseSubset(2,0),
            anchors.getSubset(2,0) + land.getIntervalSubset(gridPoint(6,0),gridPoint(8,-1)) * cfg.variance[0] * anchors.getReverseSubset(2,0),
            anchors.getSubset(2,0) + land.getIntervalSubset(gridPoint(8,0),gridPoint(10,-1)) * cfg.variance[0] * anchors.getReverseSubset(2,0),
        }));
    } catch (const char *msg){
        std::cout << msg << " (decode_landmarks)" << std::endl;
    }
    return result;
}

template<typename T> static std::vector<T> getSelectedIndexes(std::vector<T> data, std::vector<int> idx){
    std::vector<T> result(idx.size());
    for (size_t i = 0; i < idx.size(); i++)
        result[i] = data[idx[i]];
    return result;
}
//Fix NMS
template<typename T> static std::vector<int> nms(Grid<T> dets, float thresh){
    std::vector<int> keep;
    std::vector<int> order(dets.rows());
    std::vector<int> inds;

    //bboxes areas = (x2 - x1 + 1) * (y2 - y1 + 1)
    Grid<T> areas = (dets.getIntervalSubset(gridPoint(2,0), gridPoint(3,-1)) - dets.getSubset(1,0) + 1) 
                    * (dets.getIntervalSubset(gridPoint(3,0),gridPoint(4,-1)) - dets.getIntervalSubset(gridPoint(1,-1),gridPoint(2,-1))+ 1);

    order = dets.argsort(4);

    while(order.size() > 0){
        int i = order[0];
        keep.push_back(i);

        //Gets intersection rect points (x1,y1) and (x2,y2)
        Grid<T> xx1 = GridFunc::maximum(dets.getSubset(1,0)(0,i), dets.getSubset(1,0)[std::vector<int>(order.begin()+1,order.end())]);
        Grid<T> yy1 = GridFunc::maximum(dets.getIntervalSubset(gridPoint(1,0),gridPoint(2,-1))(0,i), dets.getIntervalSubset(gridPoint(1,0),gridPoint(2,-1))[std::vector<int>(order.begin()+1,order.end())]);
        Grid<T> xx2 = GridFunc::minimum(dets.getIntervalSubset(gridPoint(2,0),gridPoint(3,-1))(0,i), dets.getIntervalSubset(gridPoint(2,0),gridPoint(3,-1))[std::vector<int>(order.begin()+1,order.end())]);
        Grid<T> yy2 = GridFunc::minimum(dets.getIntervalSubset(gridPoint(3,0),gridPoint(4,-1))(0,i), dets.getIntervalSubset(gridPoint(3,0),gridPoint(4,-1))[std::vector<int>(order.begin()+1,order.end())]);

        //Gets intersection area width and height (or 0 if non-overlapping)
        Grid<T> w = GridFunc::maximum(0.0f, xx2 - xx1 + 1);
        Grid<T> h = GridFunc::maximum(0.0f, yy2 - yy1 + 1);
        Grid<T> inter = w * h;

        //Calculates IoU overlaps 
        Grid<T> ovr = inter / (areas[std::vector<int>(order.begin()+1,order.end())] + areas(0,i) - inter);

        //Gets index where IoU is bellow the threshold (low intersection between boxes)
        inds = ovr.where([thresh](T value) -> bool {return value <= thresh;});
        // std::cout << inds << std::endl;
        //Updates indexes (since the first one is used as reference)
        for(auto &value : inds) value++;
        // std::cout << inds << std::endl;
        order = getSelectedIndexes(order, inds);

        // std::cout << order << std::endl;
        // std::cout << keep << std::endl;

    }

    return keep;
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
                    float s_kx = ((float) min_sz[s])/((float) image_size.width);
                    float s_ky = ((float) min_sz[s])/((float) image_size.height);
                    float dense_cx = (y + 0.5) * cfg.steps[i]/image_size.width;
                    float dense_cy = (x + 0.5) * cfg.steps[i]/image_size.height;
                    
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

    Grid<T> final_grid(4, anchors.size()/4);
    final_grid.setData(anchors.data(), anchors.size());
    anchors.clear();

    return final_grid;
}

Grid<float> outputPostProcessing(std::vector<Grid<float>>tensors, Config_Data cfg, bool debug, float det_scale, cv::Size image_size, float conf_th, int top_k, int keep_top_k){
    Grid<float> anchors = anchors_grid<float>(cfg, image_size);
    // std::cout << anchors << std::endl;

    Grid<float> boxes = decode(tensors[0], anchors, cfg);
    if(image_size.height == image_size.width)
        boxes *= image_size.height;
    else
        throw "Not Yet Implemented";

    Grid<float> scores = tensors[1].getIntervalSubset(gridPoint(1,0), gridPoint(-1,-1));

    Grid<float> landmarks = decode_landmarks(tensors[2], anchors, cfg);

    if(image_size.height == image_size.width)
        landmarks *= image_size.height;

    // std::ofstream os("landmarks.txt");
    // os << landmarks.print();
    // os.close();

    std::vector<int> inds = scores.where([conf_th](float value) -> bool { return value > conf_th; });
    if(!inds.size()) return Grid<float>(15,0);

    boxes = boxes[inds];
    scores = scores[inds];
    landmarks = landmarks[inds];

    if(debug){
        std::cout << "[RetinaFace Debug] Post Threshold Filtering" << std::endl;
        std::cout << boxes << std::endl;
        std::cout << scores << std::endl;
        std::cout << landmarks << std::endl;
    }
    //sort index by score
    inds = scores.argsort(0);
    boxes = boxes[inds];
    scores = scores[inds];
    landmarks = landmarks[inds];

    if (inds.size() > top_k){
        boxes = boxes.getSubset(0, top_k);
        scores = scores.getSubset(0, top_k);
        landmarks = landmarks.getSubset(0, top_k);
    }

    boxes = boxes / det_scale;
    landmarks = landmarks / det_scale;

    Grid<float> dets = GridFunc::concatenateGrids(boxes, scores);

    std::vector<int> keep = nms(dets, conf_th);

    dets = dets[keep];
    landmarks = landmarks[keep];

    if(debug){
        std::cout << "[RetinaFace Debug] Post NMS Filtering" << std::endl;
        std::cout << dets << std::endl;
        std::cout << landmarks << std::endl;
    }

    return GridFunc::concatenateGrids(dets, landmarks);
}


