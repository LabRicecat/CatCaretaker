#include "../inc/options.hpp"
#include "../inc/network.hpp"

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