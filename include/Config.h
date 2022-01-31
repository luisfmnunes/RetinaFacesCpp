#ifndef CONFIG_H
#define CONFIG_H

#include<iostream>
#include<vector>
#include<map>

class Config_Data{
    public:
        std::string name;
        std::vector<std::vector<int>> min_sizes;
        std::vector<int> steps;
        std::vector<float> variance;
        bool clip;        

        inline Config_Data(const std::string name, std::vector<std::vector<int>> min_sizes, std::vector<int> steps, std::vector<float> variance, bool clip) : name(name), min_sizes(min_sizes), steps(steps), variance(variance), clip(clip) {};
};

inline Config_Data get_R50_config(){
    return Config_Data("Resnet50", std::vector<std::vector<int>>({std::vector<int>({16,32}),std::vector<int>({64,128}),std::vector<int>({256,512})}),std::vector<int>({8,16,32}),std::vector<float>({0.1f,0.2f}),false);
}

#endif