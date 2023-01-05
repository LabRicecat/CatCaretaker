#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <iostream>
#include <string>
#include <map>
#include <filesystem>
#include <tuple>

inline bool opt_silence = false;

inline std::map<std::string,std::string> options;
inline std::map<std::string,std::string> local_redirect;

bool is_redirected(std::string name);
void make_redirect(std::string name, std::string path);

void print_message(std::string mod, std::string message);
bool confirmation(std::string context);

std::string ask_and_default(std::string defau);
std::string option_or(std::string option, std::string els);
void await_continue();
std::string app_username(std::string str);
std::string last_name(std::filesystem::path path);
std::string to_lowercase(std::string str);

std::tuple<std::string,std::string> get_username(std::string str);

bool browse(std::string file);

#endif