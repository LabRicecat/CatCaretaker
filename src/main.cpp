#include "../inc/configs.hpp"
#include "../inc/options.hpp"
#include "../mods/ArgParser/ArgParser.h"
#include "../carescript/carescript-api.hpp"
#include "../inc/catcaretaker-ccs-extension.hpp"

#include <time.h>

void print_help() {
    std::cout << "## CatCaretaker\n"
            << "A configurable helper to reuse already made work fast and efficient from github.\n\n"
            << " catcare <option> [arguments] [flags]\n\n"
            << "option :=\n"
            << "   download|get <code>          :  downloads and sets up the project.\n"
            << "   erase|remove [.all|<proj>]   :  removes an installed project.\n"
            << "   add <path>                   :  add a file to the downloadlist\n"
            << "   cleanup                      :  removes all installed projects.\n"
            << "   info <install>               :  shows infos about the selected project.\n"
            << "   this                         :  shwos infos about the current project.\n"
            << "   option <option> to <value>   :  sets OPTION to VALUE in the config file\n"
            << "   config [explain|reset|show]  :  manages the config file.\n"
            << "   check [.all|<project>]       :  checks if for PROJECT is a new version available.\n"
            << "   browse [official|<code>]     :  view a browsing file showing of different projects.\n"
            << "   whatsnew <project>           :  read the latest patch notes of a project.\n"
            << "   guide                        :  cli walkthrough tutorial for the CatCaretaker.\n"
            << "   setup <name>                 :  sets up a checklist for a project.\n"
            << "   release                      :  set up a release note easily.\n"
            << "   template [list]              :  create a template file.\n"       
            << "   blacklist [append|pop|show]  :  blacklsit certain projects.\n"  
            << "   macro <macro> <args...>      :  runs a macro file. (.help|.list for help)\n"
            << "   sync                         :  reinstalls all the dependencies of the current project.\n\n"
            << "flags := \n"
            << "   --help|-h                  :  prints this and exits.\n"
            << "   --global|-g                :  installs into the global installation directory.\n"
            << "   --no-config                :  don't create a config directory.\n"
            << "   --silent|-s                :  prevents info and error messages.\n\n"
            << "By LabRiceCat (c) 2023\n"
            << "Repository: https://github.com/LabRiceCat/catcaretaker\n";
    std::exit(0);
}

void print_config(IniDictionary config) {
    std::cout << "Name: " << (std::string)config["name"] << "\n";
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
    if(config["files"].to_list().size() != 0) {
        bool drs = false;
        std::cout << "Files: \n";
        for(auto i : config["files"].to_list()) {
            std::string f = (std::string)i;
            if(f[0] == '!')
                f.erase(f.begin());
            else if(f[0] == '$') { drs = true; continue; }
            std::cout << " - " << f << "\n";
        }
        if(drs) {
            std::cout << "Directories: \n";
            for(auto i : config["files"].to_list()) {
                std::string f = (std::string)i;
                if(f[0] == '$')
                    f.erase(f.begin());
                else continue;
                std::cout << " - " << f << "\n";
            }
        }
    }
}

// #define DEBUG

// only_installed = 0 -> only already installed projects
// only_installed = 1 -> only not installed projects
// only_installed = 2 -> all
UrlPackage ask_to_resolve(std::string inp, int only_installed = 0) {
    auto urls = get_download_url(to_lowercase(inp));
    if(urls.size() > 1) {
        for(size_t i = 0; i < urls.size(); ++i) {
            if(only_installed != 2 && (only_installed == 0) == installed(urls[i].link)) {
                urls.erase(urls.begin()+i);
                --i;
            }
        }
        if(urls.size() == 0) {
            print_message("ERROR","No such project installed!");
            return {};
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
                std::cout << "Invalid index!\n"; 
                return {};
            }
        }
    }
    if(urls.size() != 1) {
        print_message("ERROR","Can't resolve this to any rules!");
        return {};
    }
    return urls[0];
}

int main(int argc,char** argv) {
    using namespace carescript;
    Interpreter interpreter;
    bake_extension(get_extension(),interpreter.settings);

    if(options.count("default_silent") > 0 && (options["default_silent"] == "1" || options["default_silent"] == "true")) {
        arg_settings::opt_silence = true;
    }

    ArgParser parser = ArgParser()
        .addArg("--help",ARG_TAG,{"-h"},0)
        .addArg("download",ARG_SET,{"get"},0)
        .addArg("erase",ARG_SET,{"remove"},0)
        .addArg("cleanup",ARG_TAG,{},0)
        .addArg("info",ARG_SET,{},0)
        .addArg("list",ARG_TAG,{},0)
        .addArg("this",ARG_TAG,{},0)
        .addArg("option",ARG_SET,{},0)
        .addArg("to",ARG_SET,{"is"},2)
        .addArg("config",ARG_SET,{},0)
        .addArg("setup",ARG_SET,{},0)
        .addArg("guide",ARG_TAG,{},0)
        .addArg("sync",ARG_TAG,{},0)
        .addArg("check",ARG_SET,{},0)
        .addArg("browse",ARG_SET,{},0)
        .addArg("release",ARG_TAG,{},0)
        .addArg("add",ARG_SET,{},0)
        .addArg("whatsnew",ARG_SET,{},0)
        .addArg("macro",ARG_SET,{},0)
        .addArg("template",ARG_SET,{},0)
        .addArg("blacklist",ARG_TAG,{},0)
        .addArg("append",ARG_SET,{},1)
        .addArg("pop",ARG_SET,{},1)
        .addArg("show",ARG_TAG,{},1)

        .setbin()

        .addArg("--silent",ARG_TAG,{"-s"})
        .addArg("--global",ARG_TAG,{"-g"})
        .addArg("--no-config",ARG_TAG,{})
#ifdef DEBUG
        .addArg("--debug",ARG_TAG,{"-d"})
#endif
    ;

    std::atexit([](){
        if(option_or("clear_on_error","true") == "true") {
            if(std::filesystem::exists(CATCARE_TMP_PATH)) {
                std::filesystem::remove_all(CATCARE_TMP_PATH);
            }
        }
    });

    ParsedArgs pargs = parser.parse(argv,argc);

    if(!pargs || pargs["--help"]) {
        print_help();
    }
    arg_settings::opt_silence = pargs["--silent"];
    arg_settings::no_config = pargs["--no-config"];
    arg_settings::global = pargs["--global"];

    if(!arg_settings::no_config) {
        std::string hcheck = healthcheck_localconf();
        if(hcheck != "") {
            std::cout << hcheck << "\nReset it? [y/N]: ";
            std::string inp;
            std::getline(std::cin,inp);
            if(inp == "y" || inp == "Y") {
                std::ofstream of;
                make_file(CATCARE_CONFIG_FILE);
                of.open(CATCARE_CONFIG_FILE,std::ios::app);
                of << "options = {}\nblacklist = []\n";
                of.close();
            }
            return 1;
        }
        load_localconf();
        load_extensions(interpreter);
    }

    if((pargs("append") != "" || pargs("pop") != "" || pargs["show"]) && !pargs["blacklist"] || (pargs.has_bin() && pargs("macro") == "")) {
        print_help();
    }
    else if(pargs("download") != "") {
        std::string error;
        std::string project = pargs("download");
        auto url = ask_to_resolve(project,2);
        if(url.link == "") return 1;

        bool dnl = true;
        Interpreter emb_interpreter;
        bake_extension(get_extension(),emb_interpreter.settings);
        load_extensions(emb_interpreter);
        for(auto i : url.rule.embedded) {
            dnl &= i.second != 2;
            if(i.second == 0) {
                emb_interpreter.pre_process(i.first).on_error([&](Interpreter& i) {
                    print_message("ERROR","in embed: " + i.error());
                }).otherwise([](Interpreter& i) {
                    i.run().on_error([&](Interpreter& j) {
                        print_message("ERROR","in embed: " + j.error());
                    });
                });
            }
        }

        if(url.rule.script_handle == 1 && dnl)
            error = download_project(url.link);

        if(error != "")
            print_message("ERROR","Error while downloading project!\n-> " + error);
        else {
            if(!url.rule.scripts.empty()) {
                print_message("INFO","Running attachments...");
                for(auto i : url.rule.scripts) {
                    Interpreter interp;
                    bake_extension(get_extension(),interp.settings);
                    load_extensions(interp);
                    std::ifstream f(CATCARE_ATTACHMENT_PATH CATCARE_DIRSLASH + i);
                    std::string src;
                    while(f.good()) src += f.get();
                    if(!src.empty()) src.pop_back();
                    interp.pre_process(src);
                    if(!interp) {
                        print_message("ERROR","Attachment " + i + " by rule " + url.rule.name + ": \n  " + interp.error());
                    }
                    interp.run("attachment",url.link);
                    if(!interp) {
                        print_message("ERROR","Attachment " + i + " by rule " + url.rule.name + ": \n  " + interp.error());
                    }
                }
            }
            for(auto i : url.rule.embedded) {
                if(i.second == 1) {
                    emb_interpreter.pre_process(i.first).on_error([&](Interpreter& i) {
                        print_message("ERROR","in embed: " + i.error());
                    }).otherwise([](Interpreter& i) {
                        i.run().on_error([&](Interpreter& j) {
                            print_message("ERROR","in embed: " + j.error());
                        });
                    });
                }
            }
            if(url.rule.script_handle == 0 && dnl)
                error = download_project(url.link);

            print_message("RESULT","Successfully installed!");
            if(!is_dependency(url.link)) {
                add_to_dependencylist(url.link);
            }
        }
    }
    else if(pargs("erase") != "") {
        if(pargs("erase") == ".all") {
            std::filesystem::remove_all(CATCARE_ROOT);
            IniFile f = IniFile::from_file(CATCARE_CHECKLISTNAME);
            f.set("dependencies",IniList(),"Download");
            f.to_file(CATCARE_CHECKLISTNAME);
            print_message("DELETE","Removed all dependencies!");
            make_register();
        }
        else {
            auto url = ask_to_resolve(pargs("erase"),0);
            if(url.link == "") return 1;
            std::string name = url2name(url.link);
            if(!std::filesystem::exists(CATCARE_ROOT + CATCARE_DIRSLASH + name)) {
                print_message("ERROR","No such project installed!");
                return 1;
            }
            
            std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + name);
            
            print_message("DELETE","Removed project: \"" + name + "\"");
            remove_from_register(url.link);
            remove_from_dependencylist(url.link);
        }
    }
    else if(pargs["cleanup"]) {
        if(std::filesystem::exists(CATCARE_ROOT)) {
            std::filesystem::remove_all(CATCARE_ROOT);
        }
        std::cout << "All files have been deleted.\n";
        // make_register();
    }
    else if(pargs("info") != "") {
        auto url = ask_to_resolve(pargs("info"),0);
        if(url.link == "") return 1;
        std::string name = url2name(url.link);
        if(!installed(url.link)) {
            print_message("ERROR","No such project installed!");
            return 0;
        }
        
        IniDictionary d = extract_configs(CATCARE_ROOT + CATCARE_DIRSLASH + name + CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
        print_config(d);
    }
    else if(pargs["list"]) {
        IniDictionary d = get_register();
        std::cout << "Installed projects:\n";
        for(auto i : d) {
            std::cout << " -> " << (std::string)i.first << " from " << (std::string)i.second <<"\n";
        }
    }
    else if(pargs("option") != "") {
        if(pargs("to") == "") {
            print_message("ERROR","Invalid option syntax: option|set <option> to <value>");
        }
        options[pargs("option")] = pargs("to");
        write_localconf();
    }
    else if(pargs("config") != "") {
        std::string opt = pargs("config");

        if(opt == "show") {
            IniFile file = IniFile::from_file(CATCARE_CONFIG_FILE);
            IniDictionary d = file.get("options");
            std::cout << "Global options:\n";
            for(auto i : d) {
                std::cout << i.first << "\t:\t" << (std::string)i.second << "\n";
            }
        }
        else if(opt == "reset") {
            if(confirmation("reset the config directory?")) {
                reset_localconf();
                std::cout << "Done!\n";
            }
        }
        else if(opt == "explain") {
            std::cout << "Config options:\n"
            << "default_silent  :  If true -> `--silent` will be enabled by default. (default: false)\n"
            << "clear_on_error  :  If true -> clears the downloading project if an error occurs. (default: true)\n"
            << "show_script_src :  If true -> open a little lookup when a new script gets executed. (default: false)\n"
            << "no_scripts      :  If true -> stops all scripts from executing. (Warning: not recomended, default: false)\n";

        }
        else {
            print_help();
        }
    }
    else if(pargs["guide"]) {
        int t = 1;
#define CATGUIDE_HEADER() std::cout << "========> CatCaretaker guide (" << t++ << " of 12)\n"
        cat_clsscreen();
        std::cout << "Welcome to the CatCaretaker guide.\n";
        std::cout << "Explaination:\n"
        << "-> text in [] leaves options to pick from.\n"
        << "-> text in <> is a value to be replaced by the user.\n"
        << "-> text with a $ before it must be run in the shell.\n";
        await_continue();

        CATGUIDE_HEADER();
        std::cout 
        << "The CatCaretaker is a tool to reuse already created projects to create new ones faster.\n"
        << "To download such a project from github:\n"
        << " $ catcare download <user>/<project>@<branch>\n"
        << "This will download all files and create a `" << CATCARE_ROOT << "/<project>` folder where all of them are stored.\n"
        << "To download from a different website, or change the layout, see the " << CATCARE_URLRULES_FILENAME << "\n"
        << "at " << CATCARE_URLRULES_FILE << "\n";
        await_continue();

        CATGUIDE_HEADER();
        std::cout 
        << "You can also remove a project easily by running:\n"
        << " $ catcare remove [.all|<project>]\n"
        << "By doing this, it will also be removed from the dependency-list.\n"
        << "If you want to cleanup all the dependency files without removing them as dependency, run:\n"
        << " $ catcare cleanup\n"
        << "This can be useful if you want to minimize the storage usage.\n";
        await_continue();

        CATGUIDE_HEADER();
        std::cout 
        << "To quickly setup your own project as a downloadable catcare-project simply run:\n"
        << " $ catcare setup <project-name>\n"
        << "This creates a `cat_checklist.inipp` where all the data about the project are stores.\n";
        await_continue();

        CATGUIDE_HEADER();
        std::cout 
        << "Your `cat_checklist.inipp` contains an entry called `files`, \n"
        << "this is the list of files the CatCaretaker will download.\n"
        << "But instead of adding them manually, you can use this command:\n"
        << " $ catcare add <path>\n"
        << "This will also add all the directories on the way there.\n";
        await_continue();

        CATGUIDE_HEADER();
        std::cout 
        << "Not sure if any of your dependencies needs an update?\n"
        << "try:\n"
        << " $ catcare check [.all|<project>]\n"
        << "If the main repository has a newer version available, it will tell you about it.\n";
        await_continue();

        CATGUIDE_HEADER();
        std::cout 
        << "To download the newest version of each dependency run:\n"
        << " $ catcare sync\n"
        << "You can also read the latest patch notes of a project by doing:\n"
        << " $ catcare whatsnew <project>\n"
        << "Note that this will only work if the target project contains a `" << CATCARE_RELEASES_FILE << "` !\n";
        await_continue();

        CATGUIDE_HEADER();
        std::cout 
        << "To quickly see what's the new release of an project, do:\n"
        << " $ catcare whatsnew <project>\n"
        << "This will give you the latest version and patchnotes! (If it contains a valid `" << CATCARE_RELEASES_FILE "`)\n";
        await_continue();

        CATGUIDE_HEADER();
        std::cout 
        << "Releases are a feature of the CatCaretaker as mentioned before,\n"
        << "to create your own, simply do:\n"
        << " $ catcare release\n"
        << "This will ask you a few questions and generate a `" << CATCARE_RELEASES_FILE <<"` afterwards!\n";
        await_continue();

        CATGUIDE_HEADER();
        std::cout 
        << "Not sure what to download? There is always the option to browse through a project-list.\n"
        << "You can view such a list with:\n"
        << " $ catcare browse [official|<code>]\n" // TODO: add code example
        << "`official` downloads the `" << CATCARE_BROWSING_FILE << "` directly from the CatCaretaker repository.\n"
        << "To create your own `" << CATCARE_BROWSING_FILE << "` see the template with:\n"
        << " $ catcare template browsing\n";
        await_continue();

        CATGUIDE_HEADER();
        std::cout 
        << "Configuration is also possible, check out:\n"
        << " $ catcare config explain\n"
        << " $ catcare option <option> to <value>\n"
        << "...to view all available settings and set them globally.\n";
        await_continue();

        CATGUIDE_HEADER();
        std::cout 
        << "This is the end of the guide, if you have any more questions,\n"
        << "have a look at the github repository: https://github.com/LabRicecat/CatCaretaker\n"
        << "or run `$ catcare --help`.\n\n"
        << "Thanks for using the CatCaretaker!\n";
        await_continue();
    }
    else if(pargs("setup") != "") {
        std::string name = pargs("setup");
        if(std::filesystem::exists(CATCARE_CHECKLISTNAME))
            std::filesystem::remove_all(CATCARE_CHECKLISTNAME);
        make_register();
        make_checklist();
        IniFile f = IniFile::from_file(CATCARE_CHECKLISTNAME);
        f.set("name",name,"Info");
        f.to_file(CATCARE_CHECKLISTNAME);
    }
    else if(pargs["sync"]) {
        print_message("DOWNLOAD","Syncronising the dependencies...");
        IniList deps = get_dependencylist();
        if(std::filesystem::exists(CATCARE_ROOT)) {
            std::filesystem::remove_all(CATCARE_ROOT);
        }
        make_register();
        for(auto i : deps) {
            std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + (std::string)i);
            remove_from_register((std::string)i);
        }
        download_dependencies(deps);
    }
    else if(pargs("check") != "") {
        std::string proj = pargs("check");
        proj = to_lowercase(proj);
        if(proj == ".all") {
            int found = 0;
            IniDictionary reg = get_register();
            for(auto i : reg) {
                if(i.second.get_type() == IniType::String) {
                    auto [newv,oldv] = needs_update((std::string)i.second);
                    if(newv != "") {
                        std::cout << "Project \"" << i.first << "\" can be updated: " << oldv << " -> " << newv << "\n";
                        ++found;
                    }
                }
            }
            if(found == 0) {
                std::cout << "All dependencies are up to date!\n";
            }
            else {
                std::cout << "Found " << found << " available updates!\nRun `catcare sync` to update them all.\n";
            }
            return 0;
        }

        auto url = ask_to_resolve(proj,0);
        if(url.link == "") return 1;

        if(!installed(url.link)) {
            std::cout << "This project is not installed!\nDo you want to install it? [y/N]:";
            std::string inp;
            std::getline(std::cin,inp);
            if(inp == "Yes" || inp == "y" || inp == "Y" || inp == "yes") {
                std::string error = download_project(url.link);
                if(error != "")
                    print_message("ERROR","Error downloading project!\n-> " + error);
                else {
                    print_message("RESULT","Successfully installed!");
                    if(!is_dependency(url.link)) {
                        add_to_dependencylist(url.link);
                    }
                }
            }
        }
        else {
            auto [newv,oldv] = needs_update(url.link);
            if(newv != "") {
                std::cout << "This project can be updated: " << oldv << " -> " << newv << "\n";
            }
            else {
                std::cout << "This project is up to date!\n";
            }
        }
    }
    else if(pargs("browse") != "") {
        std::string brow = pargs("browse");
        UrlPackage url;
        if(brow == "official") {
            url.link = CATCARE_BROWSE_OFFICIAL;
        }
        else url = ask_to_resolve(brow,2);
        
        std::filesystem::create_directory(CATCARE_TMP_PATH);
        if(!download_page(url.link,CATCARE_TMP_PATH CATCARE_DIRSLASH CATCARE_BROWSING_FILE)) {
            print_message("ERROR","An error occured while downloading the browsing file!");
            return 1;
        }

        if(!browse(CATCARE_TMP_PATH CATCARE_DIRSLASH CATCARE_BROWSING_FILE, url.link)) {
            print_message("ERROR","The browsing file seems to be corrupted! Sorry.");
            return 1;
        }

        std::filesystem::remove_all(CATCARE_TMP_PATH);
    }
    else if(pargs["release"]) {
        std::cout << "Release version (common format: MAJOR.MINOR.PATCH)\n=> ";
        std::string version;
        std::getline(std::cin,version);
        IniSection release(version);

        IniFile file = IniFile::from_file(CATCARE_RELEASES_FILE);
        if(file.has_section(version)) {
            std::cout << "There is already such a release available! Maybe a typo?\n";
            return 1;
        }

        std::string path_notes;
        std::string input = "";
        std::cout << "Patchnotes: (:quit to exit)\n";
        while(true) {
            std::cout << ">>";
            std::getline(std::cin,input);
            if(input == ":q" || input == ":quit" || input == ":exit" || input == ":e") break;
            path_notes += input + "\n";
        }

        release["path_notes"] = path_notes;
        time_t t = time(NULL);
        tm* ctm = localtime(&t);
        release["date"] = 
            std::to_string(ctm->tm_year + 1900) + "/" +
            std::to_string(ctm->tm_mon + 1);
        
        file.sections.push_back(release);
        file.to_file(CATCARE_RELEASES_FILE);

        file = IniFile::from_file(CATCARE_CHECKLISTNAME);
        file.set("version",version,"Info");
        file.to_file(CATCARE_CHECKLISTNAME);
        std::cout << "\nSuccess! Changes successfully released!\n";
    }
    else if(pargs("add") != "") {
        make_checklist();
        std::filesystem::path file = std::filesystem::path(pargs("add"));
        std::string current = "";
        IniList list = get_filelist();
        int added = 0;
        for(auto i : file) {
            std::string p;
            if(current != "") 
                p += current + CATCARE_DIRSLASH;
            p += i.string();
            if(!std::filesystem::exists(p)) continue;
            if(std::filesystem::is_directory(p)) 
                p = "$" + p;

            bool found = false;
            for(auto j : list) {
                if(j.get_type() == IniType::String && (std::string)j == p) {
                    found = true;
                }
            }

            if(!found) {
                IniElement elem;
                elem = p;
                list.push_back(elem);
                ++added;
            }

            if(current != "")
                current += CATCARE_DIRSLASH;
            current += i.string();
        }
        if(added == 0) {
            std::cout << "No files or directories were added!\n";
        }
        else {
            set_filelist(list);
            std::cout << "File added! (New entries: " << added << ")\n";
        }
    }
    else if(pargs("whatsnew") != "") {
        std::string proj = pargs("whatsnew");
        auto url = ask_to_resolve(proj,2);
        if(url.link == "") return 1;

        std::filesystem::create_directory(CATCARE_TMP_PATH);
        if(!download_page(url.link + CATCARE_RELEASES_FILE,CATCARE_TMP_PATH CATCARE_DIRSLASH CATCARE_RELEASES_FILE)) {
            print_message("ERROR","Unable to download releases info!");
            return 1;
        }
        if(!download_page(url.link + CATCARE_CHECKLISTNAME,CATCARE_TMP_PATH CATCARE_DIRSLASH CATCARE_CHECKLISTNAME)) {
            print_message("ERROR","Unable to download checklist info!");
            return 1;
        }
        IniFile file = IniFile::from_file(CATCARE_TMP_PATH CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
        if(!file || !file.has("version","Info") || file.get("version","Info").get_type() != IniType::String
                 || !file.has("name","Info") || file.get("name","Info").get_type() != IniType::String) {
            print_message("ERROR","The main cat_checklist.inipp seems to be corrupted! Contact the maintainer if possible, or check your internet connection!");
            return 1;
        }

        std::string version = (std::string)file.get("version","Info");
        std::string name = (std::string)file.get("name","Info");
        IniFile release_file = IniFile::from_file(CATCARE_TMP_PATH CATCARE_DIRSLASH CATCARE_RELEASES_FILE);
        
        if(!release_file.has_section(version)) {
            print_message("ERROR","The releases.inipp from this project does not contain any data for the current most recent version! Oh no!");
            return 1;
        }
        IniSection data = release_file.section(version);

        if(!data.has("date") || !data.has("path_notes")) {
            print_message("ERROR","The releases.inipp data for the latest version seems to be incomplete!");
            return 1;
        }
        IniElement date = data["date"];
        IniElement path_notes = data["path_notes"];

        if(date.get_type() != IniType::String || path_notes.get_type() != IniType::String) {
            print_message("ERROR","The releases.inipp data for the latest version is invalid!");
            return 1;
        }
        std::cout << "Name: " << name << "\n";
        std::cout << "Version: " << version << "\n";
        std::cout << "Release from date: " << (std::string)date << "\n";
        std::cout << "Notes:\n" << (std::string)path_notes << "\n";
        std::filesystem::remove_all(CATCARE_TMP_PATH);
    }
    else if(pargs("template") != "") {
        std::string templ = pargs("template");
        if(templ == "checklist") {
            make_file(CATCARE_CHECKLISTNAME,R"(
[Info]
name = "project-name"
# optional
version = "0.0.0"
description = ""

[Download]
files = [] # append files manually or use `catcare add <file>`
# optional
dependencies = [] # append manually and sync or use `catcare get <user>/<project>`
scripts = [] 
tags = []
authors = []
license = ""
documentation = "" # put the link here
)");
            print_message("INFO","Template successfully created as " CATCARE_CHECKLISTNAME);
        }
        else if(templ == "script") {
            make_file("script.ccs",R"(
@main[]
    echoln("Hello, Script!")
    if(WINDOWS)
        if(not installed("dependency"))
            add_project("dependency")
        endif()
    else()
        add_file("file.txt","File Content")
    endif()
)");
            print_message("INFO","Template successfully created as script.css");
        }
        else if(templ == "browsing") {
            make_file(CATCARE_BROWSING_FILE,R"(
[Main]
browsing = [
    "https://...", # individual projects
    "https://..."
]

include = [] # add other "browsing.inipp" files from different sources via URL here
)");
            print_message("INFO","Template successfully created as " CATCARE_BROWSING_FILE);
        }
        else if(templ == "urlrules") {
            make_file(CATCARE_URLRULES_FILENAME,R"(
"--- urlrules.ccr ---"

" name here your rule to resolve a input"
RULE github;
    "the link with {} for placeholders"
    LINK "https://raw.githubusercontent.com/{user}/{project}/{branch}/";
    "define the order in which the user enters the placeholders"
    WITH user 1
         project 2
         branch 3;
    "define the splitting symbols for the input"
    SIGNS / @;
    "after a resolved download by this rule, execute this file from )" CATCARE_ATTACHMENT_PATH R"(
    ATTACH "some-script"; 
)");
            print_message("INFO","Template successfully created as " CATCARE_URLRULES_FILENAME);
        }
        else if(templ == "list") {
            std::cout << "Available templates:\n"
                << "  checklist -> creates a template " CATCARE_CHECKLISTNAME "\n"
                << "  script    -> creates a template script.ccs\n"
                << "  urlrules  -> creates a template " CATCARE_URLRULES_FILENAME "\n"
                << "  browsing  -> creates a template " CATCARE_BROWSING_FILE "\n";
        }
        else {
            print_message("ERROR","Unknown template: " + templ);
        }
    }
    else if(pargs["blacklist"]) {
        if(pargs("append") == "" && pargs("pop") == "" && !pargs["show"]) {
            print_help();
        }

        if(pargs("append") != "") {
            std::string entry = pargs("append");
            auto url = ask_to_resolve(entry,2);
            if(url.link == "") return 1;
            if(blacklisted(url.link)) {
                std::cout << "This project is already blacklisted!\n";
                return 0;
            }
            IniFile file = IniFile::from_file(CATCARE_CONFIG_FILE);
            IniList list = file.get("blacklist");
            list.push_back(url.link);
            file.set("blacklist",list);
            file.to_file(CATCARE_TMP_PATH);
            std::cout << "Successfully added!\n";
        }
        else if(pargs("pop") != "") {
            std::string entry = pargs("pop");
            auto url = ask_to_resolve(entry,2);
            if(url.link == "") return 1;
            if(!blacklisted(url.link)) {
                std::cout << "This project is not blacklisted!\n";
                return 0;
            }
            IniFile file = IniFile::from_file(CATCARE_TMP_PATH);
            IniList list = file.get("blacklist");
            for(size_t i = 0; i < list.size(); ++i) {
                if(list[i].get_type() == IniType::String && (std::string)list[i] == url.link) {
                    list.erase(list.begin()+i);
                    break;
                }
            }
            file.set("blacklist",list);
            file.to_file(CATCARE_TMP_PATH);
            std::cout << "Successfully removed!\n";
        }
        else {
            IniFile file = IniFile::from_file(CATCARE_CONFIG_FILE);
            IniList list = file.get("blacklist");

            if(list.empty()) {
                std::cout << "No projects blacklisted!\n";
            }
            else {
                std::cout << "Blacklist: \n";
                for(auto i : list) {
                    if(i.get_type() == IniType::String) {
                        std::cout << "-> " << (std::string)i << "\n";
                    }
                }
            }
        }

        write_localconf();
    }
    else if(pargs("macro") != "") {
        std::string macro = pargs("macro");
        if(!std::filesystem::exists(CATCARE_TMP_PATH)) {
            std::filesystem::create_directory(CATCARE_MACRO_PATH);
        }

        if(macro == ".list") {
            std::cout << "Installed macros:\n";
            for(auto i : std::filesystem::directory_iterator(CATCARE_MACRO_PATH CATCARE_DIRSLASH)) {
                std::cout << i.path().string() << "\n";
            }
            return 0;
        }
        if(macro == ".help") {
            std::cout 
            << "Macro help:\n\n"
            << "Macros are user defined ulitity scripts written in a CareScript variant. (See the wiki for docs)\n"
            << "Your macro directory is " << CATCARE_MACRO_PATH << "\n"
            << "Every file placed there can be executed with `catcare macro <name> <args...>`\n"
            << "This file requires a `macro_call` label with N amount of arguments, this label will get executed\n"
            << "with automatically loaded extension from " << CATCARE_EXTENSION_PATH << "\n";
            return 0;
        }

        if(!std::filesystem::exists(CATCARE_MACRO_PATH CATCARE_DIRSLASH + macro + CATCARE_CARESCRIPT_EXT)) {
            std::cout << "No such macro: " << (macro + CATCARE_CARESCRIPT_EXT) << "\n"
            << "Check " << (CATCARE_MACRO_PATH CATCARE_DIRSLASH) << " to see if it's installed!\n";
            return 1;
        }

        interpreter.settings.line = 1;
        ScriptArglist args;
        for(auto i : pargs.get_bin()) {
            args.push_back(ScriptVariable(i));
        }
        
        std::string r;
        std::ifstream ifile(CATCARE_MACRO_PATH CATCARE_DIRSLASH + macro + CATCARE_CARESCRIPT_EXT);
        while(ifile.good()) r += ifile.get();
        if(r != "") r.pop_back();

        interpreter.pre_process(r).on_error([&](Interpreter& i) {
            std::cout << "Error in macro: " << i.error() << "\n";
        });
        if(!interpreter) {
            return 1;
        }

        if(interpreter.settings.labels.count("macro_call") == 0) {
            std::cout << "Invalid macro file! No \"macro_call\" label found! (at " << (CATCARE_MACRO_PATH CATCARE_DIRSLASH + macro + CATCARE_CARESCRIPT_EXT) << "\n";
            return 1;
        }
        auto call = interpreter.settings.labels["macro_call"];
        
        if(call.arglist.size() != args.size()) {
            std::cout << "Invalid arguments!\nExpected: [";
            std::string p;
            for(auto i : call.arglist) {
                p += i + ", ";
            }
            if(p != "") { p.pop_back(); p.pop_back(); }
            std::cout << p << "]\n";
            std::cout << "But got:";
            p = "";
            for(auto i : args) {
                p += i.printable() + ", ";
            }
            if(p != "") { p.pop_back(); p.pop_back(); }
            std::cout << "[" << p << "]\n";
            return 1;
        }
    
        interpreter.run("macro_call",args).on_error([](Interpreter& i){
            print_message("ERROR",i.error());
        });
    }
    else if(pargs["this"]) {
        if(!std::filesystem::exists(CATCARE_CHECKLISTNAME)) {
            std::cout << "Not in a catcaretaker project.\n";
            return 1;
        }
        IniDictionary d = extract_configs(IniFile::from_file(CATCARE_CHECKLISTNAME));
        print_config(d);
    }
    else {
        print_help();
    }
}