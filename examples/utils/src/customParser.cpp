#include "../include/customParser.h"

using namespace std;

GParser::GParser() : debug(false){
    _log("Initializing Parser");
}

GParser::GParser(string description) : debug(false){
    _log("Initializing Parser and Setting Description");
    this->description = description;
}

GParser::GParser(bool debug){
    _log("Initializing Parser ", debug ? "in Debug mode." : "");
    this->debug = debug;
}

GParser::GParser(string description, bool debug) {
    _log("Initializing Parser and Setting Description %s", debug ? "in Debug mode." : "");
    this->description = description;
    this->debug = debug;
}

GParser::GParser(std::string description, std::string usage) : debug(false) {
    _log("Initializing Parser and Setting Description and Usage");
    this->description = description;
    this->usage = usage;
}

GParser::GParser(std::string description, std::string usage, bool debug){
    _log("Initializing Parser and Setting Description and Usage %s", debug ? "in Debug mode." : "");
    this->description = description;
    this->usage = usage;
    this->debug = debug;
}
GParser::GPErrors GParser::set_debug(bool value){
    debug = value;
    return GPErrors::PERROR_NONE;
}

GParser::GPErrors GParser::add_flag(string option_name, string help_str, bool &var){
    return add_directive(option_name,help_str,var,false);
}


GParser::GPErrors GParser::add_var_name(string option_name, string var_name){
    vector<string> directives = str_split(option_name,",");
    for(auto directive : directives){
        directive = str_trim(directive);
        if(debug) _debug("Binding directive '%s' to variable: %s.",directive.c_str(),var_name.c_str());
        while(directive.find('-')!=string::npos)
            directive.erase(directive.find('-'),1);
        
        var_names[directive] = var_name;
    }

    return GPErrors::PERROR_NONE;

}

GParser::GPErrors GParser::add_help(string option_name){
    vector<string> directives = str_split(option_name, ",");
    for(auto directive : directives){
        directive = str_trim(directive);
        if(debug) _debug("Binding directive %s to show help", directive.c_str());
        while(directive.find('-')!=string::npos){
            directive.erase(directive.find('-'),1);
        }
        
        references[directive] = [this](string nothing){
            this->print_help();
        };

        required[directive] = false;
        
    }
    help[option_name] = option_name+": Prints Help";

    return GPErrors::PERROR_NONE;
}

void GParser::print_help(){
    //(void)!system("clear");
	cout << endl << endl;
	if(!description.empty())
        cout << "DESCRIPTION" << endl << endl << '\t' << description << endl << endl;
    for(auto directive : help)
        cout << "\t\t" << directive.second << endl << endl;
    if(!usage.empty())
        cout << "USAGE" << endl << endl << '\t' << usage << endl << endl;

    exit(EXIT_SUCCESS);
}

GParser::GPErrors GParser::parseValues(int argc, char** argv){
    if (argc < 2){
        _warn("Parser called but not enough arguments");
        return GPErrors::PERROR_INSUFICIENT_ARGUMENTS;
    }
    queue<string> directives;
    queue<string> arguments;

    //Parse arguments from command line to queues
    for(int i = 0; i < argc; i++){
        if(!strncmp("--",argv[i],2)){
            string filter(argv[i]);
            while(filter.find('-')!=string::npos)
                filter.erase(filter.find('-'),1);
            if(references.find(filter)!=references.end()){
                if(i+1>=argc && (required.find(filter)!=required.end() ? required[filter] : 0)){
                    _warn("Directive %s called but argument is not given.",argv[i]);
                    continue;
                }
                directives.emplace(filter);
                if(required[filter]){
                    if(strncmp("-",argv[i+1],1)){
                        in_quotes(argv[i+1]) ? arguments.emplace(remove_quotes(argv[++i])) : arguments.emplace(argv[++i]);
                    }
                    else
                        _error("Forbiden initialization of argument %s, '-' initializer is exclusive for directives (value: %s)", argv[i],argv[i+1]);
                    continue;
                }
                else {
                    arguments.emplace("1");
                }
            }
            else _warn("Directive (option or flag) %s not set to parser.",argv[i]);
        }

        else if(!strncmp("-",argv[i],1)){
            // os_debug(i);
            int count = 0;
            for(size_t c = 1; c < strlen(argv[i]); c++){
                string directive(1,argv[i][c]);
                if(references.find(directive)!=references.end() && required.find(directive)!=required.end()){
                    if(required.find(directive)->second){
                        if(i+1+count >= argc)
                            _warn("Directive %s called but argument is not given", directive.c_str());
                        else if(!strncmp("-",argv[i+1+count],1))
                            _error("Forbiden initialization of argument -%s, '-' initializer is exclusive for directives (value: %s)", directive.c_str() ,argv[i+1+count]);
                        else{
                            directives.emplace(directive);
                            in_quotes(argv[i+1+count]) ? arguments.emplace(remove_quotes(argv[i+1+count++])) : arguments.emplace(argv[i+1+count++]);
                        }
                    }
                    else {
                        directives.emplace(directive);
                        arguments.emplace("1");
                    }
                }
                else _warn("Directive (option or flag) -%s not set to parser.",directive.c_str());
            }
            i+=count;
        }
    }

    //Update variables passed to parser
    if(directives.size()!=arguments.size()){
        _error("Something went wrong during parsing, Directives: %d Arguments (Non-Required is also filled with auto true argument): %d",directives.size(),arguments.size());
        return GPErrors::PERROR_INVALID_DATA;
    }

    while(!directives.empty() || !arguments.empty()){
        string directive = directives.front();
        string argument = arguments.front();

        if(references.find(directive)!=references.end()){
            references[directive](argument);
            if(var_names.find(directive)!=var_names.end())
                if(debug) _debug("Parsing variable '%s' with value=%s",var_names[directive].c_str(), (required[directive] ? argument.c_str() : "true"));
        }

        directives.pop();
        arguments.pop();
    }

    return GPErrors::PERROR_NONE;

}

GParser::~GParser(){
    _log("Releasing parser variables from memory");
    references.clear();
    required.clear();
    var_names.clear();
    help.clear();
}