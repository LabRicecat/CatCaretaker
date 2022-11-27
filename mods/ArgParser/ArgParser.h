#ifndef ARG_PARSER_H_
#define ARG_PARSER_H_

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#define ARG_GET 2
#define ARG_SET 1
#define ARG_TAG 0

#define CLEAR_LOG
#define LOG(message)

struct Arg
{
    enum class Priority
    {
        OPTIONAL,
        FORCE,
        IGNORE
    };
    ///
    /// type >= 2:  get     (takes 1 arg without call)
    /// type = 1:   set     (takes 1 arg with call)
    /// type = 0:   tag     (takes no arg with call)
    ///
    Priority priority = Priority::FORCE;
    int type = 0;
    std::string name;
    std::vector<std::string> aliase;
    // internal
    bool is = false;
    int fixed_pos = -1;
    std::string val = "";

    bool hasAlias(std::string name);
};

enum class ArgParserErrors 
{
    OK,
    UNKNOWN,
    NO_ARGS,
    UNKNOWN_ARG,
    INVALID_SET,
    UNMATCHED_DEP_FORCE,
    UNMATCHED_DEP_IGNORE,
    POSITION_MISSMATCH
};

class ParsedArgs
{
    std::vector<Arg> args_tag;
    std::vector<Arg> args_set;
    std::vector<Arg> args_get;
    ArgParserErrors error_code = ArgParserErrors::OK;
    std::string error_msg = "";

    std::vector<std::string> bin;
    bool bin_filled = false;
public:
    ParsedArgs(std::vector<Arg> args, ArgParserErrors error_code, std::string error_msg, std::vector<std::string> bin = {}) 
    {
        if (!args.empty()) 
        {
            for (auto &i : args) 
            {
                if (i.type == ARG_SET) 
                {
                    args_set.push_back(i);
                }
                else if (i.type == ARG_TAG) 
                {
                    args_tag.push_back(i);
                }
                else 
                {
                    args_get.push_back(i);
                }
            }
        }
        this->error_msg = error_msg;
        this->error_code = error_code;
        this->bin = bin;
        this->bin_filled = !bin.empty();
    }

    // returns args(tag)
    bool operator[](const char *);
    bool operator[](std::string arg);

    // returns args(set)
    std::string operator()(std::string arg);

    // returns error_code == OK
    operator bool();

    // checks for error_code == error
    bool operator==(ArgParserErrors error);
    bool operator!=(ArgParserErrors error);

    // returns the error location substring (for debug purpose)
    std::string error() const;

    // returns true if `arg_name` is a set argument
    bool has(std::string arg_name);

    std::vector<std::string> get_bin() const;

    bool has_bin() const;
};

class ArgParser
{
    char strsym = ' ';
    uint32_t unusedGetArgs = 0;
    std::vector<Arg> args;
    bool has_bin = false;
    std::vector<std::string> bin;

    size_t find(std::string name, bool &failed);

    size_t find_next_getarg(bool& failed);

public:
    ArgParser &addArg(std::string name, int type, std::vector<std::string> aliase = {},int fixed_pos = -1, Arg::Priority priority = Arg::Priority::OPTIONAL);
    ArgParser &enableString(char sym);
    ArgParser &setbin();
    ParsedArgs parse(std::vector<std::string> args);
    ParsedArgs parse(char **args, int size);
};

#endif
