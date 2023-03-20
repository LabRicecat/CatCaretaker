#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <stdlib.h>

#include "../mods/inipp.hpp"
#include "options.hpp"
#include "../inc/pagelist.hpp"

std::string get_username();
bool download_page(std::string url, std::string file);

#define CATCARE_PROGNAME "catcaretaker"

#ifdef __linux__
# include <unistd.h>
# define CATCARE_HOME "/home/" + get_username() + "/.config/" CATCARE_PROGNAME "/"
# define CATCARE_DIRSLASH "/"
# define CATCARE_DIRSLASHc '/'
inline void cat_clsscreen() { system("clear"); }
inline void cat_sleep(unsigned int mil) { usleep(mil * 1000); }
#elif defined(_WIN32)
# include <windows.h>
# define CATCARE_HOME "C:\\ProgramData\\." CATCARE_PROGNAME "\\"
# define CATCARE_DIRSLASH "\\"
# define CATCARE_DIRSLASHc '\\'
inline void cat_clsscreen() { system("cls"); }
inline void cat_sleep(unsigned int mil) { Sleep(mil); }
#endif

#define CATCARE_URLRULES_FILENAME "urlrules.ccr"
#define CATCARE_URLRULES_FILE CATCARE_HOME CATCARE_URLRULES_FILENAME
#define CATCARE_CONFIG_FILENAME "config.inipp"
#define CATCARE_CONFIG_FILE CATCARE_HOME CATCARE_CONFIG_FILENAME
#define CATCARE_TMP_DIR "tmp"
#define CATCARE_TMP_PATH CATCARE_HOME CATCARE_TMP_DIR
#define CATCARE_MACRO_DIR "macros"
#define CATCARE_MACRO_PATH CATCARE_HOME CATCARE_MACRO_DIR
#define CATCARE_EXTENSION_DIR "extensions"
#define CATCARE_EXTENSION_PATH CATCARE_HOME CATCARE_EXTENSION_DIR
#define CATCARE_ATTACHMENT_DIR "attachments"
#define CATCARE_ATTACHMENT_PATH CATCARE_HOME CATCARE_ATTACHMENT_DIR


#define CATCARE_CHECKLISTNAME "cat_checklist.inipp"
#define CATCARE_REGISTERNAME "cat_register.inipp"
#define CATCARE_RELEASES_FILE "releases.inipp"
#define CATCARE_BROWSING_FILE "browsing.inipp"

#define CATCARE_ROOT_NAME std::string("catpkgs")
#define CATCARE_ROOT (!arg_settings::global ? CATCARE_ROOT_NAME : CATCARE_HOME + CATCARE_ROOT_NAME)
#define CATCARE_BROWSE_OFFICIAL "https://raw.githubusercontent.com/labricecat/catcaretaker/main/" CATCARE_BROWSING_FILE
#define CATCARE_CARESCRIPT_EXT ".ccs"


std::vector<UrlPackage> get_download_url(std::string input);
void download_dependencies(IniList list);

std::string download_project(std::string url);
IniFile download_checklist(std::string url);

// new - old (new == "" when no update needed)
std::tuple<std::string,std::string> needs_update(std::string name);

#endif