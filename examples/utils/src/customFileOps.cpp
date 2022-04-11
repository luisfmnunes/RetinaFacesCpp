#include "../include/customFileOps.h"

std::vector<std::string> utils_file_vec(0);
static int dirent_recursion(std::string dir, std::string filter, std::vector<std::string>& out);
static int list_files_callback(const char *fpath, const struct stat *s, int typeflag);

#ifndef _WIN32
std::vector<std::string> list_files_from_ftw(const char *dir, const char *filter){
    ftw(dir, list_files_callback, FTW_F);
    std::vector<std::string> out (utils_file_vec);

    if(filter != NULL){    
        for(std::vector<std::string>::iterator it = out.begin();it != out.end();){
            if(get_filename(*it).find(filter)==std::string::npos) it = out.erase(it);
            else it++;
        }
    }

    utils_file_vec.clear();
    return out;
}

std::vector<std::string> list_files_from_ftw(std::string dir, std::string filter){
    ftw(dir.c_str(), list_files_callback, FTW_F);
    std::vector<std::string> out (utils_file_vec);

    if(!filter.empty()){
        for(std::vector<std::string>::iterator it = out.begin();it != out.end();){
            if(get_filename(*it).find(filter)==std::string::npos) it = out.erase(it);
            else it++;
        }
    }

    utils_file_vec.clear();
    return out;
}

std::vector<std::string> list_files_from_ftw_sorted(std::string dir, std::string filter){
    ftw(dir.c_str(), list_files_callback, FTW_F);
    std::vector<std::string> out (utils_file_vec);

    if(!filter.empty()){
        for(std::vector<std::string>::iterator it = out.begin();it != out.end();){
            if(it->find(filter)==std::string::npos) it = out.erase(it);
            else it++;
            // _debug("%s: %d",it->c_str(),out.size());
        }
    }

    std::sort(out.begin(), out.end(), [dir](std::string first, std::string second){
        return first.substr(first.find(dir)) < second.substr(second.find(dir));
    });

    utils_file_vec.clear();
    return out;
}

std::vector<std::string> list_files_from_directory(std::string dir, std::string filter){
    std::vector<std::string> out(0);

    if(dir.empty()){
        _error("Expected directory for listing, received %s",dir.c_str());
        return std::vector<std::string>();
    }

    if( *(dir.end()-1) != '/')
        dir.append("/");

    dirent_recursion(dir,filter,out);
    
    return out;
}

std::vector<std::string> list_files_from_directory_sorted(std::string dir, std::string filter){
    std::vector<std::string> out(0);

    if(dir.empty()){
        _error("Expected directory for listing, received %s",dir.c_str());
        return std::vector<std::string>();
    }

    if( dir.back() == '/' )
        dir.erase(dir.end()-1);

    dirent_recursion(dir,filter,out);

    std::sort(out.begin(),out.end(),[dir](std::string first, std::string second){
        return (first.substr(first.find(dir)) < second.substr(second.find(dir)));
    });
    
    return out;
}

static int list_files_callback(const char *fpath, const struct stat *s, int typeflag){
    if(typeflag == FTW_F)
        utils_file_vec.push_back(fpath);
    
    return 0;
}

static int dirent_recursion(std::string dir, std::string filter, std::vector<std::string>& out){
    struct dirent *dp;
    DIR *dir_ptr;

    if(!(dir_ptr = opendir(dir.c_str()))){
        _error("Couldnt open directory stream");
        return EXIT_FAILURE;
    }

     while((dp = readdir(dir_ptr)) != NULL){
        std::string path(dir+'/'+dp->d_name);
        if(dp->d_type == DT_DIR){
            if(strcmp(dp->d_name,".") == 0 || strcmp(dp->d_name,"..") == 0)
                continue;
            
            // std::cout << path << std::endl;
            dirent_recursion(path,filter,out);
        }
        else{
            // _debug("%s",path.c_str());
            if(filter.empty())
                out.push_back(path);
            else if(get_filename(path).find(filter)!=std::string::npos)
                out.push_back(path);
        }
    }

    closedir(dir_ptr);
    return EXIT_SUCCESS;
}

int mkdir(const std::string path){
    char * command = (char*) malloc(sizeof(char)*path.size() + 6);
    sprintf(command,"mkdir %s",path.c_str());
    return system(command);
}
#else

std::vector<std::string> list_files_from_directory(std::string dir, std::string filter){ //NOT RECURSIVE - Should add recursion to fileinfo.attrib & _A_SUBDIR
	std::vector<std::string> out(0);
	char originalDirectory[_MAX_PATH];

	// Get the current directory so we can return to it
	_getcwd(originalDirectory, _MAX_PATH);

	_chdir(dir.c_str());  // Change to the working directory
	_finddata_t fileinfo;

	// This will grab the first file in the directory
	// "*" can be changed if you only want to look for specific files
	intptr_t handle = _findfirst(filter.empty() ? "*" : filter.c_str(), &fileinfo);

	if (handle == -1)  // No files or directories found
	{
		os_warn("No", filter.c_str(), "files in the current directory!\n");
		return out;
	}

	do
	{
		if (strcmp(fileinfo.name, ".") == 0 || strcmp(fileinfo.name, "..") == 0)
			continue;
		if (fileinfo.attrib & _A_SUBDIR) // Use bitmask to see if this is a directory
			continue;
		else
			out.push_back(dir + '\\' + fileinfo.name);
	} while (_findnext(handle, &fileinfo) == 0);

	_findclose(handle); // Close the stream

	_chdir(originalDirectory);
	
	return out;
}

std::vector<std::string> list_files_from_directory_sorted(std::string dir, std::string filter){
	std::vector<std::string> out(0);

	out = list_files_from_directory(dir, filter);

	std::sort(out.begin(), out.end(), [dir](std::string first, std::string second){
		return (str_lower(first.substr(first.find(dir))) < str_lower(second.substr(second.find(dir))));
	});

	return out;
}

int mkdir(const std::string path){
	return _mkdir(path.data());
}

#endif
;