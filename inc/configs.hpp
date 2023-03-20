#ifndef CONFIGS_HPP
#define CONFIGS_HPP

#include "network.hpp"
#include "../mods/inipp.hpp"

inline static RuleList global_rulelist;

void make_file(std::string name, std::string std = "");
void fill_global_pagelist();

void reset_localconf();
void load_localconf();
std::string healthcheck_localconf();
void delete_localconf();
void write_localconf();

namespace carescript { struct Interpreter; }
void load_extensions(carescript::Interpreter& interp);

IniDictionary extract_configs(IniFile file);

bool valid_configs(IniDictionary conf);

std::string config_healthcare(IniDictionary conf);

void make_register();
IniDictionary get_register();
bool installed(std::string name);
void add_to_register(std::string url, std::string name);
void remove_from_register(std::string name);

bool is_dependency(std::string name);
void add_to_dependencylist(std::string name);
void remove_from_dependencylist(std::string name);
IniList get_dependencylist();

std::string url2name(std::string url);

void make_checklist();

IniList get_filelist();
void set_filelist(IniList list);

bool blacklisted(std::string url);

#endif