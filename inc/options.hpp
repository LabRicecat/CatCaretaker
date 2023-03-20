#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <iostream>
#include <string>
#include <map>
#include <filesystem>
#include <tuple>

namespace arg_settings {
    inline bool opt_silence = false;
    inline bool global = false;
    inline bool no_config = false;
}

inline std::map<std::string,std::string> options;
void print_message(std::string mod, std::string message);
bool confirmation(std::string context);

std::string ask_and_default(std::string defau);
std::string option_or(std::string option, std::string els);
void await_continue();
std::string last_name(std::filesystem::path path);
std::string to_lowercase(std::string str);

bool browse(std::string file, std::string url);

#endif