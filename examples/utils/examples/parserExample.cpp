#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>

#include "../customUtils.h"

using namespace std;

int main (int argc, char **argv){
    GParser* parser = new GParser("A simple parser Example",true);

    string filename = "ab";
    int val = 10;
    bool is = false;
    string directives = "--file,-f";
    for(auto i : str_split(directives,","))
        _debug("Split: %s",i.c_str());

    _result("Filename before setting: %s", filename.c_str());
    // parser->add_option("--file,-f","Hello World",filename);
    // parser->add_flag("-b","Flag Test",is);
    // parser->parseValues(argc,argv);

    GPARSER_ADD_OPTION((*parser),"--file,-f","Filename",filename);
    GPARSER_ADD_FLAG((*parser),"-b","Flag Test",is);
    GPARSER_ADD_HELP((*parser),"-h,--help");
    GPARSER_PARSE((*parser),argc,argv);

    _result("Filename after setting: %s", filename.c_str());
    _result("Sort is set: %s",(is ? "True" : "False"));

    return EXIT_SUCCESS;
}