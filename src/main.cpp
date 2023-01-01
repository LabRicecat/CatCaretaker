#include "../inc/configs.hpp"
#include "../inc/options.hpp"
#include "../mods/ArgParser/ArgParser.h"

void print_help() {
    std::cout << "## CatCaretaker\n"
            << "A configurable helper to reuse already made work fast and efficient from github.\n\n"
            << " catcare <option> [arguments] [flags]\n\n"
            << "option :=\n"
            << "   download|get <repository>    :  downloads and sets up the project.\n"
            << "   local <path/redirect>        :  copies an already existing project by path or redirect.\n"
            << "   erase|remove [.all|<proj>]   :  removes an installed project.\n"
            << "   cleanup                      :  removes all installed projects.\n"
            << "   info <install>               :  shows infos about the selected project.\n"
            << "   option <option> to <value>   :  sets OPTION to VALUE in the config file\n"
            << "   redirect <option> to <value> :  sets OPTION as local redirect to VALUE in the config file.\n"
            << "   config [explain|reset|show]  :  manages the config file.\n"
            << "   check [.all|<project>]       :  checks if for PROJECT is a new version available.\n"
            << "   browse [official|<repo>]     :  view a browsing file showing of different projects.\n"
            << "   guide                        :  starts a little config questionary.\n\n"
            << "   setup <name>                 :  sets up a checklist for a project.\n"
            << "   sync                         :  reinstalls all the dependencies of the current project.\n"
            << "flags := \n"
            << "   --help|-h                  :  prints this and exits.\n"
            << "   --silent|-s                :  prevents info and error messages.\n\n"
            << "By LabRiceCat (c) 2023\n"
            << "Repository: https://github.com/LabRiceCat/catcaretaker\n";
    std::exit(0);
}

int main(int argc,char** argv) {
    if(!std::filesystem::exists(CATCARE_HOME)) {
        reset_localconf();
    }
    if(!std::filesystem::exists(CATCARE_CONFIGFILE)) {
        std::ofstream of;
        make_file(CATCARE_CONFIGFILE);
        of.open(CATCARE_CONFIGFILE,std::ios::app);
        of << "options={}\nredirects={}\n";
        of.close();
    }
    load_localconf();

    if(options.count("default_silent") > 0 && (options["default_silent"] == "1" || options["default_silent"] == "true")) {
        opt_silence = true;
    }

    ArgParser parser = ArgParser()
        .addArg("--help",ARG_TAG,{"-h"},0)
        .addArg("download",ARG_SET,{"get"},0)
        .addArg("local",ARG_SET,{},0)
        .addArg("erase",ARG_SET,{"remove"},0)
        .addArg("cleanup",ARG_TAG,{},0)
        .addArg("info",ARG_SET,{},0)
        .addArg("list",ARG_TAG,{},0)
        .addArg("option",ARG_SET,{},0)
        .addArg("redirect",ARG_SET,{},0)
        .addArg("to",ARG_SET,{"is"},2)
        .addArg("config",ARG_SET,{},0)
        .addArg("setup",ARG_SET,{},0)
        .addArg("guide",ARG_TAG,{},0)
        .addArg("sync",ARG_TAG,{},0)
        .addArg("check",ARG_SET,{},0)
        .addArg("browse",ARG_SET,{},0)
        .addArg("--silent",ARG_TAG,{"-s"})
    ;

    ParsedArgs pargs = parser.parse(argv,argc);

    if(!pargs || pargs["--help"]) {
        print_help();
    }

    if(pargs["--silent"]) {
        opt_silence = true;
    }

    if(pargs("download") != "") {
        std::string error;
        std::string repo = pargs("download");
        repo = to_lowercase(repo);
        error = download_repo(repo);

        if(error != "")
            print_message("ERROR","Error downloading repo: \"" + repo + "\"\n-> " + error);
        else {
            print_message("RESULT","Successfully installed!");
            if(!is_dependency(repo)) {
                add_to_dependencylist(repo);
            }
        }
    }
    else if(pargs("local") != "") {
        std::string repo = pargs("local");
        std::string error = "";
        if(is_redirected(repo)) {
            error = get_local(repo,local_redirect[repo]);
        }
        else {
            error = get_local(last_name(repo),std::filesystem::path(repo).parent_path().string());
        }

        if(error != "")
            print_message("ERROR","Error copying local repo: \"" + repo + "\"\n-> " + error);
        else {
            print_message("RESULT","Successfully copied!");
            if(!is_dependency(repo)) {
                add_to_dependencylist(repo,true);
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
            if(!std::filesystem::exists(CATCARE_ROOT + CATCARE_DIRSLASH + pargs("erase"))) {
                print_message("ERROR","No such repo installed: \"" + pargs("erase") + "\"");
                return 0;
            }
            std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + pargs("erase"));
            
            print_message("DELETE","Removed repo: \"" + pargs("erase") + "\"");
            remove_from_register(pargs("erase"));
            remove_from_dependencylist(pargs("erase"));
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
        std::string repo = pargs("info");
        repo = to_lowercase(repo);
        repo = app_username(repo);
        auto [usr, proj] = get_username(repo);
        if(!installed(repo)) {
            print_message("ERROR","No such repo installed: \"" + repo + "\"");
            return 0;
        }
        IniDictionary d = extract_configs(proj);
        std::cout << "Name: " << (std::string)d["name"] << "\n";
        if(d.count("version") != 0) {
            std::cout << "Version: " << (std::string)d["version"] << "\n";
        }
        if(d.count("dependencies") != 0 && d["dependencies"].to_list().size() != 0) {
            std::cout << "Dependencies: \n";
            for(auto i : d["dependencies"].to_list()) {
                std::cout << " - " << (std::string)i << "\n";
            }
        }
        std::cout << "Files: \n";
        for(auto i : d["files"].to_list()) {
            std::cout << " - " << (std::string)i << "\n";
        }
    }
    else if(pargs["list"]) {
        IniList l = get_register();
        std::cout << "Installed projects:\n";
        for(auto i : l) {
            std::cout << " -> " << (std::string)i << "\n";
        }
    }
    else if(pargs("option") != "") {
        if(pargs("to") == "") {
            print_message("ERROR","Invalid option syntax: option|set <option> to <value>");
        }
        options[pargs("option")] = pargs("to");
        write_localconf();
    }
    else if(pargs("redirect") != "") {
        if(pargs("to") == "") {
            print_message("ERROR","Invalid option syntax: redirect <name> to <path>");
        }
        local_redirect[pargs("redirect")] = std::filesystem::current_path().string() + CATCARE_DIRSLASH + pargs("to");
        write_localconf();
    }
    else if(pargs("config") != "") {
        std::string opt = pargs("config");

        if(opt == "show") {
            IniFile file = IniFile::from_file(CATCARE_CONFIGFILE);
            IniDictionary d = file.get("options");
            std::cout << "Global options:\n";
            for(auto i : d) {
                std::cout << i.first << "\t:\t" << (std::string)i.second << "\n";
            }
        }
        else if(opt == "reset") {
            if(confirmation("reset the config file?")) {
                reset_localconf();
                std::cout << "Done!\n";
            }
        }
        else if(opt == "explain") {
            std::cout << "Config options:\n"
            << "username        :  The github username. (default: LabRicecat)\n"
            << "default_branch  :  The default branch for the projects. (default: main)\n"
            << "install_dir     :  The directory it installs the projects into. (default: catmods)\n"
            << "install_url     :  The url we try to download from. (default: https://raw.githubusercontent.com/)\n"
            << "default_silent  :  If \"true\" -> `--silent` will be enabled by default. (default: false)\n"
            << "local_over_url  :  If \"true\" -> local redirects get priorities when it comes to dependencies (default: false)\n"
            << "clear_on_error  :  If \"true\" -> clears the downloading project if an error occurs. (default: true)\n"
            ;

        }
        else {
            print_help();
        }
    }
    else if(pargs["guide"]) {
        std::cout << "Welcome to the CatCaretaker guide.\nI will ask some question for setup purpose.\nIf you want to select the default option, press enter with no input.\n\n";
        
        std::cout << "What is your github username (default: Labricecat): ";
        std::string username = ask_and_default("Labricecat");

        std::cout << "\nWhat branch should be used for the projects (default: main): ";
        std::string branch = ask_and_default("main");

        std::cout << "\nWhat should be the name for the project directory be (default: catmods): ";
        std::string instdir = ask_and_default("catmods");

        std::cout << "\nGreat! That's everything I need.\nFor more use `config explain`.";

        options["username"] = username;
        options["default_branch"] = branch;
        options["install_dir"] = instdir;

        write_localconf();
    }
    else if(pargs("setup") != "") {
        std::string name = pargs("setup");

        make_checklist();
        IniFile f = IniFile::from_file(CATCARE_CHECKLISTNAME);
        f.set("name",name,"Info");
        f.to_file(CATCARE_CHECKLISTNAME);
    }
    else if(pargs["sync"]) {
        print_message("DOWNLOAD","Syncronising the dependencies...");
        IniList deps = get_dependencylist();
        for(auto i : deps) {
            std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + (std::string)i);
            remove_from_register((std::string)i);
        }
        download_dependencies(get_dependencylist());
    }
    else if(pargs("check") != "") {
        std::string proj = pargs("check");
        proj = to_lowercase(proj);
        if(proj == ".all") {
            int found = 0;
            IniList reg = get_register();
            for(auto i : reg) {
                if(i.get_type() == IniType::String) {
                    std::string p = app_username((std::string)i);
                    auto [newv,oldv] = needs_update(p);
                    if(newv != "") {
                        std::cout << "Project \"" << p << "\" can be updated: " << oldv << " -> " << newv << "\n";
                        ++found;
                    }
                }
            }
            if(found == 0) {
                std::cout << "All dependencies up to date!\n";
            }
            else {
                std::cout << "Found " << found << " available updates!\nRun `catcare sync` to update them all.\n";
            }
            return 0;
        }

        app_username(proj);
        if(!installed(proj)) {
            std::cout << "Project: \"" << proj << "\" is not installed!\nDo you want to install it? [y/N]:";
            std::string inp;
            std::getline(std::cin,inp);
            if(inp == "Yes" || inp == "y" || inp == "Y" || inp == "yes") {
                std::string error = download_repo(proj);
                if(error != "")
                    print_message("ERROR","Error downloading repo: \"" + proj + "\"\n-> " + error);
                else {
                    print_message("RESULT","Successfully installed!");
                    if(!is_dependency(proj)) {
                        add_to_dependencylist(proj);
                    }
                }
            }
        }
        else {
            auto [newv,oldv] = needs_update(proj);
            if(newv != "") {
                std::cout << "Project \"" << proj << "\" can be updated: " << oldv << " -> " << newv << "\n";
            }
            else {
                std::cout << "Project \"" << proj << "\" is up to date!\n";
            }
        }
    }
    else if(pargs("browse") != "") {
        std::string brow = pargs("browse");
        std::string to_download = CATCARE_REPOFILE(brow,CATCARE_BROWSING_FILE);
        if(brow == "official") {
            to_download = CATCARE_BROWSE_OFFICIAL;
        }
        
        std::filesystem::create_directory(CATCARE_TMPDIR);
        if(!download_page(to_download,CATCARE_TMPDIR CATCARE_DIRSLASH CATCARE_BROWSING_FILE)) {
            print_message("ERROR","An error occured while downloading the browsing file");
            std::filesystem::remove(CATCARE_TMPDIR);
            return 1;
        }

        if(!browse(CATCARE_TMPDIR CATCARE_DIRSLASH CATCARE_BROWSING_FILE)) {
            print_message("ERROR","The browsing file seems to be corrupted! Sorry.");
            std::filesystem::remove(CATCARE_TMPDIR);
            return 1;
        }

        std::filesystem::remove(CATCARE_TMPDIR);
    }
    else {
        print_help();
    }
}