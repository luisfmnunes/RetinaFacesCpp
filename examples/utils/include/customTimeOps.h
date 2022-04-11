#ifndef CUSTOM_TIME_OPS_H
#define CUSTOM_TIME_OPS_H 

/** @file customTimeOps.h 
 *  @brief The module responsible for time related operations
 * 
 * A header containing Macros used to print information about execution time. The Macros are:
 * 
 * - TICK(X): Creates a variable X and stores the current time. **Caution**: Variable X can't exist
 * - TOCK(X): Retrieves the time elapsed since TICK(X) and prints the execution time in seconds to microseconds
 * - TOCK_MIN(X): Same as TOCK but prints execution time in minutes and seconds
 * - TOCK_H(X): Same as TOCK but prints execution time in hours, minutes and seconds
*/

#include<chrono>

#define TICK(X) auto X = std::chrono::high_resolution_clock::now(); ///< Creates a variable X and stores the current time
#define TOCK(X) _log("Execution time of %s: %.6f seconds.",(#X), static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - X).count())/1000000.0); ///< Retrieves the time elapsed since TICK(X) and prints the execution time in seconds to microseconds
#define TOCK_MIN(X) {\
			custom_time this_time_min_##X = get_execution_time(X); \
            _log("Execution time of %s: %lld minutes and %d seconds.",(#X),this_time_min_##X.minutes,this_time_min_##X.seconds);\
		} ///< Same as TOCK but prints execution time in minutes and seconds
#define TOCK_H(X){ \
            custom_time this_time_h_##X = get_execution_time(X); \
            _log("Execution time of %s: %d hours, %d minutes and %d seconds.",(#X),this_time_h_##X.hours,this_time_h_##X.minutes,this_time_h_##X.seconds); \
        }///< Same as TOCK but prints execution time in hours, minutes and seconds
#endif

struct custom_time{
    int hours;
    int minutes;
    int seconds;
    int microseconds;
    custom_time(int h, int min, int s, int usec) : hours(h), minutes(min), seconds(s), microseconds(usec) {}
};
#ifndef _WIN32
inline custom_time get_execution_time(std::chrono::_V2::system_clock::time_point X){
#else
inline custom_time get_execution_time(std::chrono::system_clock::time_point X){
#endif
    auto utime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - X).count();
    long long int time = static_cast<long long int>(utime)/1000000;
    return custom_time(time/3600,(time%3600)/60,(time%3600)%60,utime%1000000);
};