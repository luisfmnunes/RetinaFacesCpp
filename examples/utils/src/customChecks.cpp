#include "../include/customChecks.h"
#if !defined(S_ISREG) && !defined(S_ISDIR) && defined(S_IFMT) && defined(S_IFREG)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

bool is_file(const char* filename){
    struct stat s;
    if(stat(filename,&s) != 0) return false;
    return static_cast<bool>(S_ISREG(s.st_mode));
}

bool is_file(std::string filename){
    return is_file(filename.c_str());
}

bool is_dir(const char* dir){
    struct stat s;
    if(stat(dir,&s) != 0) return false;
    return static_cast<bool>(S_ISDIR(s.st_mode));
}

bool is_dir(std::string dir){
    return is_dir(dir.c_str());
}

// template<typename Numeric> bool is_numeric(std::string arg){
//     Numeric n;
//     return((std::istringstream(arg) >> n >> std::ws).eof());
// }

// template<typename Numeric> bool is_numeric(const char* arg){
//     std::string s_arg(arg);
//     return(is_numeric<Numeric>(s_arg));
// }

// bool is_numeric (const char *arg){
//     string s(arg);
//     return !s.empty() && find_if(s.begin(),s.end(),[](unsigned char c){ return !isdigit(c);}) == s.end();
// }

int64_t file_size(const char* file){
    struct stat s;
    if(stat(file,&s)!=0) throw(errno);
    return (static_cast<int64_t>(s.st_size));
}

int64_t file_size(std::string file){
    return file_size(file.c_str());
}