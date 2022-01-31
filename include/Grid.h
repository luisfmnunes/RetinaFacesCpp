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
        Grid() : c(0), r(0) {}

        Grid(uint cols, uint rows) : c(cols), r(rows){
            _data.resize(c*r);
        }
        Grid<T> createFromGrid(Grid<T> &rhs){
            Grid<T> result(rhs.cols(), rhs.rows());
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    result.setPos(x,y,rhs(x,y));
                }
            }
            
            return result;
        }

        inline void setData(const T* data, const size_t data_size){
            memcpy(&_data[0], data, data_size * sizeof(T));
        }

        T& operator()(uint x, uint y){
            if (x >= c || y >= r){
                std::cout << "x = " << x << " c = " << c << " y = " << y << " r = " << r << std::endl;
                throw "Out of range index in operator ()";
            }
            return _data[y*c + x];
        }

        Grid<T>& operator+=(Grid<T> rhs){
            if(c!=rhs.cols() || r!=rhs.rows())
                throw "Different Sized Grid";
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    _data[y*c + x] += rhs(x,y);
                }
            }

            return *this;
        }

        Grid<T>& operator-=(Grid<T> rhs){
            if(c!=rhs.cols() || r!=rhs.rows())
                throw "Different Sized Grid";
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    _data[y*c + x] -= rhs(x,y);
                }
            }

            return *this;
        }

        Grid<T>& operator*=(const T &scalar){
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    _data[y*c + x] *= scalar; 
                }
            }

            return *this;
        }

        Grid<T> operator+(Grid<T> rhs){
            Grid<T> result = createFromGrid(*this);
            if(c!=rhs.cols() || r!=rhs.rows())
                throw "Different Sized Grids";
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    result.setPos(x,y,_data[y*c+x]+rhs(x,y));
                }
            }

            return result;
        }

        Grid<T> operator*(const T &scalar){
            Grid<T> result = createFromGrid(*this);
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    result.setPos(x,y,_data[y*c+x]*scalar); 
                }
            }

            return result;
        }

        Grid<T> operator*(Grid<T> rhs){
            if(rhs.cols()!= c || rhs.rows() != r)
                throw "Different Sized Grids";
            Grid<T> result = createFromGrid(*this);
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    result.setPos(x,y,_data[y*c+x]*rhs(x,y)); 
                }
            }

            return result;
        }

        // grid[:xlimit, :ylimit]
        Grid<T> getSubset(uint xlimit=0, uint ylimit=0){
            if(xlimit >= c || ylimit >= r){
                std::cout << "xlimit: " << xlimit <<  " >= c: " << c << " ou ylimit: " << ylimit << " >= r: " << r << std::endl;
                throw "Out of range index in getSubset";
            }
            Grid<T> result(xlimit ? xlimit : c, ylimit ? ylimit : r);
            for(uint y = 0; y < result.rows(); y++){
                for(uint x = 0; x < result.cols(); x++){
                    result.setPos(x,y,_data[y*c+x]);
                }
            }

            return result;
        }

        // grid[xlimit:, ylimit:]
        Grid<T> getReverseSubset(uint xlimit = 0, uint ylimit= 0){
            if(xlimit >= c || ylimit >= r){
                std::cout << "xlimit: " << xlimit <<  " >= c: " << c << " ou ylimit: " << ylimit << " >= r: " << r << std::endl;  
                throw "Out of range index in getReverseSubset";
            }
            Grid<T> result(xlimit ? xlimit : c, ylimit ? ylimit : r);
            for (uint y = ylimit; y < result.rows(); y++){
                for(uint x = xlimit; x < result.cols(); x++){
                    result.setPos(x-xlimit,y-ylimit,_data[(x + y*c)]);
                }
            }

            return result;
        }

        Grid<T> getExponential(T scale = 1){
            Grid<T> result = createFromGrid(*this);
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    result.setPos(x,y,std::exp(_data[y*c+x]*scale));
                }
            }

            return result;
        }

        void sumUntil(Grid<T> &rhs, uint limit){
            if(limit > rhs.rows() || limit > r)
                throw "Limit passed greater than y dimension";
            for(uint y = 0; y < limit; y++){
                for(uint x = 0; x < c; x++){
                    this->setPos(x,y,_data[y*c+x]+rhs(x,y));
                }
            }
        }

        void setPos(uint x, uint y, T value){
            if(x+y*c > _data.size())
                throw "Out of range index in setPos";
            _data[y*c+x] = value;
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

        T max(){
            return *std::max_element(_data.begin(), _data.end());
        }
};

namespace GridFunc{
    template<typename T> inline Grid<T> concatenateGrids(Grid<T> lgd, Grid<T> rgd){
        if(lgd.rows() != rgd.rows())
            throw "Cannot Concatenate Grids of different height";
        Grid<T> result(lgd.cols(),lgd.rows()+rgd.rows());
        
        for (uint y = 0; y < result.rows(); y++){
            for(uint x = 0; x < result.cols(); x++){
                T value = y < lgd.rows() ? lgd(x,y) : rgd(x,y-lgd.rows()); 
                result.setPos(x,y,value);
            }
        }

        return result;
    }
}

template<typename T> inline std::ostream& operator<<(std::ostream& os, const Grid<T> &grid){
    os << 
    "{Grid: {cols: " << grid.cols() << 
    ", rows: " << grid.rows() << 
    ", total: " << grid.total_elements() <<
    "}}";

    return os;
}

#endif