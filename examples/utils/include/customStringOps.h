#ifndef CUSTOM_STRING_OPS_H
#define CUSTOM_STRING_OPS_H

/** @file customStringOps.h
 *  @brief A module for string operations.
 * 
 *  A header containing functions to perform string operations, returning different structures depending on the operation.
 * 
 */

#include<iostream>
#include<cstring>
#include<cstdlib>
#include<string>
#include<vector>

///Extract the filename from a filepath
/**
 * Receives a path and extract the filename of the path (The filename can't contain slashes (/) on name)
 * 
 * @param path [in] A string containing a filepath
 * 
 * @return A substring containing the filename
 */
std::string get_filename(std::string path);
///Extract the file directory from a filepath
/**
 * Receives a path and extract the file directory of the path (The filename can't contain slashes (/) on name)
 * 
 * @param path [in] A string containing a filepath
 * 
 * @return A substring containing the file directory or empty string if the filepath is a filename
 */ 
std::string get_filedir(std::string path);
///Splits the string based on a delimiter
/**
 * Receives a string possibly containing multiple elements separated by a delimiter
 * 
 * @param input [in] A string containing multiple elements separated by a delimiter
 * @param delimiter [in] (default = whitespace) A string defining the spliting delimiter
 * 
 * @return A vector containing multiple elements or an empty vector if the delimiter is not found on the input
*/
std::vector<std::string> str_split_or_empty(std::string input, std::string delimiter = " ");
///Splits the string based on a delimiter
/**
 * Receives a string possibly containing multiple elements separated by a delimiter
 * 
 * @param input [in] A string containing multiple elements separated by a delimiter
 * @param delimiter [in] (default = whitespace) A string defining the spliting delimiter
 * 
 * @return A vector containing multiple elements or vector containing the original string if the delimiter is not found on the input
*/
std::vector<std::string> str_split(std::string input, std::string delimiter = " ");
///Trim a string on both sides
/**
 * Receives a string possibly containing multiple whitespaces at the beginning or end
 * 
 * @param input [in] A string which the trimmed version is desired
 * 
 * @return A string in its trimmed version
*/
std::string str_trim(std::string input);

bool has_extension(std::string path);
bool in_quotes(std::string text);
std::string remove_quotes(std::string input);
std::string remove_extension(std::string input);
std::string str_lower(std::string input);
std::string str_upper(std::string input);

#endif
