#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "../mods/InI++/Inipp.hpp"
#include "options.hpp"

std::string get_username();
bool download_page(std::string url, std::string file);

#define CATCARE_PROGNAME "catcaretaker"

#ifdef __linux__
# define CATCARE_HOME "/home/" + get_username() + "/." CATCARE_PROGNAME "/"
# define CATCARE_DIRSLASH "/"
# define CATCARE_DIRSLASHc '/'
#elif defined(_WIN32)
# define CATCARE_HOME "C:\\ProgramData\\." CATCARE_PROGNAME "\\"
# define CATCARE_DIRSLASH "\\"
# define CATCARE_DIRSLASHc '\\'
#endif

#define CATCARE_CONFIGFILE CATCARE_HOME "config.inipp"
#define CATCARE_TMPDIR CATCARE_HOME "tmp"
#define CATCARE_USER option_or("username","LabRiceCat")
#define CATCARE_USERREPO(x) option_or("install_url","https://raw.githubusercontent.com/") + x

#define CATCARE_BRANCH option_or("default_branch","main")

#define CATCARE_REPOFILE(repo,x) CATCARE_USERREPO(repo) + "/" + CATCARE_BRANCH + "/" + std::string(x) 

#define CATCARE_CHECKLISTNAME "cat_checklist.inipp"
#define CATCARE_REPO_CHECKLIST(x) CATCARE_USERREPO(x) "/" CATCARE_CHECKLISTNAME

#define CATCARE_REGISTERNAME "cat_register.inipp"

#define CATCARE_ROOT option_or("install_dir","catmods")

void download_dependencies(IniList list);

std::string download_repo(std::string name);
std::string get_local(std::string name, std::string path);

// new - old (new == "" when no update needed)
std::tuple<std::string,std::string> needs_update(std::string name);

#endif