#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<string>
#include<vector>

#include"../customUtils.h"

using namespace std;

int main(int argc, char** argv){

    GParser parser("An algorithm to list files from a subdir and sort them.", true);
    
    string dir, filter;
    bool sorted, flag, result;
    vector<string> ftw_files;
    vector<string> dirent_files;
    
    GPARSER_ADD_OPTION(parser,"-i,--input","An input directory to list files",dir);
    GPARSER_ADD_OPTION(parser,"-f,--filter","An expression to filter files", filter);
    GPARSER_ADD_HELP(parser,"-h,--help");
    GPARSER_ADD_FLAG(parser,"-s,--sort","A flag indicating if the vector should be sorted",sorted);
    GPARSER_ADD_FLAG(parser,"--flag","A random flag",flag);
    GPARSER_ADD_FLAG(parser,"-R","Flag to print Results", result);
    GPARSER_PARSE(parser,argc,argv);

#ifndef _WIN32
    TICK(FTW);
    if(!sorted)
        ftw_files = list_files_from_ftw(dir, filter);
    else
        ftw_files = list_files_from_ftw_sorted(dir, filter);
    TOCK(FTW);
#endif

     // TOCK_MIN(List_Files);
    // TOCK_H(List_Files);

    TICK(DIRENT);
    if(!sorted)
        dirent_files = list_files_from_directory(dir,filter);
    else
        dirent_files = list_files_from_directory_sorted(dir,filter);
    TOCK(DIRENT);
    // TOCK_MIN(DIRENT);

    if(result){
        for(auto file : dirent_files) {
            _result("%s",file.c_str());
        }
    }


    os_result("FTW Size = ", ftw_files.size()," DIRENT Size = ",dirent_files.size());

    return EXIT_SUCCESS;
}