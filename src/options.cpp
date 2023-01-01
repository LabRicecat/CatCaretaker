#include "../inc/options.hpp"
#include "../inc/network.hpp"
#include "../inc/configs.hpp"

bool is_redirected(std::string name) {
    return local_redirect.count(name) > 0;
}
void make_redirect(std::string name, std::string path) {
    if(is_redirected(name)) {
        if(!confirmation("redefine the aleady existing redirect for \"" + name + "\" ?")) {
            return;
        }
    }
    local_redirect[name] = path;
}


void print_message(std::string mod, std::string message) {
    if(!opt_silence)
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

std::string option_or(std::string option, std::string els) {
    if(options.count(option) > 0) {
        return options[option];
    }
    return els;
}

std::string app_username(std::string str) {
    auto [usr,name] = get_username(str);
    if(usr != "") {
        return str;
    }
    return to_lowercase(CATCARE_USER) + "/" + str;
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

std::tuple<std::string,std::string> get_username(std::string str) {
    while(!str.empty() && str.back() == '/') str.pop_back();
    
    std::string ret;
    std::string el;
    bool sw = false;
    for(auto i : str) {
        if(i == '/') {
            sw = true;
            el += ret + "/";
        }
        else if(sw) {
            ret += i;
        }
        else {
            el += i;
        }
    }
    return std::make_tuple(el,ret);
}

#define TYPE_CHECK(entry, type) if(entry.get_type() != type)
#define MUST_HAVE(entry, member) if(entry.count(member) == 0)

void print_entry(IniDictionary current) {
    std::cout << "Description: " << current["description"] << "\n"
        << "Written for language: " << current["language"] << "\n"
        << "By " << current["author"];
}

bool browse(std::string file) {
    IniFile inifile = IniFile::from_file(file);

    if(!inifile || !inifile.has("browsing","Main")) return false;
    IniElement browsing_e = inifile.get("browsing");
    TYPE_CHECK(browsing_e,IniType::Dictionary) return false;

    IniDictionary browsing = browsing_e.to_dictionary();

    std::vector<IniDictionary> entries;

    for(auto i : browsing) {
        TYPE_CHECK(i.second,IniType::Dictionary) continue;
        IniDictionary entry = i.second.to_dictionary();
        MUST_HAVE(entry,"description") continue;
        TYPE_CHECK(entry["description"],IniType::String) continue;
        MUST_HAVE(entry,"language") continue;
        TYPE_CHECK(entry["language"],IniType::String) continue;
        MUST_HAVE(entry,"author") continue;
        TYPE_CHECK(entry["author"],IniType::String) continue;
        entry["name"] = i.first;
        entries.push_back(entry);
    }

    if(entries.empty()) return false;
    
    int current = 0;
    bool exit = false;
    while(!exit) {
        cat_clsscreen();

        std::cout << ">> "<< entries[current]["name"] << " <<\n";
        print_entry(entries[current]);

        std::cout << "\n\n== ";
        if(current != entries.size()-1) {
            std::cout << "[n]ext, ";
        }
        if(current != 0) {
            std::cout << "[p]revious, ";
        }

        std::cout << "[i]nstall, [e]xit\n= ";
        std::string inp;
        std::getline(std::cin,inp);
        if((inp == "n" || inp == "N" || inp == "next") && current != entries.size()-1) {
            ++current;
        }
        else if((inp == "p" || inp == "P" || inp == "previous") && current != 0) {
            --current;
        }
        else if(inp == "i" || inp == "I" || inp == "install") {
            std::string repo = (std::string)entries[current]["author"] + "/" + (std::string)entries[current]["name"];
            repo = to_lowercase(repo);
            std::string error = download_repo(repo);

            if(error != "")
                print_message("ERROR","Error downloading repo: \"" + repo + "\"\n-> " + error);
            else {
                print_message("RESULT","Successfully installed!");
                if(!is_dependency(repo)) {
                    add_to_dependencylist(repo);
                }
            }
            cat_sleep(1000);
        }
        else if(inp == "e" || inp == "E" || inp == "exit") {
            exit = true;
        }
    }
    return true;
}