#include "../inc/options.hpp"
#include "../inc/network.hpp"
#include "../inc/configs.hpp"
#include <set>

void print_message(std::string mod, std::string message) {
    if(!arg_settings::opt_silence)
        std::cout << "[" << mod << "] " << message << "\n";
}

bool confirmation(std::string context) {
    std::cout << "Do you really want to " << context << " [y/N]:";
    std::flush(std::cout);
    std::string inp;
    std::getline(std::cin,inp);

    return(inp == "y" || inp == "Y");
}

std::string ask_and_default(std::string defau) {
    std::string inp;
    std::flush(std::cout);
    std::getline(std::cin,inp);
    if(inp == "") {
        return defau;
    }
    return inp;
}

void await_continue() {
    std::cout << "(Enter to continue...)\r";
    std::flush(std::cout);
    std::string s;
    std::getline(std::cin,s);
    cat_clsscreen();
}

std::string option_or(std::string option, std::string els) {
    if(options.count(option) > 0) {
        return options[option];
    }
    return els;
}

std::string last_name(std::filesystem::path path) {
    std::string ret;
    for(size_t i = path.string().size()-1; i >= 0; --i) {
        if(path.string()[i] == CATCARE_DIRSLASHc && ret != "")
            break;
        else if(path.string()[i] != CATCARE_DIRSLASHc);
            ret.insert(ret.begin(),path.string()[i]);
    }
    return ret;
}

std::string to_lowercase(std::string str) {
    std::string ret;
    for(auto i : str) if(isupper(i)) ret += tolower(i); else ret += i;
    return ret;
}

#define TYPE_CHECK(entry, type) if(entry.get_type() != type)
#define MUST_HAVE(entry, member) if(entry.count(member) == 0)

void print_entry(IniDictionary config) {
    if(config.count("description") != 0) std::cout << (std::string)config["description"] << "\n";
    if(config.count("version") != 0) std::cout << "Version: " << (std::string)config["version"] << "\n";
    if(config.count("license") != 0) std::cout << "License: " << (std::string)config["license"] << "\n";
    if(config.count("documentation") != 0) std::cout << "Documentation: " << (std::string)config["documentation"] << "\n";
    if(config.count("tags") != 0 && config["authors"].to_list().size() != 0) {
        std::cout << "Authors: \n";
        for(auto i : config["authors"].to_list()) {
            std::cout << " - " << (std::string)i << "\n";
        }
    }
    if(config.count("authors") != 0 && config["authors"].to_list().size() != 0) {
        std::cout << "Authors: \n";
        IniList list = config["authors"].to_list();
        for(size_t i = 0; i < list.size(); ++i) {
            if(i != 0) std::cout << ", ";
            std::cout << (std::string)list[i];
        }
        std::cout << "\n";
    }
    if(config.count("tags") != 0 && config["tags"].to_list().size() != 0) {
       std::cout << "Tags: \n";
        IniList list = config["tags"].to_list();
        for(size_t i = 0; i < list.size(); ++i) {
            if(i != 0) std::cout << ", ";
            std::cout << (std::string)list[i];
        }
        std::cout << "\n";
    }
    if(config.count("scripts") != 0 && config["scripts"].to_list().size() != 0) {
       std::cout << "Scripts: \n";
        IniList list = config["scripts"].to_list();
        for(size_t i = 0; i < list.size(); ++i) {
            if(i != 0) std::cout << ", ";
            std::cout << (std::string)list[i];
        }
        std::cout << "\n";
    }
    if(config.count("dependencies") != 0 && config["dependencies"].to_list().size() != 0) {
        std::cout << "Dependencies: \n";
        for(auto i : config["dependencies"].to_list()) {
            std::cout << " - " << (std::string)i << "\n";
        }
    }
    std::cout << "Installed: " << (installed((std::string)config["__url"]) ? "Yes" : "No") << "\n";
}

void collect_from(std::vector<IniDictionary>& src, IniFile file, std::string name, std::set<std::string>& used) {
    if(used.contains(name)) return;
    used.insert(name);
    if(!file || !file.has("browsing","Main")) return;
    IniList browsing = file.get("browsing").to_list();

    for(size_t i = 0; i < browsing.size(); ++i) {
        if(browsing[i].get_type() == IniType::String) {
            std::cout << "\rCollecting entries... " << i << " of " << browsing.size() << " from " << name;
            std::flush(std::cout);
            IniDictionary entry = extract_configs(download_checklist((std::string)browsing[i]));
            if(entry.empty()) continue;
            entry["__url"] = (std::string)browsing[i];
            src.push_back(entry);
        }
    }

    if(file.has("include","Main")) {
        IniList include = file.get("include","Main").to_list();
        for(auto i : include) {
            if(!download_page((std::string)i,CATCARE_TMP_PATH CATCARE_DIRSLASH CATCARE_BROWSING_FILE "-" + name)) continue;
            IniFile f = IniFile::from_file(CATCARE_TMP_PATH CATCARE_DIRSLASH CATCARE_BROWSING_FILE "-" + name);
            std::filesystem::remove_all(CATCARE_TMP_PATH CATCARE_DIRSLASH CATCARE_BROWSING_FILE "-" + name);
            collect_from(src,f,(std::string)i,used);
        }
    }
}

bool browse(std::string file, std::string url) {
    IniFile inifile = IniFile::from_file(file);

    std::vector<IniDictionary> entries;
    std::set<std::string> used;
    collect_from(entries,inifile,url,used);
    std::cout << "\n";

    if(entries.empty()) return false;
    
    int current = 0;
    bool exit = false;
    while(!exit) {
        cat_clsscreen();

        std::string link = (std::string)entries[current]["__url"];

        std::cout << ">>> "<< entries[current]["name"] << " (" << (current+1) << " of " << entries.size() << ") <<<\n";
        print_entry(entries[current]);

        std::cout << "\n\n== ";
        if(current != entries.size()-1) {
            std::cout << "[n]ext, ";
        }
        if(current != 0) {
            std::cout << "[p]revious, ";
        }
        if(installed(link)) {
            std::cout << "[u]ninstall, ";
        }
        else {
            std::cout << "[i]nstall, ";
        }

        std::cout << "[e]xit\n=> ";
        std::string inp;
        std::getline(std::cin,inp);
        if((inp == "n" || inp == "N" || inp == "next") && current != entries.size()-1) {
            ++current;
        }
        else if((inp == "p" || inp == "P" || inp == "previous") && current != 0) {
            --current;
        }
        else if((inp == "i" || inp == "I" || inp == "install") && !installed(link)) {
            std::string error = download_project(link);

            if(error != "")
                print_message("ERROR","Error while downloading project: \"" + (std::string)entries[current]["name"] + "\"\n-> " + error);
            else {
                print_message("RESULT","Successfully installed!");
                if(!is_dependency(link)) {
                    add_to_dependencylist(link);
                }
            }
            cat_sleep(1000);
        }
        else if((inp == "u" || inp == "U" || inp == "uninstall") && installed(link)) {
            std::string name = to_lowercase(entries[current]["name"]);
            std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
            remove_from_register(link);
            remove_from_dependencylist(link);
            print_message("DELETE","Removed project: \"" + (std::string)entries[current]["name"] + "\"");
            cat_sleep(1000);
        }
        else if(inp == "e" || inp == "E" || inp == "exit") {
            exit = true;
        }
    }
    return true;
}