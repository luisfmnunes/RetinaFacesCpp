#ifndef CUSTOM_FILE_OPS_H
#define CUSTOM_FILE_OPS_H

/** @file customFileOps.h 
 *  @brief The module responsible for File and Directory related operations
 * 
 * A header containing functions used to interact with Directories and Files
*/

#include<stdio.h>
#include<stdlib.h>
#ifndef _WIN32
#include<ftw.h>
#include<dirent.h>
#else
#include<io.h>
#endif
#include<algorithm>


#include "customLog.h"
#include "customStringOps.h"

#ifndef _WIN32
///List files using a file tree walk
/**
 * Retrieves all files in a directory and its subdirectories
 * 
 * @param dir [in] - A const char pointer containing the directory name
 * @param filter [in] (default = "") - A const char pointer containing a filter expression if desired filtering
 * 
 */
std::vector<std::string> list_files_from_ftw(const char *dir, const char *filter = NULL);
///List files using a file tree walk (String version)
/**
 * Retrieves all files in a directory and its subdirectories
 * 
 * @param dir [in] - A string containing the directory name
 * @param filter [in] (defulta = "") - A string containing a filter expression if desired filtering
 * 
 */
std::vector<std::string> list_files_from_ftw(std::string dir, std::string filter = "");
///List files using a file tree walk and sort the output
/**
 * Retrieves all files in a directory and its subdirectories and sort the output vector
 * 
 * @param dir [in] - A string containing the directory name
 * @param filter [in] (default = "") - A string containing a filter expression if desired filtering 
 * 
 */
std::vector<std::string> list_files_from_ftw_sorted(std::string dir, std::string filter = "");
#endif
///List files using dirent structure (faster but limited)
/**
 * Retrieves all files in a directory and its subdirectoryes (recursively)
 * 
 * @param dir [in] - A string containing the directory name
 * @param filter [in] (default = "") - A string containing a filter expression if desired filtering
 */
std::vector<std::string> list_files_from_directory(std::string dir, std::string filter = "");
///List files using dirent structure sorting output vector (faster but limited)
/**
 * Retrieves all files in a directory and its subdirectoryes (recursively), and sort the output result
 * 
 * @param dir [in] - A string containing the directory name
 * @param filter [in] (default = "") - A string containing a filter expression if desired filtering
 */
std::vector<std::string> list_files_from_directory_sorted(std::string dir, std::string filter = "");

int mkdir(const std::string path);
#endif