#include "../inc/network.hpp"
#include "../inc/configs.hpp"
#include "../inc/options.hpp"
#include "../inc/pagelist.hpp"
#include "../inc/catcaretaker-ccs-extension.hpp"

#include "../carescript/carescript-api.hpp"

using namespace carescript;

#ifdef __linux__
#include <curl/curl.h>
#include <unistd.h>
#include <pwd.h>

bool download_page(std::string url, std::string file) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    const char* aurl = url.c_str();
    const char* outfilename = file.c_str();
    curl = curl_easy_init();                                                                                                                                                                                                                                                           
    if (curl) {   
        fp = fopen(outfilename,"wb");
        curl_easy_setopt(curl, CURLOPT_URL, aurl);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        fclose(fp);

        if(res != 0 || (http_code >= 400 && http_code <= 599)) {
            std::filesystem::remove_all(file);
            return false;
        }
        return true;
    }   
    return false;
}

std::string get_username() {
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        return std::string(pw->pw_name);
    }
    return "";
}

#elif defined(_WIN32)
#include <windows.h>
#include <Lcmd.h>
#include <codecvt>

bool download_page(std::string url, std::string file) {
    const wchar_t* srcURL = std::wstring(url.begin(),url.end()).c_str();
    const wchar_t* destFile = std::wstring(file.begin(),file.end()).c_str();

    if (S_OK == URLDownloadToFile(NULL, srcURL, destFile, 0, NULL)) {
        return 0;
    }
    return 1;
}

std::string get_username() {
    TCHAR username[UNLEN+1];
    DWORD size = UNLEN +1;
    GetUserName((TCHAR*)username,&size);

#ifdef UNICODE
    std::wstring tmp(username,size-1);
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX,wchar_t> converterX;

    return converterX.to_bytes(tmp);
#else 
    std::string tmp(username,size-1);
    return tmp;
#endif
}

#else
std::string get_username() {}
bool download_page(std::string url, std::string file) {}
#endif

void fill_global_pagelist() {
    std::ifstream iff(CATCARE_URLRULES_FILE);
    std::string source = "";
    while(iff.good()) source += iff.get();
    if(source != "") source.pop_back();

    global_rulelist = process_rulelist(source);
}

std::vector<UrlPackage> get_download_url(std::string input) {
    if(global_rulelist.empty())
        fill_global_pagelist();

    return find_url(global_rulelist,to_lowercase(input));
}
std::string app2url(std::string url, std::string app) {
    if(url != "" && url.back() != '/') url += "/";
    return url += app; 
}

void download_dependencies(IniList list) {
    for(auto i : list) {
        if(i.get_type() == IniType::String && !installed((std::string)i)) {
            std::string is = i;
            print_message("DOWNLOAD","Downloading dependency: \"" + (std::string)i + "\"");
            std::string error = download_project((std::string)i);
            if(error != "")
                print_message("ERROR","Error while downloading dependency: \"" + (std::string)i + "\"\n-> " + error);
        }
    }
}

#define CLEAR_ON_ERR() if(option_or("clear_on_error","true") == "true") {std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);}
#define IFERR(interp) if(!interp) { CLEAR_ON_ERR(); return interp.error(); }

bool download_scripts(IniList scripts,std::string install_url, std::string name) {
    Interpreter interpreter;
    bake_extension(get_extension(),interpreter.settings);
    load_extensions(interpreter);

    for(auto i : scripts) {
        if(i.get_type() == IniType::String) {
            print_message("DOWNLOAD","Downloading script: " + (std::string)i);
            if(!download_page(install_url + (std::string)i,CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + (std::string)i)) {
                print_message("ERROR","Failed to download script!");
                continue;
            }
            std::string source;
            std::ifstream ifile(CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + (std::string)i);
            while(ifile.good()) source += ifile.get();
            if(source != "") source.pop_back();

            if(option_or("show_script_src","false") == "true") {
                KittenLexer line_lexer = KittenLexer()
                    .add_linebreak('\n');
                auto lexed = line_lexer.lex(source);
                std::cout << "> Q to exit, S to stop download, enter to continue\n";
                std::cout << "================> Script " << (std::string)i << " | lines: " << lexed.back().line << "\n";
                std::string inp;
                for(size_t i = 0; i < lexed.size(); ++i) {
                    std::cout << lexed[i].line << ": " << lexed[i].src;
                    std::getline(std::cin,inp);
                    if(inp == "q" || inp == "Q" || inp == "s" || inp == "S") break;
                    else if(inp == "a" || inp == "A") {
                        for(; i < lexed.size(); ++i) {
                            std::cout << lexed[i].line << ": " << lexed[i].src;
                        }
                        break;
                    }
                }
                if(inp == "s" || inp == "S") {
                    print_message("INFO","\nStopping download");
                    CLEAR_ON_ERR();
                    return true;
                }
                std::cout << "================> Enter to continue...";
                std::getline(std::cin,inp);
            }

            print_message("INFO","Entering CCScript: \"" + (std::string)i + "\"");
            interpreter.pre_process(source).on_error([&](Interpreter& i) {
                print_message("ERROR","Script failed:\n" + i.error());
            });
            interpreter.run().on_error([&](Interpreter& i) {
                print_message("ERROR","Script failed:\n" + i.error());
            });
        }
    }
    return false;
}

std::string download_project(std::string install_url) {
    make_register();
    if(!arg_settings::global) make_checklist();
    
    if(blacklisted(install_url)) {
        return "This project is blacklisted!";
    }
    if(install_url == "") {
        return "Could not resolve url key " + install_url;
    }

    std::filesystem::create_directory(CATCARE_ROOT + CATCARE_DIRSLASH + "__tmp");
    if(!download_page(install_url + CATCARE_CHECKLISTNAME,CATCARE_ROOT + CATCARE_DIRSLASH + "__tmp" + CATCARE_DIRSLASH CATCARE_CHECKLISTNAME)) {
        std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + "__tmp");
        return "Could not download checklist!";
    }
    IniFile checklist = IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH + "__tmp" + CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    if(!checklist) {
        // std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + "__tmp");
        return "Error in checklist: " + checklist.error_msg();
    }

    IniDictionary configs = extract_configs(checklist);
    if(!valid_configs(configs)) {
        std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + "__tmp");
        return config_healthcare(configs);
    }
    std::string name = to_lowercase((std::string)configs["name"]);

    if(std::filesystem::exists(CATCARE_ROOT + CATCARE_DIRSLASH + name)) {
        std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
    }
    std::filesystem::create_directories(CATCARE_ROOT + CATCARE_DIRSLASH + name);
    std::filesystem::create_symlink(".." CATCARE_DIRSLASH ".." CATCARE_DIRSLASH + CATCARE_ROOT_NAME, CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + CATCARE_ROOT_NAME);
    std::filesystem::copy(CATCARE_ROOT + CATCARE_DIRSLASH + "__tmp" + CATCARE_DIRSLASH CATCARE_CHECKLISTNAME, CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + "__tmp");
   
    IniList files = configs["files"].to_list();

    for(auto i : files) {
        if(i.get_type() != IniType::String) {
            continue;
        }
        std::string file = (std::string)i;
        if(file.size() == 0) {
            continue;
        }

        if(file[0] == '$') {
            file.erase(file.begin());
            if(file.empty()) {
                continue;
            }
            print_message("DOWNLOAD","Adding dictionary: " + file);
            std::filesystem::create_directory(CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + file);
            continue;
        }
        else if(file[0] == '#') {
            continue;
        }
        else if(file[0] == '!') {
            file.erase(file.begin());
            if(file.empty()) {
                continue;
            }
            std::string ufile = last_name(file);
            print_message("DOWNLOAD","Downloading file: " + ufile);
            if(!download_page(install_url + file,CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + ufile)) {
                CLEAR_ON_ERR()
                return "Error downloading file: " + ufile;
            }
        }
        else {
            print_message("DOWNLOAD","Downloading file: " + file);
            if(!download_page(install_url + file,CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + file)) {
                CLEAR_ON_ERR()
                return "Error downloading file: " + file;
            }
        }
    }

    if(configs.count("scripts") != 0 && option_or("no_scripts","false") == "false") {
        IniList scripts = configs["scripts"].to_list();

        if(download_scripts(scripts,install_url,name)) {
            return "";
        }
    }

    if(!installed(install_url)) {
        add_to_register(install_url, name);
    }

    download_dependencies(configs["dependencies"].to_list());

    return "";
}

IniFile download_checklist(std::string url) {
    bool existed = std::filesystem::exists(CATCARE_TMP_PATH);
    if(!existed) std::filesystem::create_directory(CATCARE_TMP_PATH);

    if(!download_page(url + CATCARE_CHECKLISTNAME, CATCARE_TMP_PATH CATCARE_DIRSLASH "_dl_checklist")) {
        if(existed) std::filesystem::remove_all(CATCARE_TMP_PATH CATCARE_DIRSLASH "_dl_checklist");
        else std::filesystem::remove_all(CATCARE_TMP_PATH);
        return IniFile();
    }
    IniFile file = IniFile::from_file(CATCARE_TMP_PATH CATCARE_DIRSLASH "_dl_checklist");
    if(existed) std::filesystem::remove_all(CATCARE_TMP_PATH CATCARE_DIRSLASH "_dl_checklist");
    else std::filesystem::remove_all(CATCARE_TMP_PATH);
    return file;
}

#define REMOVE_TMP() std::filesystem::remove_all(CATCARE_TMP_PATH)
#define RETURN_TUP(a,b) return std::make_tuple<std::string,std::string>(a,b)

std::tuple<std::string,std::string> needs_update(std::string project_url) {
    if(!installed(project_url)) RETURN_TUP("","");
    std::string name = url2name(project_url);

    std::filesystem::create_directories(CATCARE_TMP_PATH);
    download_page(project_url + CATCARE_CHECKLISTNAME,CATCARE_TMP_PATH CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    IniFile r = IniFile::from_file(CATCARE_TMP_PATH CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    if(!r || !r.has("version","Info")) {
        REMOVE_TMP(); RETURN_TUP("","");
    }

    auto newest_version = r.get("version","Info");
    if(newest_version.get_type() != IniType::String) { REMOVE_TMP(); RETURN_TUP("",""); }

    r = IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    if(!r || !r.has("version","Info")) { REMOVE_TMP(); RETURN_TUP((std::string)newest_version,"???"); }

    auto current_version = r.get("version","Info");
    if(current_version.get_type() != IniType::String) {
        REMOVE_TMP(); RETURN_TUP((std::string)newest_version,"???");
    }
    
    if(newest_version.to_string() != current_version.to_string()) {
        REMOVE_TMP(); RETURN_TUP((std::string)newest_version,(std::string)current_version);
    }

    REMOVE_TMP();
    RETURN_TUP("","");
}

#undef REMOVE_TMP
#undef RETURN_TUP