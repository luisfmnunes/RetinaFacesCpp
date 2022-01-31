#ifndef GRID_H
#define GRID_H

#include<vector>

template<typename T>
class Grid{
    private:
        uint c;
        uint r;
        std::vector<T> _data;
    public:
        Grid(uint cols, uint rows) : c(cols), r(rows){
            _data.resize(c*r);
        }

        inline void setData(const T* data, const size_t data_size){
            memcpy(&_data[0], data, data_size * sizeof(T));
        }

        T& operator()(uint x, uint y){
            if (x >= c || y >= r)
                throw "Out of range index";
            return _data[y*c + x];
        }

        uint cols() const{
            return c;
        }

        uint rows() const{
            return r;
        }

        size_t total_elements() const{
            return c * r;
        }
};

template<typename T> inline std::ostream& operator<<(std::ostream& os, const Grid<T> &grid){
    os << 
    "{Grid: {cols: " << grid.cols() << 
    ", rows: " << grid.rows() << 
    ", total: " << grid.total_elements() <<
    "}}";

    return os;
}

#endif