#include "../carescript/carescript-api.hpp"
#include "network.hpp"
#include "configs.hpp"

class CCSExtension : public carescript::Extension {   
public:
    virtual carescript::BuiltinList get_builtins() override {
        using namespace carescript;
        return {
            {"download",{2,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);
                cc_builtin_var_requires(args[1],ScriptStringValue);
                if(!download_page(
                    get_value<ScriptStringValue>(args[0]),
                    get_value<ScriptStringValue>(args[1])
                )) {
                    return script_false; /*="error while downloading from URL: " + get_value<ScriptStringValue>(args[0]); */
                }
                return script_true;
            }}},
            
            {"add_file",{2,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);
                cc_builtin_var_requires(args[1],ScriptStringValue);
                std::ofstream of(get_value<ScriptStringValue>(args[0]), std::ios::trunc);
                of.close();
                of.open(get_value<ScriptStringValue>(args[0]));
                of << get_value<ScriptStringValue>(args[1]);
                of.close();
                return script_null;
            }}},
            {"add_directory",{1,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);
                std::filesystem::create_directory(get_value<ScriptStringValue>(args[0]));
                return script_null;
            }}},
            {"exists",{1,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);
                return std::filesystem::exists(get_value<ScriptStringValue>(args[0])) ? script_true : script_false;
            }}},
            {"remove",{1,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);
                return std::filesystem::remove_all(get_value<ScriptStringValue>(args[0])) ? script_true : script_false;
            }}},
            {"copy",{2,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);
                cc_builtin_var_requires(args[1],ScriptStringValue);
                try {
                    std::filesystem::copy(get_value<ScriptStringValue>(args[0]),get_value<ScriptStringValue>(args[1]));
                }
                catch(...) { return script_false; }
                return script_true;
            }}},
            {"move",{2,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);
                cc_builtin_var_requires(args[1],ScriptStringValue);
                try {
                    std::filesystem::copy(get_value<ScriptStringValue>(args[0]),get_value<ScriptStringValue>(args[1]));
                    std::filesystem::remove_all(get_value<ScriptStringValue>(args[0]));
                }
                catch(...) { return script_false; }
                return script_true;
            }}},
            
            {"installed",{1,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);
                return installed(get_value<ScriptStringValue>(args[0])) ? script_true : script_false;
            }}},
            {"add_project",{1,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);
                settings.error_msg = download_project(get_value<ScriptStringValue>(args[0]));
                return script_null;
            }}},
            {"resolve",{1,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);
                auto urls = get_download_url(to_lowercase(get_value<ScriptStringValue>(args[0])));
                if(urls.size() > 1) {
                    if(urls.size() == 0) {
                        return "";
                    }
                    else if(urls.size() > 1) {
                        std::cout << "More than one potential resolve, please select:\n";
                        for(size_t i = 0; i < urls.size(); ++i) {
                            std::cout << "[" << (i+1) << "] " << urls[i].rule.name << ": \t\t " << urls[i].link << "\n";
                        }
                        std::cout << "... or [e]xit\n=> ";
                        std::string inp;
                        std::getline(std::cin,inp);
                        if(inp == "e" || inp == "exit") return {};
                        try {
                            int select = std::stoi(inp);
                            if(select < 1 || select > urls.size()) { std::cout << "Invalid index!\n"; return {}; }
                            urls = {urls[select-1]};
                        }
                        catch(...) {
                            return "";
                        }
                    }
                }
                if(urls.size() != 1) {
                    return "";
                }
                return urls[0].link;
            }}},
            {"download_project",{2,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);
                cc_builtin_var_requires(args[1],ScriptStringValue);
                
                std::string url = get_value<ScriptStringValue>(args[0]);
                auto current = std::filesystem::current_path();
                std::filesystem::create_directory(get_value<ScriptStringValue>(args[1]));
                std::filesystem::current_path(current / get_value<ScriptStringValue>(args[1]));

                std::string status = download_project(url);
                if(status != "") {
                    std::filesystem::current_path(current);
                    std::filesystem::remove_all(current / get_value<ScriptStringValue>(args[1]));
                    return script_false;
                }

                IniDictionary regist = get_register();
                std::string name = "";
                for(auto i : regist) {
                    if((std::string)i.second == url) {
                        name = i.first;
                        break;
                    }
                }
                if(name == "") {
                    std::filesystem::current_path(current);
                    std::filesystem::remove_all(current / get_value<ScriptStringValue>(args[1]));
                    return script_false;
                }
                std::filesystem::remove_all(CATCARE_CHECKLISTNAME);
                for(auto p : std::filesystem::directory_iterator(CATCARE_ROOT + CATCARE_DIRSLASH + name)) {
                    if(p.is_symlink() && p.path().filename() == CATCARE_ROOT_NAME) continue;
                    if(std::filesystem::exists(p.path().filename())) std::filesystem::remove_all(p.path().filename());
                    std::filesystem::copy(p.path(),p.path().filename());
                }
                std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
                remove_from_register(name);

                std::filesystem::current_path(current);
                return script_true;
            }}},

            {"get_config",{1,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);

                return options[get_value<ScriptStringValue>(args[0])];
            }}},
            {"set_config",{2,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);
                cc_builtin_var_requires(args[1],ScriptStringValue);

                return options[get_value<ScriptStringValue>(args[0])] = get_value<ScriptStringValue>(args[0]);
            }}},
        
            {"checklist",{2,[](const ScriptArglist& args, ScriptSettings& settings)->ScriptVariable {
                cc_builtin_if_ignore();
                cc_builtin_var_requires(args[0],ScriptStringValue);
                cc_builtin_var_requires(args[1],ScriptStringValue);
                IniFile checklist = IniFile::from_file(get_value<ScriptStringValue>(args[0]));
                if(!checklist) { settings.error_msg = checklist.error_msg(); return script_null; }
                std::string option = get_value<ScriptStringValue>(args[1]);
                std::string ret = (std::string)checklist.get(option,"Info");
                if(!checklist) { settings.error_msg = checklist.error_msg(); return script_null; }
                return ret;
            }}},
        };
    }
    virtual carescript::OperatorList get_operators() override {
        return {};
    }
    virtual carescript::MacroList get_macros() override {
        return {
            {"ROOT","\"" + CATCARE_ROOT + "\""},
            {"MACRO_DIR","\"" CATCARE_MACRO_PATH "\""},
            {"EXTENSION_DIR","\"" CATCARE_EXTENSION_PATH "\""},
            {"ATTACHMENT_DIR","\"" CATCARE_ATTACHMENT_PATH "\""},
            {"HOME","\"" CATCARE_HOME "\""},
            {"CONFIG","\"" CATCARE_CONFIG_FILE "\""},
            {"CHECKLIST","\"" CATCARE_CHECKLISTNAME "\""},
            {"PROGNAME","\"" CATCARE_PROGNAME "\""}
        };
    }
    virtual carescript::TypeList get_types() override {
        return {};
    }

};

CARESCRIPT_EXTENSION_GETEXT_INLINE(
    return new CCSExtension();
)