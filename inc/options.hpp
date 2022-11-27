#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <iostream>
#include <string>
#include <map>
#include <filesystem>

inline bool opt_silence = false;

inline std::map<std::string,std::string> options;
inline std::map<std::string,std::string> local_redirect;

bool is_redirected(std::string name);
void make_redirect(std::string name, std::string path);

void print_message(std::string mod, std::string message);
bool confirmation(std::string context);

std::string ask_and_default(std::string defau);

std::string option_or(std::string option, std::string els);

std::string last_name(std::filesystem::path path);

std::string get_username(std::string str);

#endif