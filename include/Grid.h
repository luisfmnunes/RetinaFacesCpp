#ifndef GRID_H
#define GRID_H

#include<vector>
#include<numeric>
#include<algorithm>

class gridPoint{
    public:
        int x;
        int y;

        gridPoint();
        gridPoint(int x, int y) : x(x), y(y){};

        // removes negatives
        void relu(){ if(x<0) x = 0; if(y<0) y =0; }
};

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

        // x = column, y = row:  Grid(col, row) 
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

        Grid<T> operator+(const T &scalar){
            Grid<T> result = createFromGrid(*this);
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    result.setPos(x,y,_data[y*c+x]+scalar); 
                }
            }

            return result;
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

        Grid<T> operator-(const T &scalar){
            Grid<T> result = createFromGrid(*this);
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    result.setPos(x,y,_data[y*c+x]-scalar); 
                }
            }

            return result;
        }

        Grid<T> operator-(Grid<T> rhs){
            Grid<T> result = createFromGrid(*this);
            if(c!=rhs.cols() || r!=rhs.rows())
                throw "Different Sized Grids";
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    result.setPos(x,y,_data[y*c+x]-rhs(x,y));
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

        Grid<T> operator/(const T &scalar){
            Grid<T> result = createFromGrid(*this);
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    result.setPos(x,y,_data[y*c+x]/scalar); 
                }
            }

            return result;
        }

        Grid<T> operator/(Grid<T> rhs){
            if(rhs.cols()!= c || rhs.rows() != r)
                throw "Different Sized Grids";
            Grid<T> result = createFromGrid(*this);
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    result.setPos(x,y,_data[y*c+x]/rhs(x,y)); 
                }
            }

            return result;
        }

        Grid<T> operator[](std::vector<int> index){
            Grid<T> result(c, index.size());
            for(int y = 0; y < index.size(); y++){
                for(int x = 0; x < c; x++){
                    result.setPos(x, y, _data[x+index[y]*c]);
                }
            }

            return result;
        }

        // grid[:xlimit, :ylimit]
        Grid<T> getSubset(uint xlimit=0, uint ylimit=0){
            if(xlimit > c || ylimit > r){
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
            for (uint y = ylimit; y < r; y++){
                for(uint x = xlimit; x < c; x++){
                    result.setPos(x-xlimit,y-ylimit,_data[(x + y*c)]);
                }
            }

            return result;
        }

        // grid[begin.x : end.x, begin.y : end.y]
        Grid<T> getIntervalSubset(gridPoint begin, gridPoint end){
            begin.relu();
            if(begin.x < 0) begin.x = 0;
            if(begin.y < 0) begin.x = 0;
            if(end.x < 0) end.x = c;
            if(end.y < 0) end.y = r;

            Grid<T> result(end.x-begin.x, end.y-begin.y);

            if(begin.x >= end.x || begin.y >= end.y)
                throw "Begin Coordinate(s) higher than end";
            if(end.x > c || end.y > r)
                throw "Out of range index in getIntervalSubset";
            
            for(uint y = begin.y; y < end.y; y++){
                for(uint x = begin.x; x < end.x; x++){
                    result.setPos(x-begin.x,y-begin.y,_data[y*c + x]);
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

        //return rows that attends to condition lambda functor
        template<typename F> std::vector<int> where(F condition_lambda, int col = -1){
            std::vector<int> index;
            for(int y = 0; y < r; y++){
                for(int x = col < 0 ? 0 : col; col < 0 ? (x < c) : (x == col); x++){
                    if(condition_lambda( _data[x+y*c])){
                        index.push_back(y);
                    }
                }
            }

            return index;
        }

        std::vector<int> argsort(int col){
            std::vector<int> order(r);
            iota(order.begin(), order.end(), 0);
            std::vector<T> col_data(r);
            for(int y = 0; y < r; y++)
                col_data[y] = _data[y*c + col];
            
            stable_sort(order.begin(), order.end(),
            [&col_data](int i1, int i2) {return col_data[i1] > col_data[i2];});

            return order;
        }

        std::string print(){
            std::stringstream ss;
            for(int y = 0; y < r; y++){
                for(int x = 0; x < c; x++){
                    ss << _data[x + y*c] << " ";
                }
                ss << std::endl;
            }

            return ss.str();
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

        void mul(Grid<T> rhs){
            if(c != rhs.cols() && rhs.rows() != 1)
                throw "Not a vector or vector columns diverge from Grid";
            for(uint y = 0; y < r; y++){
                for(uint x = 0; x < c; x++){
                    this->setPos(x,y,_data[y*c+x]*rhs(x,0));
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
        Grid<T> result(lgd.cols() + rgd.cols(),lgd.rows());
        
        for (uint y = 0; y < result.rows(); y++){
            for(uint x = 0; x < result.cols(); x++){
                T value = x < lgd.cols() ? lgd(x,y) : rgd(x-lgd.cols(),y); 
                result.setPos(x,y,value);
            }
        }

        return result;
    }

    template<typename T> inline Grid<T> concatenateNGrids(std::vector<Grid<T>> list){
        Grid<T> result(0, list.front().rows());
        for(int i = 0; i < list.size(); i++){
            try{
                result = concatenateGrids(result, list[i]);
            } catch (const char* msg){
                std::cout << msg << " -> Concatenation Failed at index " << i << std::endl;
                return Grid<T>();
            }
        }

        return result;
        
    }

    template<typename T> inline Grid<T> maximum(T value, Grid<T> rhs){
        Grid<T> result = rhs.createFromGrid(rhs);
        for(int y = 0; y < rhs.rows(); y++){
            for(int x = 0; x < rhs.cols(); x++){
                if(value > result(x,y))
                    result.setPos(x,y,value);
            }
        }
        return result;
    }

    template<typename T> inline Grid<T> minimum(T value, Grid<T> rhs){
        Grid<T> result = rhs.createFromGrid(rhs);
        for(int y = 0; y < rhs.rows(); y++){
            for(int x = 0; x < rhs.cols(); x++){
                if(value < result(x,y))
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