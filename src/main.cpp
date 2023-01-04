#include "../inc/configs.hpp"
#include "../inc/options.hpp"
#include "../mods/ArgParser/ArgParser.h"
#include "../carescript/carescript.hpp"

#include <time.h>

void print_help() {
    std::cout << "## CatCaretaker\n"
            << "A configurable helper to reuse already made work fast and efficient from github.\n\n"
            << " catcare <option> [arguments] [flags]\n\n"
            << "option :=\n"
            << "   download|get <repository>    :  downloads and sets up the project.\n"
            << "   local <path/redirect>        :  copies an already existing project by path or redirect.\n"
            << "   erase|remove [.all|<proj>]   :  removes an installed project.\n"
            << "   add <path>                   :  add a file to the downloadlist\n"
            << "   cleanup                      :  removes all installed projects.\n"
            << "   info <install>               :  shows infos about the selected project.\n"
            << "   option <option> to <value>   :  sets OPTION to VALUE in the config file\n"
            << "   redirect <option> to <value> :  sets OPTION as local redirect to VALUE in the config file.\n"
            << "   config [explain|reset|show]  :  manages the config file.\n"
            << "   check [.all|<project>]       :  checks if for PROJECT is a new version available.\n"
            << "   browse [official|<repo>]     :  view a browsing file showing of different projects.\n"
            << "   whatsnew <project>           :  read the latest patch notes of a project.\n"
            << "   guide                        :  starts a little config questionary.\n"
            << "   setup <name>                 :  sets up a checklist for a project.\n"
            << "   release                      :  set up a release note easily.\n"
            << "   template [list]              :  create a template file.\n"       
            << "   blacklist [append|pop|show]  :  blacklsit certain projects.\n"  
            << "   sync                         :  reinstalls all the dependencies of the current project.\n\n"
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
        .addArg("release",ARG_TAG,{},0)
        .addArg("add",ARG_SET,{},0)
        .addArg("whatsnew",ARG_SET,{},0)
        // .addArg("run",ARG_SET,{},0)
        .addArg("template",ARG_SET,{},0)

        .addArg("blacklist",ARG_TAG,{},0)
        .addArg("append",ARG_SET,{},1)
        .addArg("pop",ARG_SET,{},1)
        .addArg("show",ARG_TAG,{},1)

        .addArg("--silent",ARG_TAG,{"-s"})
    ;

    std::atexit([](){
        if(option_or("clear_on_error","true") == "true") {
            if(std::filesystem::exists(CATCARE_TMPDIR)) {
                std::filesystem::remove_all(CATCARE_TMPDIR);
            }
        }
    });

    ParsedArgs pargs = parser.parse(argv,argc);

    if(!pargs || pargs["--help"]) {
        print_help();
    }

    if(pargs["--silent"]) {
        opt_silence = true;
    }

    if((pargs("append") != "" || pargs("pop") != "" || pargs["show"]) && !pargs["blacklist"]) {
        print_help();
    }
    else if(pargs("download") != "") {
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
            std::string name = to_lowercase(pargs("erase"));
            name = app_username(name);
            auto [usr,proj] = get_username(name);
            if(!std::filesystem::exists(CATCARE_ROOT + CATCARE_DIRSLASH + name)) {
                print_message("ERROR","No such repo installed: \"" + name + "\"");
                return 0;
            }
            
            std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + proj);
            
            print_message("DELETE","Removed repo: \"" + name + "\"");
            remove_from_register(name);
            remove_from_dependencylist(name);
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
            << "default_silent  :  If true -> `--silent` will be enabled by default. (default: false)\n"
            << "local_over_url  :  If true -> local redirects get priorities when it comes to dependencies (default: false)\n"
            << "clear_on_error  :  If true -> clears the downloading project if an error occurs. (default: true)\n"
            << "show_script_src :  If true -> open a little lookup when a new script gets executed. (default: false)\n"
            << "no_scripts      :  If true -> stops all scripts from executing. (Warning: not recomended, default: false)\n";

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
        make_register();
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
        download_dependencies(deps);
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
            return 1;
        }

        if(!browse(CATCARE_TMPDIR CATCARE_DIRSLASH CATCARE_BROWSING_FILE)) {
            print_message("ERROR","The browsing file seems to be corrupted! Sorry.");
            return 1;
        }

        std::filesystem::remove_all(CATCARE_TMPDIR);
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
            for(size_t i = 0; i < input.size(); ++i) {
                if(input[i] == '"' || input[i] == '\'' || input[i] == '\\') {
                    input.insert(input.begin()+i,'\\');
                    ++i;
                }
            }
            path_notes += input + "\n";
        }

        release["path_notes"] = path_notes;
        time_t t = time(NULL);
        tm* ctm = localtime(&t);
        release["date"] = 
            std::to_string(ctm->tm_year + 1900) + "/" +
            std::to_string(ctm->tm_mon + 1) + "/" + 
            std::to_string(ctm->tm_mday) + " " +
            std::to_string(ctm->tm_hour) + ":" +
            std::to_string(ctm->tm_min) + ":" +
            std::to_string(ctm->tm_sec);
        
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
        proj = to_lowercase(app_username(proj));

        std::filesystem::create_directory(CATCARE_TMPDIR);
        if(!download_page(CATCARE_REPOFILE(proj,CATCARE_RELEASES_FILE),CATCARE_TMPDIR CATCARE_DIRSLASH CATCARE_RELEASES_FILE)) {
            print_message("ERROR","Unable to download releases info!");
            return 1;
        }
        if(!download_page(CATCARE_REPOFILE(proj,CATCARE_CHECKLISTNAME),CATCARE_TMPDIR CATCARE_DIRSLASH CATCARE_CHECKLISTNAME)) {
            print_message("ERROR","Unable to download checklist info!");
            return 1;
        }
        IniFile file = IniFile::from_file(CATCARE_TMPDIR CATCARE_DIRSLASH CATCARE_CHECKLISTNAME);
        if(!file || !file.has("version","Info") || file.get("version","Info").get_type() != IniType::String
                 || !file.has("name","Info") || file.get("name","Info").get_type() != IniType::String) {
            print_message("ERROR","The main cat_checklist.inipp seems to be corrupted! Contact the maintainer if possible, or check your internet connection!");
            return 1;
        }

        std::string version = (std::string)file.get("version","Info");
        std::string name = (std::string)file.get("name","Info");
        file = IniFile::from_file(CATCARE_TMPDIR CATCARE_DIRSLASH CATCARE_RELEASES_FILE);
        
        if(!file.has_section(version)) {
            print_message("ERROR","The releases.inipp from this project does not contain any data for the current most recent version! Oh no!");
            return 1;
        }
        IniSection data = file.section(version);

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
        std::filesystem::remove_all(CATCARE_TMPDIR);
    }
    else if(pargs("run") != "") {
        std::string r;
        std::ifstream f(pargs("run"));
        while(f.good()) r += f.get();
        if(!r.empty()) r.pop_back();

        std::string err = run_script(r);
        if(err != "") {
            print_message("ERROR",err);
            return 1;
        }
    }
    else if(pargs("template") != "") {
        std::string templ = pargs("template");
        if(templ == "checklist") {
            make_file("cat_checklist.inipp",
            "[Info]\n"
            "name = \"project-name\"\n"
            "version = \"version-here\" # optional \n\n"
            "[Download]\n"
            "files = [] # append files manually or use `catcare add <file>`\n"
            "dependencies = [] # append manually and sync or use `catcare get <user>/<project>`\n"
            "scripts = [] # optional\n");
            print_message("INFO","Template successfully created as cat_checklist.inipp");
        }
        else if(templ == "script") {
            make_file("script.ccs",
            "echo(\"Hello Script\") # print text\n"
            "add(FILE,\"name\") # create a file\n\n"
            "if(WINDOWS) # check for a specific OS\n"
            "add(DEPENDENCY,\"name\") # download a dependency\n"
            "endif() # end of the if from above\n\n"
            "project_info(NAME,name)\n"
            "project_info(VERSION,version)\n"
            "echo(\"project: \" + $name + \" version: \" + $version)\n\n"
            "# look at the wiki for more information about carescript\n");
            print_message("INFO","Template successfully created as script.css");
        }
        else if(templ == "browsing") {
            make_file("browsing.inipp",
            "[Main]\n"
            "browsing = {\n"
                "\tPROJECT_NAME: {\n"
                    "\t\tdescription: \"short description of the project.\",\n"
                    "\t\tlanguage: \"python/C/...\",\n"
                    "\t\tauthor: \"username of the author\",\n"
                "\t},\n"
            "}\n\n"
            "# keep PROJECT_NAME and \"author\" the same as the github user and project\n"
            "# so it can be directly downloaded\n"
            );
            print_message("INFO","Template successfully created as browsing.inipp");
        }
        else if(templ == "list") {
            std::cout << "Available templates:\n"
                << "  checklist -> creates a template cat_checklist.inipp\n"
                << "  script -> creates a template script.ccs\n"
                << "  browsing -> creates a template browsing.inipp\n";
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
            entry = app_username(to_lowercase(entry));
            if(blacklisted(entry)) {
                std::cout << "This repo is already blacklisted!\n";
                return 0;
            }
            IniFile file = IniFile::from_file(CATCARE_CONFIGFILE);
            IniList list = file.get("blacklist");
            list.push_back(entry);
            file.set("blacklist",list);
            file.to_file(CATCARE_CONFIGFILE);
            std::cout << "Successfully added!\n";
        }
        else if(pargs("pop") != "") {
            std::string entry = pargs("pop");
            entry = app_username(to_lowercase(entry));
            if(!blacklisted(entry)) {
                std::cout << "This repo is not blacklisted!\n";
                return 0;
            }
            IniFile file = IniFile::from_file(CATCARE_CONFIGFILE);
            IniList list = file.get("blacklist");
            for(size_t i = 0; i < list.size(); ++i) {
                if(list[i].get_type() == IniType::String && (std::string)list[i] == entry) {
                    list.erase(list.begin()+i);
                    break;
                }
            }
            file.set("blacklist",list);
            file.to_file(CATCARE_CONFIGFILE);
            std::cout << "Successfully removed!\n";
        }
        else {
            IniFile file = IniFile::from_file(CATCARE_CONFIGFILE);
            IniList list = file.get("blacklist");

            if(list.empty()) {
                std::cout << "No repos blacklisted!\n";
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
    else {
        print_help();
    }
}