#include "../inc/configs.hpp"
#include "../inc/options.hpp"
#include "../inc/pagelist.hpp"
#include "../carescript/carescript-api.hpp"

#include <string.h>

static std::streamsize get_flength(std::ifstream& file) {
	if(!file.is_open()) {
		return 0;
	}
	std::streampos temp_1 = file.tellg();
	file.seekg(0, std::fstream::end);
	std::streampos temp_2 = file.tellg();
	file.seekg(temp_1, std::fstream::beg);

	return temp_2;
}

static std::string read(std::string path) {
	std::ifstream ifile;
	ifile.open(path, std::ios::binary);
	std::streamsize len = get_flength(ifile);
	char* dummy = new char[len];

	try {
		ifile.read(dummy, len);
	}
	catch(std::exception& err) {
		ifile.close();
        return "";
	}
	if (dummy == nullptr || strlen(dummy) == 0) {
		ifile.close();
        return "";
	}
	ifile.close();
	//dummy += '\0';
	std::string re;
	re.assign(dummy, len);

	delete[] dummy;
	dummy = nullptr;

	return re;
}

void make_file(std::string name, std::string stdc) {
    std::ofstream of;
    of.open(name,std::ios::trunc);
    of.close();
    if(stdc != "") {
        of.open(name,std::ios::app);
        of << stdc;
        of.close();
    }
}

std::string healthcheck_localconf() {
    if(arg_settings::no_config) return "";
    if(!std::filesystem::exists(CATCARE_HOME))
        std::filesystem::create_directory(CATCARE_HOME);
    std::ofstream of;
    if(!std::filesystem::exists(CATCARE_CONFIG_FILE)) {   
        make_file(CATCARE_CONFIG_FILE);
        of.open(CATCARE_CONFIG_FILE,std::ios::app);
        of << "options = {}\nblacklist = []\n";
        of.close();
    }
    if(!std::filesystem::exists(CATCARE_ATTACHMENT_PATH))
        std::filesystem::create_directory(CATCARE_ATTACHMENT_PATH);
    if(!std::filesystem::exists(CATCARE_MACRO_PATH))
        std::filesystem::create_directory(CATCARE_MACRO_PATH);
    if(!std::filesystem::exists(CATCARE_EXTENSION_PATH))
        std::filesystem::create_directory(CATCARE_EXTENSION_PATH);
    if(!std::filesystem::exists(CATCARE_URLRULES_FILE))
        make_file(CATCARE_URLRULES_FILE,R"(
"--- urlrules.ccr ---"
" may be changed by the user "

"- DEFAULT GITHUB RULE -"
RULE github;
    LINK "https://raw.githubusercontent.com/{user}/{project}/{branch}/";
    WITH user 1
         project 2
         branch 3;
    SIGNS / @;
)");
    IniFile ifile = IniFile::from_file(CATCARE_CONFIG_FILE);
    if(!ifile) { return "corrupted config file at " CATCARE_CONFIG_FILE; }
    return "";
}

void reset_localconf() {
    if(arg_settings::no_config) return;
    if(std::filesystem::exists(CATCARE_HOME)) {
        delete_localconf();
    }
    healthcheck_localconf();
}

void load_localconf() {
    if(arg_settings::no_config) return;
    IniFile file = IniFile::from_file(CATCARE_CONFIG_FILE);
    IniDictionary s = file.get("options");
    for(auto i : s) {
        options[i.first] = (std::string)i.second;
    }
    // IniDictionary b = file.get("blacklist") TODO
    fill_global_pagelist();
}

void delete_localconf() {
    if(arg_settings::no_config) return;
    std::filesystem::remove_all(CATCARE_HOME);
}

void write_localconf() {
    if(arg_settings::no_config) return;
    IniFile file = IniFile::from_file(CATCARE_CONFIG_FILE);
    IniDictionary s = file.get("options");
    for(auto i : options) {
        IniElement elm;
        elm = i.second;
        s[i.first] = elm;
    }
    file.set("options",s);
    file.to_file(CATCARE_CONFIG_FILE);
}

void load_extensions(carescript::Interpreter& interp) {
    if(arg_settings::no_config) return;
    for(auto i : std::filesystem::recursive_directory_iterator(CATCARE_EXTENSION_PATH)) {
        if(i.is_directory()) continue;
        carescript::bake_extension(i.path().string(),interp.settings);
    }
}

IniDictionary extract_configs(IniFile file) {
    IniDictionary ret;
    if(!file || !file.has("name","Info") || !file.has("files","Download")) {
        return ret;
    }

    ret["name"] = file.get("name","Info");
    ret["files"] = file.get("files","Download");
    if(file.has("version","Info"))
        ret["version"] = file.get("version","Info");
    if(file.has("dependencies","Download"))
        ret["dependencies"] = file.get("dependencies","Download");
    if(file.has("scripts","Download"))
        ret["scripts"] = file.get("scripts","Download");
    if(file.has("description","Info"))
        ret["description"] = file.get("description","Info");
    if(file.has("tags","Info"))
        ret["tags"] = file.get("tags","Info");
    if(file.has("authors","Info"))
        ret["authors"] = file.get("authors","Info");
    if(file.has("license","Info"))
        ret["license"] = file.get("license","Info");
    if(file.has("documentation","Info"))
        ret["documentation"] = file.get("documentation","Info");

    return ret;
}

bool valid_configs(IniDictionary conf) {
    return config_healthcare(conf) == "";
}

std::string config_healthcare(IniDictionary conf) {
    if(conf.count("name") == 0) return "\"name\" is required!";
    if(conf.count("files") == 0) return "\"files\" is required!";
    if(conf["name"].get_type() != IniType::String) return "\"name\" must be a string!";
    if(conf["files"].get_type() != IniType::List) return "\"files\" must be a list!";

    if(conf.count("dependencies") != 0 && conf["dependencies"].get_type() != IniType::List) return "\"dependencies\" must be a list!";
    if(conf.count("version") != 0 && conf["version"].get_type() != IniType::String) return "\"version\" must be a string!";
    if(conf.count("scripts") != 0 && conf["scripts"].get_type() != IniType::List) return "\"scripts\" must be a list!";
    if(conf.count("description") != 0 && conf["description"].get_type() != IniType::String) return "\"description\" must be a string!";
    if(conf.count("tags") != 0 && conf["tags"].get_type() != IniType::List) return "\"tags\" must be a list!";
    if(conf.count("authors") != 0 && conf["authors"].get_type() != IniType::List) return "\"authors\" must be a list!";
    if(conf.count("license") != 0 && conf["license"].get_type() != IniType::String) return "\"license\" must be a string!";
    if(conf.count("documentation") != 0 && conf["documentation"].get_type() != IniType::String) return "\"documentation\" must be a string!";
    
    return "";
}

void make_register() {
    if(!std::filesystem::exists(CATCARE_ROOT)) {
        std::filesystem::create_directory(CATCARE_ROOT);
    }
    if(!std::filesystem::exists(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME)) {
        std::ofstream os;
        os.open(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME,std::ios::app);
        os << "installed = []\n";
        os.close();
    }
}

IniDictionary get_register() {
    make_register();
    IniFile reg = IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
    return reg.get("installed").to_dictionary();
}

bool installed(std::string name) {
    IniDictionary lst = get_register();
    for(auto i : lst) {
        if((std::string)i.first == name || (std::string)i.second == name) return true;
    }
    return false;
}

void add_to_register(std::string url, std::string name) {
    if(installed(url)) { return; }
    name = to_lowercase(name);
    IniFile reg = IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
    IniDictionary l = reg.get("installed").to_dictionary();
    l[name] = url;
    reg.set("installed",l);
    reg.to_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
}

void remove_from_register(std::string name) {
    name = to_lowercase(name);
    if(!installed(name)) { return; }

    IniFile reg = IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
    IniDictionary d = reg.get("installed").to_dictionary();
    d.erase(name);
    reg.set("installed",d);
    reg.to_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
}

bool is_dependency(std::string url) {
    if(arg_settings::global) return false;
    IniList lst = get_dependencylist();
    for(auto i : lst) {
        if(i.get_type() == IniType::String && url == (std::string)i) {
            return true;
        }
    }
    return false;
}

void add_to_dependencylist(std::string url) {
    if(arg_settings::global) return;
    if(is_dependency(url)) return;
    IniFile reg = IniFile::from_file(CATCARE_CHECKLISTNAME);
    IniList l = reg.get("dependencies","Download");
    IniElement elm = url;
    l.push_back(elm);
    reg.set("dependencies",l,"Download");
    reg.to_file(CATCARE_CHECKLISTNAME);
}

void remove_from_dependencylist(std::string url) {
    if(arg_settings::global) return;
    IniFile reg = IniFile::from_file(CATCARE_CHECKLISTNAME);
    IniList l = reg.get("dependencies","Download").to_list();

    for(size_t i = 0; i < l.size(); ++i) {
        if(l[i].get_type() == IniType::String && (std::string)l[i] == url) {
            l.erase(l.begin()+i);
            break;
        }
    }
    reg.set("dependencies",l,"Download");
    reg.to_file(CATCARE_CHECKLISTNAME);
}

std::string url2name(std::string url) {
    IniDictionary regist = get_register();
    for(auto i : regist) {
        if((std::string)i.second == url) return i.first;
    }
    return "";
}

IniList get_dependencylist() {
    IniFile reg = IniFile::from_file(CATCARE_CHECKLISTNAME);
    return reg.get("dependencies","Download").to_list();
}

void make_checklist() {
    if(!std::filesystem::exists(CATCARE_CHECKLISTNAME)) {
        make_file(CATCARE_CHECKLISTNAME,R"(
[Info]
name = "project-name"
# optional
version = "0.0.0" 
description = ""
tags = []
authors = []
license = ""
documentation = "" # put the link here

[Download]
files = [] # append files manually or use `catcare add <file>`
# optional
dependencies = [] # append manually and sync or use `catcare get <user>/<project>`
scripts = [] 

)");
    }
}

IniList get_filelist() {
    if(std::filesystem::exists(CATCARE_CHECKLISTNAME)) {
        IniFile file = IniFile::from_file(CATCARE_CHECKLISTNAME);
        return file.get("files","Download");
    }
    return IniList();
}
void set_filelist(IniList list) {
    if(std::filesystem::exists(CATCARE_CHECKLISTNAME)) {
        IniFile file = IniFile::from_file(CATCARE_CHECKLISTNAME);
        file.set("files",list,"Download");
        file.to_file(CATCARE_CHECKLISTNAME);
    }
}

bool blacklisted(std::string url) {
    IniFile file = IniFile::from_file(CATCARE_CONFIG_FILE);
    IniList l = file.get("blacklist");
    for(auto i : l) {
        if(i.get_type() == IniType::String && (std::string)i == url) return true;
    }
    return false;
}