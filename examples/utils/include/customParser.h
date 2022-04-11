#ifndef CUSTOM_PARSER_H
#define CUSTOM_PARSER_H

/** @file customParser.h 
 *  @brief The module contains a parse Class that instatiate a parse Object with possibility to add options, flags, and an automatic help print.
 * 
 * A header containing functions used and macros used to parse information from command line.
 * The macros are used to store the variable names binded to certain option/flag/help The list of macros are:
 * 
 * - GPARSER_ADD_OPTION(parser,option_name,help_str,ref): receives a parser object and add option to parser binding variable name of ref variable to such option 
 * - GPARSER_ADD_FLAG(parser,option_name,help_str,ref): same as GPARSER_ADD_OPTION but related to a FLAG <
 * - GPARSER_ADD_HELP(parser,option_name): add an option exclusive to help printing 
 * - GPARSER_PARSE(parser,argc,argv): parse command line 
*/

#include<queue>
#include<map>
#include<functional>
#include "customChecks.h"
#include "customStringOps.h"
#include "customLog.h"

/** 
 * This macro receives a GParser Object and call the @ref GParser::add_option "add_option" and @ref GParser::add_var_name "add_var_name" routines with given parameters
 * 
 * @param parser [in]: A GParser Object meant to be used.
 * @param option_name [in]: A string containing the option directive(s), separated by comma if multiple <b>(*e.g* "-f,--file")</b>;
 * @param help_str [in]: A string containing the help description of the option
 * @param ref [in]: Variable to be parsed by given option
 * 
 * @code
 * #include "path_to/customParser.h"
 * #include "path_to/customLog.h"
 * 
 * using namespace std;
 * 
 * int main(int argc, char** argv){
 *      GParser parser("Example of Parser", true); //Creates parser with description and debug ON.
 *      string text = "";
 *      int value = 0;
 * 
 *      os_debug("Result of text before: ", text);
 *      os_debug("Result of value before: ",value);
 * 
 *      GPARSER_ADD_OPTION(parser,"-s,--string","Example of option to set string value",text); //Add options -s and --string to change text
 *      GPARSER_ADD_OPTION(parser,"-i","Example of option to set int value", value); //Add option -i to change value
 *
 *      GPARSER_PARSE(parser,argc,argv); //Parse command line
 * 
 *      // Possible command lines:
 *      // ./example -i 30 -s Griaule
 *      // ./example -is 30 Griaule 
 *      // ./example --string Griaule -i 30
 *      os_debug("Result of text after: ", text); //should print "Result of text after: Griaule"
 *      os_debug("Result of value after: ", value); //should print "Result of value after: 30"
 *  
 *      return EXIT_SUCESS;
 * } 
 *
 * @endcode
 */
#define GPARSER_ADD_OPTION(parser,option_name,help_str,ref) \
    parser.add_option(option_name,help_str,ref); \
    parser.add_var_name(option_name,#ref); ///< Add option to parser and bind ref variable to option
/**
 * This macro receives a GParser Object and call the @ref GParser::add_flag "add_flag" and @ref GParser::add_var_name "add_var_name" routines with given parameters
 * 
 * @param parser [in]: A GParser Object meant to be used.
 * @param option_name [in]: A string containing the flag directive(s), separated by comma if multiple <b>(*e.g* "-s,--sort")</b>;
 * @param help_str [in]: A string containing the help description of the flag
 * @param ref [in]: Bool to be parsed by given flag
 * 
 * @code
 * #include "path_to/customParser.h"
 * #include "path_to/customLog.h"
 * 
 * using namespace std;
 * 
 * int main(int argc, char** argv){
 *      GParser parser("Example of Parser", true); //Creates parser with description and debug ON.
 *      bool sort = false;
 *  
 *      os_debug("Result of sort before: ", sort ? "true" : "false"); //false
 * 
 *      GPARSER_ADD_FLAG(parser,"-s,--sort","Example of option of flag",sort); //Add options -s and --sort to set sort
 *      GPARSER_PARSE(parser,argc,argv); //Parse command line
 * 
 *      // Possible command lines:
 *      // ./example -s 
 *      // ./example --sort
 *      os_debug("Result of sort after: ", sort ? "true" : "false"); //true
 *  
 *      return EXIT_SUCESS;
 * } 
 *
 * @endcode
 */
#define GPARSER_ADD_FLAG(parser,option_name,help_str,ref) \
    parser.add_flag(option_name,help_str,ref); \
    parser.add_var_name(option_name,#ref); ///< Add flag to parser and bind ref variable to flag

#define GPARSER_ADD_HELP(parser,option_name) \
    parser.add_help(option_name); ///< Add a help option to parser

#define GPARSER_PARSE(parser,argc,argv) parser.parseValues(argc,argv); ///< Parse the command line with the parser object

///An object to parse command line
class GParser{
    public:
        ///An enumerator of possible parsing errorsto assist
        enum class GPErrors{ 
            PERROR_NONE, ///<No Error
            PERROR_INVALID_DATA, ///<Invalid Data passed to parser
            PERROR_INSUFICIENT_ARGUMENTS, ///<Insuficient Arguments given to option
        };
       

        GParser(); ///< Default constructor
        ///Description constructor
        /**
         * A constructor to set the description of the module
         * 
         * @param description [in]: String containing the description of the module
         */
        GParser(std::string description);
        ///Debug constructor
        /**
         * A constructor to set the debug mode of the parser
         * 
         * @param debug [in]: bool to set debug
         */
        GParser(bool debug);
        ///Description and debug constructor
        /**
         * Construct object and set description and debug
         * 
         * @param description [in]: A string containing the description of the module
         * @param debug [in]: A bool setting the debug mode
         */
        GParser(std::string description, bool debug);
        ///Description and usage constructor
        /**
         * Construct object and set description and usage
         * 
         * @param description [in]: A string containing the description of the module
         * @param usage [in]: A string containing the usage of the module
         */
        GParser(std::string description, std::string usage);
        ///Description, usage and debug constructor
        /**
         * A full constructor of the parser class
         * 
         * @param description [in]: A string containing the description of the module
         * @param usage [in]: A string containing the usage of the module
         * @param debug [in]: A bool setting the debug mode
         */
        GParser(std::string description, std::string usage, bool debug);
        ///Parse the values of the stored variables
        /**
         * Receives the command line arguments (argc and argv) and resolve the variables stored on the object
         * 
         * @param argc [in]: Count of arguments present on command line
         * @param argv [in]: Buffer array containing the arguments passed on command line
         * 
         * @see GPARSER_PARSE
         */
        GPErrors parseValues(int argc, char** argv);
        ///Add a flag to parser
        /**
         * Add a flag to the parser object and bind the bool var to the options present in option_name.
         * Recommended use of macro GPARSER_ADD_FLAG to store variable name
         * 
         * @param option_name [in]: A string containing the options separated by commas (",")
         * @param help_str [in]: A description of the flag for the automatic help print
         * @param var [in]: Reference to boolean variable to be set in the module
         * 
         * @see #GPARSER_ADD_FLAG
         */
        GPErrors add_flag(std::string option_name, std::string help_str, bool &var);
        ///Add automatic help to parser
        GPErrors add_help(std::string option_name);
        ///Add var name to a set of options, used by macros
        /**
         * Receives the options and the var name as argument and store on the map
         * 
         * @param option_name [in]: A string containing the options separated by commas (",")
         * @param var_name [in]: A string containing the name of the variable
         */
        GPErrors add_var_name(std::string option_name, std::string var_name);
        ///Set debug mode of the parser
        /**
         * Set debug mode of the parser
         * 
         * @param value [in]: A boolean to set the debug mode
         */
        GPErrors set_debug(bool value);
        ///Add a option to parser
        /**
         * Add a flag to the parser object and bind the bool var to the options present in option_name.
         * Recommended use of macro GPARSER_ADD_OPTION to store variable name
         * 
         * @param option_name [in]: A string containing the options separated by commas (",")
         * @param help_str [in]: A description of the option for the automatic help print
         * @param var [in]: Reference to template variable to be set in the calling module
         * 
         * @see #GPARSER_ADD_OPTION
         */
        template <typename T> inline GPErrors add_option(std::string option_name,std::string help_str, T &var){
            return add_directive(option_name,help_str,var,true);
        };

        ~GParser();

    private:

        bool debug; ///< Debug mode of the parser
        std::string description; ///< Description of the parser to print help
        std::string usage; ///< Usage of the parser to print help
        std::map<   std::string ,   std::function<void (std::string)> >references; ///< Map of options directives containing lambdas to alter references' values
        std::map<   std::string ,   std::string > var_names; ///< Map of options directives containing variable names
        std::map<   std::string ,   std::string > help; ///< Map of options directives containing the help description
        std::map<   std::string ,   bool > required; ///<< Map of options directives indicating if it's a flag or an option
        
        void print_help(); ///< Function to print parser help

        ///Core function of add_flag and add_option
        template <typename T> inline GPErrors add_directive(std::string option_name, std::string help_str, T &var, bool req){
            std::vector<std::string> directives = str_split(option_name,",");
            for(auto directive : directives){
                directive = str_trim(directive);
                while(directive.find('-')!=std::string::npos)
                    directive.erase(directive.find('-'),1);
                
                references[directive] = [this,&var,directive](std::string value) {
                    if(is_type_numeric(var) && !is_numeric<T>(value))
                        _warn("Variable %s passed is numeric but argument %s is not.", this->var_names.find(directive)==this->var_names.end() ? "" : this->var_names[directive].c_str(), value.c_str());
                    std::stringstream ss;
                    ss << value;
                    ss >> var;
                    ss.clear();
                };
                required[directive] = req;
            }
            help[option_name] = option_name+": "+help_str;
            return GPErrors::PERROR_NONE;
        }
};

#endif