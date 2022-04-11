#include"../include/customLog.h"

#ifndef _WIN32
uint64_t pid = getpid();
#else
uint64_t pid = _getpid();
#endif


void _log(const char *format, ...){
#ifndef _WIN32
    fprintf(stderr,"(%lu)[\033[0;36mLOG\033[0m]: ",pid);
#else
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	fprintf(stderr, "(%lu)[", pid);
	SetConsoleTextAttribute(hConsole, 3); 
	fprintf(stderr, "LOG"); 
	SetConsoleTextAttribute(hConsole, 7); 
	fprintf(stderr, "]:");
#endif
	va_list argptr;
	va_start(argptr, format);
	vfprintf(stderr, format, argptr);
	va_end(argptr);
	fprintf(stderr, "\n");
}

void _warn(const char *format, ...){
#ifndef _WIN32
    fprintf(stderr,"(%lu)[\033[0;33mWARNING\033[0m]: ",pid);
#else
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	fprintf(stderr, "(%lu)[", pid);
	SetConsoleTextAttribute(hConsole, 14);
	fprintf(stderr, "WARNING");
	SetConsoleTextAttribute(hConsole, 7);
	fprintf(stderr, "]:");
#endif
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    fprintf(stderr, "\n");

}

void _error(const char *format, ...){
#ifndef _WIN32
    fprintf(stderr,"(%lu)[\033[0;31mERROR\033[0m]: ",pid);
#else
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	fprintf(stderr, "(%lu)[", pid);
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
	fprintf(stderr, "ERROR");
	SetConsoleTextAttribute(hConsole, 7);
	fprintf(stderr, "]:");
#endif
	va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    fprintf(stderr, "\n");
}

void _result(const char *format, ...){
#ifndef _WIN32
    fprintf(stderr,"(%lu)[\033[0;34mRESULT\033[0m]: ",pid);
#else
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	fprintf(stderr, "(%lu)[", pid);
	SetConsoleTextAttribute(hConsole, 1);
	fprintf(stderr, "RESULT");
	SetConsoleTextAttribute(hConsole, 7);
	fprintf(stderr, "]:");
#endif
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    fprintf(stderr,"\n");
}

void _debug(const char *format, ...){
#ifndef _WIN32
    fprintf(stderr,"(%lu)[\033[0;35mDEBUG\033[0m]: ", pid);
#else
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	fprintf(stderr, "(%lu)[", pid);
	SetConsoleTextAttribute(hConsole, 5);
	fprintf(stderr, "DEBUG");
	SetConsoleTextAttribute(hConsole, 7);
	fprintf(stderr, "]:");
#endif
	va_list argptr;
    va_start(argptr,format);
    vfprintf(stderr,format,argptr);
    va_end(argptr);
    fprintf(stderr,"\n");
}

