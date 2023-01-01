#include "../inc/network.hpp"
#include "../inc/configs.hpp"
#include "../inc/options.hpp"

#ifdef __linux__
#include <curl/curl.h>
#include <unistd.h>
#include <pwd.h>

bool download_page(std::string url, std::string file) {
    // std::cout << "Downloading " << url << "\n";
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
        fclose(fp);
        if(res != 0) {
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


void download_dependencies(IniList list) {
    for(auto i : list) {
        if(i.getType() == IniType::String && !installed((std::string)i)) {
            std::string is = i;
            if(is.size() > 2 && is[0] == '.' && is[1] == CATCARE_DIRSLASHc) {
                print_message("COPYING","Copying local dependency: \"" + is + "\"");
                std::string error = get_local(
                    last_name(is),
                    std::filesystem::path(is).parent_path().string());
                if(error != "")
                    print_message("ERROR","Error copying local repo: \"" + is + "\"\n-> " + error);
            }
            else {
                print_message("DOWNLOAD","Downloading dependency: \"" + (std::string)i + "\"");
                std::string error = download_repo((std::string)i);
                if(error != "")
                    print_message("ERROR","Error downloading repo: \"" + (std::string)i + "\"\n-> " + error);
            }
        }
    }
}

#define CLEAR_ON_ERR() if(option_or("clear_on_error","true") == "true") {std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);}

std::string download_repo(std::string install) {
    make_register();
    make_checklist();
    install = to_lowercase(install);
    auto [usr,name] = get_username(install);
    if(usr == "") usr = CATCARE_USER;
    install = app_username(install);
    if(std::filesystem::exists(CATCARE_ROOT + CATCARE_DIRSLASH + name)) {
        std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
    }
    std::filesystem::create_directories(CATCARE_ROOT + CATCARE_DIRSLASH + name);
    std::filesystem::create_symlink(".." CATCARE_DIRSLASH ".." CATCARE_DIRSLASH + CATCARE_ROOT, CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + CATCARE_ROOT);
    if(!download_page(CATCARE_REPOFILE(install,CATCARE_CHECKLISTNAME),CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH CATCARE_CHECKLISTNAME)) {
        CLEAR_ON_ERR()
        return "Could not download checklist!";
    }
    IniDictionary conf = extract_configs(name);
    if(!valid_configs(conf)) {
        CLEAR_ON_ERR()
        return config_healthcare(conf);
    }
    IniList files = conf["files"].to_list();

    for(auto i : files) {
        if(i.getType() != IniType::String) {
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
            if(!download_page(CATCARE_REPOFILE(install,file),CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + ufile)) {
                CLEAR_ON_ERR()
                return "Error downloading file: " + ufile;
            }
        }
        else {
            print_message("DOWNLOAD","Downloading file: " + file);
            if(!download_page(CATCARE_REPOFILE(install,file),CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + file)) {
                CLEAR_ON_ERR()
                return "Error downloading file: " + file;
            }
        }
    }

    if(!installed(install)) {
        add_to_register(install);
    }

    download_dependencies(conf["dependencies"].to_list());

    return "";
}

std::string get_local(std::string name, std::string path) {
    make_register();
    make_checklist();
    name = to_lowercase(name);
    if(std::filesystem::exists(CATCARE_ROOT + CATCARE_DIRSLASH + name)) {
        std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
    }
    std::filesystem::create_directories(CATCARE_ROOT + CATCARE_DIRSLASH + name);
    
    try {
        std::filesystem::copy(path + CATCARE_DIRSLASH + CATCARE_CHECKLISTNAME,CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    }
    catch(...) {
        std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
        return "Could not copy checklist!";
    }
    IniDictionary conf = extract_configs(name);
    if(!valid_configs(conf)) {
        std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
        return config_healthcare(conf);
    }
    IniList files = conf["files"].to_list();

    for(auto i : files) {
        if(i.getType() != IniType::String) {
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
            print_message("COPYING","Adding dictionary: " + file);
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
            print_message("COPYING","Copying file: " + file);
            try {
                std::filesystem::copy(path + CATCARE_DIRSLASH + file,CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + ufile);
            }
            catch(...) {
                std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
                return "Error downloading file: " + file;
            }
        }
        else {
            print_message("COPYING","Downloading file: " + file);
            try {
                std::filesystem::copy(path + CATCARE_DIRSLASH + file,CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH + file);
            }
            catch(...) {
                std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
                return "Error downloading file: " + file;
            }
        }
    }

    if(!installed(name)) {
        add_to_register(name);
    }
    for(auto i : conf["dependencies"].to_list()) {
        if(i.getType() == IniType::String && !installed((std::string)i)) {
            print_message("COPYING","Downloading dependency: \"" + (std::string)i + "\"");
            std::string error;
            if(option_or("local_over_url","false") == "true") {
                if(is_redirected((std::string)i)) {
                    error = get_local((std::string)i,local_redirect[(std::string)i]);
                }
                else {
                    error = get_local(last_name((std::string)i),std::filesystem::path((std::string)i).parent_path().string());
                }
                if(error != "") {
                    error = download_repo((std::string)i);
                }
            }
            else {
                error = download_repo((std::string)i);
                if(error != "") {
                    if(is_redirected((std::string)i)) {
                        error = get_local((std::string)i,local_redirect[(std::string)i]);
                    }
                    else {
                        error = get_local(last_name((std::string)i),std::filesystem::path((std::string)i).parent_path().string());
                    }
                }
            }

            if(error != "")
                print_message("ERROR","Error copying project: \"" + (std::string)i + "\"\n-> " + error);
        }
    }

    // download_dependencies(conf["dependencies"].to_list());

    return "";
}

#define REMOVE_TMP() std::filesystem::remove_all(CATCARE_TMPDIR)
#define RETURN_TUP(a,b) return std::make_tuple<std::string,std::string>(a,b)

std::tuple<std::string,std::string> needs_update(std::string name) {
    name = app_username(name);
    if(!installed(name)) RETURN_TUP("","");
    auto [usr,proj] = get_username(name);

    std::filesystem::create_directories(CATCARE_TMPDIR);
    download_page(CATCARE_REPOFILE(name,CATCARE_CHECKLISTNAME),CATCARE_TMPDIR CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    IniFile r = IniFile::from_file(CATCARE_TMPDIR CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    if(!r || !r.has("version","Info")) {
        REMOVE_TMP(); RETURN_TUP("","");
    }

    auto newest_version = r.get("version","Info");
    if(newest_version.getType() != IniType::String) { REMOVE_TMP(); RETURN_TUP("",""); }

    r = IniFile::from_file(CATCARE_ROOT + CATCARE_DIRSLASH + proj + CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
    if(!r || !r.has("version","Info")) { REMOVE_TMP(); RETURN_TUP(newest_version.to_string(),"???"); }

    auto current_version = r.get("version","Info");
    if(current_version.getType() != IniType::String) {
        REMOVE_TMP(); RETURN_TUP(newest_version.to_string(),"???");
    }
    
    if(newest_version.to_string() != current_version.to_string()) {
        REMOVE_TMP(); RETURN_TUP(newest_version.to_string(),current_version.to_string());
    }

    REMOVE_TMP();
    RETURN_TUP("","");
}

#undef REMOVE_TMP
#undef RETURN_TUP