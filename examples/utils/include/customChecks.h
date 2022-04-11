#ifndef CUSTOM_CHECKS_H
#define CUSTOM_CHECKS_H

/** @file customChecks.h 
 * @brief The module used for custom checking types or values
 * 
 * A header containing functions used to verify certain condition or associated value.
*/

#include<iostream>
#include<cstdlib>
#include<cstring>
#include <cstdint>
#include<sys/stat.h>
#include<string>
#include<sstream>
#include<type_traits>

///Check if given input is a file
/**
 * Receives a filename and checks if it exists
 * 
 * @param filename [in] A string containing a filepath/filename.
 * 
 * @return True or False
 */
bool is_file(std::string filename);
///Check if given input is a file
/**
 * Receives a filename and checks if it exists
 * 
 * @param filename [in] A char buffer array containing a filepath/filename.
 * 
 * @return True or False
 */
bool is_file(const char *filename);
///Check if given input is a directory
/**
 * Receives a directory path and checks if it exists
 * 
 * @param dir [in] A string containing a directory path.
 * 
 * @return True or False
 */
bool is_dir(std::string dir);
///Check if given input is a directory
/**
 * Receives a directory path and checks if it exists
 * 
 * @param dir [in] A char buffer array containing a directory path.
 * 
 * @return True or False
 */
bool is_dir(const char *dir);

///Check if given input is a templated numeric
/**
 * Calls a numeric templated function and receives a string to check if it can be converted
 * 
 * @tparam Numeric type (*e.g.* int, double)
 * @param arg [in] A string to check if it matches the numeric template standard (useful checking prior to conversion from string to numeric)
 * 
 * @return True or False
 * 
 * @code
 *  #include "customChecks.h" 
 * 
 *  int main(){
 *  
 *      string s = "130.4";
 *      is_numeric<int>(s); //False
 *      is_numeric<double>(s); //True
 * 
 *      return 0;
 *  }
 * 
 * @endcode
 */ 
template<typename Numeric> bool is_numeric(std::string arg){
    Numeric n;
    return((std::istringstream(arg) >> n >> std::ws).eof());
};
///Check if given input is a templated numeric
/**
 * Calls a numeric templated function and receives a string to check if it can be converted
 * 
 * @tparam Numeric type (*e.g.* int, double)
 * @param arg [in] A char buffer array to check if it matches the numeric template standard (useful checking prior to conversion from string to numeric)
 * 
 * @return True or False
 */ 
template<typename Numeric> bool is_numeric(const char *arg){
    std::string s_arg(arg);
    return(is_numeric<Numeric>(s_arg));
};
///Check if type of given input is a numeric container (arithmetic)
/**
 * Calls a templated function and receives a variable to check if its type is arithmetic
 * 
 * @tparam Type
 * @param var [in] Any Variable (primitive types or not)
 * 
 * @return True or False
 */ 
template<typename T> bool is_type_numeric(T var){
    return std::is_arithmetic<T>::value;
}
///Return the size of a file
/**
 * Receives a filename/filepath
 * 
 * @param file [in] A char buffer array containing a filename/filepath
 */ 
int64_t file_size(const char* file);
///Return the size of a file
/**
 * Receives a filename/filepath
 * 
 * @param file [in] A string containing a filename/filepath
 */ 
int64_t file_size(std::string file);


#endif