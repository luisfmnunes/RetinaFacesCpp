#include"../include/customStringOps.h"

std::string get_filename(std::string path){
#ifndef _WIN32
    std::string::size_type pos = path.find_last_of('/');
#else
	std::string::size_type pos = path.find_last_of('\\');
#endif
    if(pos==std::string::npos) return path;
    return path.substr(pos+1);
}

std::string get_filedir(std::string path){
#ifndef _WIN32
	std::string::size_type pos = path.find_last_of('/');
#else
	std::string::size_type pos = path.find_last_of('\\');
#endif
    if(pos==std::string::npos) return "";
    return path.substr(0,pos+1);
}

std::vector<std::string> str_split(std::string input, std::string delimiter){
    std::vector<std::string> ss(0);
    std::string::size_type pos;
     while((pos=input.find_first_of(delimiter))!= std::string::npos){
        ss.push_back(input.substr(0,pos));
        input.erase(0, pos + delimiter.length());
     }
     ss.push_back(input);
     return ss;
}

std::vector<std::string> str_split_or_empty(std::string input, std::string delimiter){
    std::vector<std::string> ss(0);
    std::string::size_type pos;
    bool splited = false;
    while((pos=input.find_first_of(delimiter))!= std::string::npos){
        ss.push_back(input.substr(0,pos));
        input.erase(0, pos + delimiter.length());
        splited = true;
    }
    if(splited)
        ss.push_back(input);
    return ss;
}

std::string str_trim(std::string input){
    input.erase(input.find_last_not_of(" \n\r\t")+1);
    input = (input.find(" \n\r\t")!=std::string::npos) ? input.substr(input.find_first_of(" \n\r\t")-1) : input;
    return input;
}

bool has_extension(std::string path){
    std::string::size_type pos = path.find_last_of('.');
    return pos != std::string::npos && ( 5 >= path.size() - pos);
}

bool in_quotes(std::string text){
    std::string::size_type pos1 = text.find_first_of('"');
    std::string::size_type pos2 = text.find_last_of('"');
    
    std::string::size_type pos3 = text.find_first_of("'");
    std::string::size_type pos4 = text.find_last_of("'");
    return (!(pos1 == pos2) || !(pos3 == pos4));
}

std::string remove_quotes(std::string input){
    std::string::size_type pos1 = input.find_first_of('"');
    std::string::size_type pos2 = input.find_last_of('"');
    
    std::string::size_type pos3 = input.find_first_of("'");
    std::string::size_type pos4 = input.find_last_of("'");

    if(pos1 != std::string::npos && pos2 != std::string::npos){
        return input.substr(pos1+1,pos2-pos1-1);
    }

    if(pos3 != std::string::npos && pos4 != std::string::npos){
        return input.substr(pos3+1,pos4-pos3-1);
    }
    return input;

}

std::string remove_extension(std::string input){
    std::string::size_type pos = input.find_last_of('.');
    return pos==std::string::npos ? input : input.substr(0,pos);
}

static char ascii_tolower(char in){
	if (in <= 'Z' && in >= 'A')
		return in - ('A' - 'a');
	return in;
}

static char ascii_toupper(char in){
	if (in <= 'z' && in >= 'a')
		return in + ('A' - 'a');
	return in;
}

std::string str_lower(std::string input){
	for (auto &ch : input)
		ch = ascii_tolower(ch);
	return input;
}
std::string str_upper(std::string input){
	for (auto &ch : input)
		ch = ascii_tolower(ch);
	return input;
}