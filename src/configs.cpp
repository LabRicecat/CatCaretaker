#include "../inc/configs.hpp"
#include "../inc/options.hpp"

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

void make_file(std::string name, std::string std) {
    std::ofstream of;
    of.open(name,std::ios::trunc);
    of.close();
    if(std != "") {
        of.open(name,std::ios::app);
        of << std;
        of.close();
    }
}

void reset_localconf() {
    if(std::filesystem::exists(CATCARE_HOME)) {
        delete_localconf();
    }

    std::filesystem::create_directory(CATCARE_HOME);
    std::ofstream of;
    make_file(CATCARE_CONFIGFILE);
    of.open(CATCARE_CONFIGFILE,std::ios::app);
    of << "options={}\nredirects={}\n";
    of.close();
}

void load_localconf() {
    IniFile file = IniFile::from_file(CATCARE_CONFIGFILE);
    IniDictionary s = file.get("options");
    for(auto i : s) {
        options[i.first] = (std::string)i.second;
    }
    IniDictionary z = file.get("redirects");
    for(auto i : z) {
        local_redirect[i.first] = (std::string)i.second;
    }
}

void delete_localconf() {
    std::filesystem::remove_all(CATCARE_HOME);
}

void write_localconf() {
    IniFile file = IniFile::from_file(CATCARE_CONFIGFILE);
    IniDictionary s = file.get("options");
    for(auto i : options) {
        IniElement elm;
        elm = i.second;
        s[i.first] = elm;
    }
    file.set("options",s);
    IniDictionary z = file.get("redirects");
    for(auto i : local_redirect) {
        IniElement elm;
        elm = i.second;
        z[i.first] = elm;
    }
    file.set("redirects",z);
    file.to_file(CATCARE_CONFIGFILE);
}

IniDictionary extract_configs(std::string name) {
    IniDictionary ret;

    IniFile file = IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    if(!file) {
        std::cout << file.error_msg() << "\n";
        return ret;
    }


    if(!file || !file.has("name","Info") || !file.has("files","Download")) {
        return ret;
    }

    ret["name"] = file.get("name","Info");
    ret["files"] = file.get("files","Download");
    if(file.has("version","Info"))
        ret["version"] = file.get("version","Info");
    if(file.has("dependencies","Download"))
        ret["dependencies"] = file.get("dependencies","Download");
    return ret;
}

bool valid_configs(IniDictionary conf) {
    return config_healthcare(conf) == "";
}

std::string config_healthcare(IniDictionary conf) {
    if(conf["name"].getType() != IniType::String) {
        return "\"name\" must be a string!";
    }
    if(conf.count("dependencies") != 0 && conf["dependencies"].getType() != IniType::List) {
        return "\"dependencies\" must be a list!";
    }
    if(conf["files"].getType() != IniType::List) {
        return "\"files\" must be a list!";
    }
    if(conf.count("version") != 0 && conf["version"].getType() != IniType::String) {
        return "\"version\" must be a string!";
    }
    return "";
}

void make_register() {
    if(!std::filesystem::exists(CATCARE_ROOT)) {
        std::filesystem::create_directory(CATCARE_ROOT);
    }
    if(!std::filesystem::exists(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME)) {
        std::ofstream os;
        os.open(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME,std::ios::app);
        os << "installed=[]\n";
        os.close();
    }
}

IniList get_register() {
    make_register();
    IniFile reg = IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
    return reg.get("installed").to_list();
}

bool installed(std::string name) {
    IniList lst = get_register();
    for(auto i : lst) {
        if(i.getType() == IniType::String && name == (std::string)i) {
            return true;
        }
    }
    return false;
}

void add_to_register(std::string name) {
    if(installed(name)) { return; }
    IniFile reg = IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
    IniList l = reg.get("installed").to_list();
    l.push_back((std::string)("\"" + name + "\""));
    reg.set("installed",l);
    reg.to_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
}

void remove_from_register(std::string name) {
    if(!installed(name)) { return; }
    IniFile reg = IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
    IniList l = reg.get("installed").to_list();
    for(size_t i = 0; i < l.size(); ++i) {
        if(l[i].getType() == IniType::String && (std::string)l[i] == name) {
            l.erase(l.begin()+i);
            break;
        }
    }
    reg.set("installed",l);
    reg.to_file(CATCARE_ROOT + CATCARE_DIRSLASH CATCARE_REGISTERNAME);
}

bool is_dependency(std::string name) {
    IniList lst = get_dependencylist();
    for(auto i : lst) {
        if(i.getType() == IniType::String && name == (std::string)i) {
            return true;
        }
    }
    return false;
}

void add_to_dependencylist(std::string name) {
    IniFile reg = IniFile::from_file(CATCARE_CHECKLISTNAME);
    IniList l = reg.get("dependencies","Download").to_list();
    for(size_t i = 0; i < l.size(); ++i) {
        if(l[i].getType() == IniType::String && (std::string)l[i] == name) {
            return;
        }
    }
    IniElement elm;
    elm = name;
    l.push_back(elm);
    reg.set("dependencies",l,"Download");
    reg.to_file(CATCARE_CHECKLISTNAME);
}

void remove_from_dependencylist(std::string name) {
    IniFile reg = IniFile::from_file(CATCARE_CHECKLISTNAME);
    IniList l = reg.get("dependencies","Download").to_list();
    for(size_t i = 0; i < l.size(); ++i) {
        if(l[i].getType() == IniType::String && (std::string)l[i] == name) {
            l.erase(l.begin()+i);
            break;
        }
    }
    reg.set("dependencies",l,"Download");
    reg.to_file(CATCARE_CHECKLISTNAME);
}

IniList get_dependencylist() {
    IniFile reg = IniFile::from_file(CATCARE_CHECKLISTNAME);
    return reg.get("dependencies","Download").to_list();
}

void make_checklist() {
    if(!std::filesystem::exists(CATCARE_CHECKLISTNAME)) {
        make_file(CATCARE_CHECKLISTNAME,"[Info]\nname=\"\"\n\n[Download]\ndependencies=[]\nfiles=[]\n");
    }
}