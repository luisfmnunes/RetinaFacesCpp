#ifndef CUSTOM_LOG_H
#define CUSTOM_LOG_H

/**
 * @mainpage Index
 * 
 * This is a C++ library containing utility functionalities, from log printing, to string operations.
 * 
 * @section Headers
 * + @ref customLog.h - Operations related to printing log output
 * + @ref customChecks.h - Operations related to status verification or value verification
 * + @ref customStringOps.h - Operations related to string manipulation
 * + @ref customFileOps.h - Operations related to files and dirs (*e.g.* list files from dir)
 * + @ref customTimeOps.h - Operations related to execution time
 * + @ref customParser.h - A Class header to a command line arguments Parser
 * + @ref customEER.h - Operations related to EER analysis
 */ 

/** @file customLog.h 
 *  @brief The module responsible for log output
 * 
 * A header containing functions used to print information in the standard output.
*/

#include<iostream>
#include<cstdlib>
#include<cstdio>
#include<cstdarg>
#include <cstdint>

#ifdef _WIN32
#include<process.h>
#include<Windows.h>
#ifdef _MSC_VER
#include<direct.h>
#endif
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#define C_RED "\033[0;31m" ///< Macro to set red color on streams
#define C_GREEN "\033[0;32m" ///< Macro to set green color on streams
#define C_YELLOW "\033[0;33m" ///< Macro to set yellow color on streams
#define C_BLUE "\033[0;34m" ///< Macro to set blue color on streams
#define C_MAGENTA "\033[0;35m" ///< Macro to set magenta color on streams
#define C_CYAN "\033[0;36m" ///< Macro to set cyan color on streams
#define C_WHITE "\033[0;37m" ///< Macro to set white color on streams
#define C_RESET "\033[0m" ///< Macro to reset color back to normal

//PRINTF Like

/// Prints a log message in the standard output
/**
 * Receives a format string, and an array of corresponding arguments (like printf)
 * @param format [in] A const char array representing the message (*e.g.* "file: %s")
 * @param ... [in] respective arguments of the message
 * 
 * @code
 * #include "customLog.h"
 * 
 * int main(int argc, char** argv){
 * 
 *      if(argc > 1){
 * 
 *          for(int i = 0; i < argc; i++) _log("Argument %d = %s",i,argv[i]); //Prints a log list of arguments passed in line command
 *      
 *      }
 * 
 *      return 0;
 * }
 * @endcode
*/
void _log(const char *format, ...);
/// Prints a warning message in the standard output
/**
 * Receives a format string, and an array of corresponding arguments (like printf)
 * @param format [in] A const char array representing the message (*e.g.* "file: %s")
 * @param ... [in] respective arguments of the message
 * 
 * @code
 * #include "customLog.h"
 * #include "customChecks.h"
 * 
 * int main(){
 * 
 *      string num = "154a"; //String to check if the content is numeric.
 *      if(!is_numeric<int>(num)) _warn("String %s is not a number.",num.c_str()); //Prints Warning if the string num doesn't represent an Integer.
 * 
 *      return 0;
 * }
 * @endcode
*/
void _warn(const char *format, ...);
/// Prints an error message in the standard output
/**
 * Receives a format string, and an array of corresponding arguments (like printf)
 * @param format [in] A const char array representing the message (*e.g.* "file: %s")
 * @param ... [in] respective arguments of the message
 * 
 * @code
 * #include "customLog.h"
 * 
 * //...
 * 
 * bool corruptedTemplate (Template tpt){ ... }
 * 
 * int main(){
 *      //...
 * 
 *      if(corruptedTemplate){
 * 
 *          _error("Corrupted Template File %s. Exitting application.", tpt.name);
 *          return EXIT_FAILURE; 
 * 
 *      }
 * 
 *      //...
 * 
 * }
 * 
 * @endcode
*/
void _error(const char *format, ...);
/// Prints a result message in the standard output
/**
 * Receives a format string, and an array of corresponding arguments (like printf)
 * @param format [in] A const char array representing the message (*e.g.* "file: %s")
 * @param ... [in] respective arguments of the message
 * 
 * @code
 * 
 * #include "customLog.h"
 * 
 * double computeScore(Template tpt1, Template tpt2) { ... }
 * 
 * ...
 * 
 *  if(PRINT_RESULT){
 *       _result("Resultant Score: %.2f",computeScore(tpt1,tpt2)); 
 *  }
 * 
 * ...
 * 
 * @endcode
*/
void _result(const char *format, ...);
/// Prints a debug message in the standard output
/**
 * Receives a format string, and an array of corresponding arguments (like printf)
 * @param format [in] A const char array representing the message (*e.g.* "file: %s")
 * @param ... [in] respective arguments of the message
 * 
 * @code
 * 
 *  cv::Mat* pre_processing(cv::Mat* input_ptr, int Operation){ ... }
 * 
 *  int main(int argc, char** argv){
 *  //...
 *
 *      processed_image = pre_processing(original_image);
 * 
 *      //Checking if proper behavior is happening * 
 *      if(DEBUG_FLAG){
 *        processed_image == NULL ? _debug("Processed Image is Null") : _debug("Processed Image has size: (%d,%d)",processed_image.cols,processed_image.rows); 
 *      }
 *  
 *  //...
 * 
 *  }
 * @endcode
*/
void _debug (const char *format, ...);

//STREAM-BASED

static inline void custom_print(){
    std::cout << std::endl;
}

template<typename First, typename... Args> static void custom_print(const First& first, const Args&... args){
    std::cout << first << " ";
        custom_print(args...);
}

/// Prints a log message in the standard output (Stream version)
/**
 * Receives any arguments and print if the object has the ostream << operator implemented
 * @param ...args [in] respective arguments of the message
 * 
 * @code
 * #include "path_to: customLog.h"
 * 
 * int main(int argc, char** argv){
 * 
 *      if(argc > 1){
 * 
 *          for(int i = 0; i < argc; i++) os_log("Argument",i," = ",argv[i]); //Prints a log list of arguments passed in line command
 *      
 *      }
 * 
 *      return 0;
 * }
 * @endcode
*/
template<typename... Args>void os_log(Args ...args){
#ifndef _WIN32
	std::cout << "(" << getpid() << ")[" << C_CYAN << "LOG" << C_RESET << "]: "; 
#else
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	std::cout << "(" << _getpid() << ")[";
	SetConsoleTextAttribute(hConsole, 3);
	std::cout << "LOG";
	SetConsoleTextAttribute(hConsole, 7);
	std::cout << "]: ";
#endif
	
	custom_print(args...);
}
/// Prints a warning message in the standard output (Stream version)
/**
 * Receives any arguments and print if the object has the ostream << operator implemented
 * @param ...args [in] respective arguments of the message
 * 
 * @code
 * #include "customLog.h"
 * #include "customChecks.h"
 * 
 * int main(){
 * 
 *      string num = "154a"; //String to check if the content is numeric.
 *      if(!is_numeric<int>(num)) os_warn("String ", num.c_str()," is not a number.",); //Prints Warning if the string num doesn't represent an Integer.
 * 
 *      return 0;
 * }
 * @endcode
*/
template<typename... Args>void os_warn(Args ...args){
#ifndef _WIN32
	std::cout << "(" << getpid() << ")[" << C_YELLOW << "WARNING" << C_RESET << "]: "; 
#else
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	std::cout << "(" << _getpid() << ")[";
	SetConsoleTextAttribute(hConsole, 14);
	std::cout << "WARNING";
	SetConsoleTextAttribute(hConsole, 7);
	std::cout << "]: ";
#endif
	custom_print(args...);
}
/// Prints an error message in the standard output (Stream version)
/**
 * Receives any arguments and print if the object has the ostream << operator implemented
 * @param ...args [in] respective arguments of the message
 * 
 * @code
 * #include "customLog.h"
 * 
 * //...
 * 
 * bool corruptedTemplate (Template tpt){ ... }
 * 
 * int main(){
 *      //...
 * 
 *      if(corruptedTemplate){
 * 
 *          os_error("Corrupted Template File ", tpt.name, ". Exitting application.");
 *          return EXIT_FAILURE; 
 * 
 *      }
 * 
 *      //...
 * 
 * }
 * 
 * @endcode
*/
template<typename... Args>void os_error(Args ...args){
#ifndef _WIN32
	std::cout << "(" << getpid() << ")[" << C_RED << "ERROR" << C_RESET << "]: "; 
#else
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	std::cout << "(" << _getpid() << ")[";
	SetConsoleTextAttribute(hConsole, 4);
	std::cout << "ERROR";
	SetConsoleTextAttribute(hConsole, 7);
	std::cout << "]: ";
#endif
	custom_print(args...);
}
/// Prints a result message in the standard output (Stream version)
/**
 * Receives any arguments and print if the object has the ostream << operator implemented
 * @param ...args [in] respective arguments of the message
 * 
 * @code
 * 
 * #include "customLog.h"
 * 
 * double computeScore(Template tpt1, Template tpt2) { ... }
 * 
 * ...
 * 
 *  if(PRINT_RESULT){
 *       os_result("Resultant Score: ", computeScore(tpt1,tpt2)); 
 *  }
 * 
 * ...
 * 
 * @endcode
*/
template<typename... Args>void os_result(Args ...args){
#ifndef _WIN32
	std::cout << "(" << getpid() << ")[" << C_BLUE << "RESULT" << C_RESET << "]: "; 
#else
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	std::cout << "(" << _getpid() << ")[";
	SetConsoleTextAttribute(hConsole, 1);
	std::cout << "RESULT";
	SetConsoleTextAttribute(hConsole, 7);
	std::cout << "]: ";
#endif
	custom_print(args...);
}
/// Prints a debug message in the standard output (Stream version)
/**
 * Receives any arguments and print if the object has the ostream << operator implemented
 * @param ...args [in] respective arguments of the message
 * 
 * @code
 * 
 *  cv::Mat* pre_processing(cv::Mat* input_ptr, int Operation){ ... }
 * 
 *  int main(int argc, char** argv){
 *  //...
 *
 *      processed_image = pre_processing(original_image);
 * 
 *      //Checking if proper behavior is happening * 
 *      if(DEBUG_FLAG){
 *        processed_image == NULL ? os_debug("Processed Image is Null") : os_debug("Processed Image has size: (",processed_image.cols,",",processed_image.rows,")"); 
 *      }
 *  
 *  //...
 * 
 *  }
 * @endcode
*/
template<typename... Args>void os_debug(Args ...args){
#ifndef _WIN32
	std::cout << "(" << getpid() << ")[" << C_MAGENTA << "DEBUG" << C_RESET << "]: "; 
#else
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	std::cout << "(" << _getpid() << ")[";
	SetConsoleTextAttribute(hConsole, 5);
	std::cout << "DEBUG";
	SetConsoleTextAttribute(hConsole, 7);
	std::cout << "]: ";
#endif
	custom_print(args...);
}

#endif