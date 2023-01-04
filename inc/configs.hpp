#ifndef CONFIGS_HPP
#define CONFIGS_HPP

#include "network.hpp"
#include "../mods/inipp.hpp"

void make_file(std::string name, std::string std = "");

void reset_localconf();
void load_localconf();
void delete_localconf();
void write_localconf();

IniDictionary extract_configs(std::string name);

bool valid_configs(IniDictionary conf);

std::string config_healthcare(IniDictionary conf);

void make_register();
IniList get_register();
bool installed(std::string name);
void add_to_register(std::string name);
void remove_from_register(std::string name);

bool is_dependency(std::string name);
void add_to_dependencylist(std::string name, bool local = false);
void remove_from_dependencylist(std::string name);
IniList get_dependencylist();

void make_checklist();

IniList get_filelist();
void set_filelist(IniList list);

bool blacklisted(std::string repo);

#endif