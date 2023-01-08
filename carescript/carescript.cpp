#include "carescript.hpp"
#include "../inc/configs.hpp"
#include "../inc/options.hpp"
#include <filesystem>

ScriptVariable to_var(KittenToken src) {
    ScriptVariable ret;
    if(src.str) {
        ret.string = src.src;
        ret.type = ScriptVariable::Type::String;
    }
    else if(src.src == "null") {
        ret.type = ScriptVariable::Type::Null;
    }
    else {
        try {
            ret.number = std::stold(src.src);
            ret.type = ScriptVariable::Type::Number;
        }
        catch(...) {
            ret.name = src.src;
            ret.type = ScriptVariable::Type::Name;
        }
    }
    return ret;
}

std::string run_script(std::string source) {
    ScriptSettings settings;
    auto labels = into_labels(source);
    std::string ret = run_script("main",labels,std::filesystem::current_path().parent_path());
    return ret;
}

std::string run_script(std::string label_name, std::map<std::string,ScriptLabel> labels, std::filesystem::path parent_path, std::vector<ScriptVariable> args) {
    if(labels.empty() || labels.count(label_name) == 0) return "";
    ScriptLabel label = labels[label_name];
    std::vector<lexed_kittens> lines;
    int line = -1;
    for(auto i : label.lines) {
        if(i.line != line) {
            line = i.line;
            lines.push_back({});
        }
        lines.back().push_back(i);
    }

    for(auto i : lines) 
        if(i.size() != 2 || i[0].str || i[1].str || i[1].src.front() != '(' || script_builtins.count(i[0].src) == 0) { 
            return "line " + std::to_string(i.front().line) + " is invalid (in label " + label_name + ")"; 
        }

    ScriptSettings settings;
    settings.parent_path = parent_path;
    settings.labels = labels;
    if(labels.count("global") != 0) {
        get_globalload(labels["global"].lines,settings);
        if(settings.error_msg != "") {
            return "line: " + std::to_string(settings.line) + ": " + settings.error_msg + " (in label global)";
        }
    }

    for(size_t i = 0; i < args.size(); ++i) {
        settings.variables[label.arglist[i]] = args[i];
    }
    for(auto i : lines) {
        if(settings.exit) return "";
        settings.line = i[0].line;
        std::string name = i[0].src;
        auto arglist = parse_argumentlist(i[1].src,settings);
        if(settings.error_msg != "") {
            if(settings.raw_error) return settings.error_msg;
            return "line " + std::to_string(settings.line) + ": " + settings.error_msg + " (in label " + label_name + ")";
        }
        ScriptBuiltin builtin = script_builtins[name];
        if(builtin.arg_count != arglist.size() && builtin.arg_count >= 0) {
            return "line " + std::to_string(i[0].line) + " " + name + " has invalid argument count " + " (in label " + label_name + ")";
        }
        std::string err = builtin.exec(arglist,settings);
        if(err != "") {
            return "line " + std::to_string(settings.line) + ": " + name + ": " + err + " (in label " + label_name + ")";
        }
        if(settings.error_msg != "") {
            if(settings.raw_error) return settings.error_msg;
            return "line " + std::to_string(settings.line) + ": " + name + ": " + settings.error_msg + " (in label " + label_name + ")";
        }
    }
    return "";
}

bool is_operator_char(char c) {
    return c == '+' ||
           c == '-' ||
           c == '*' ||
           c == '/' ||
           c == '^' ||
           c == '%' ||
           c == '$';
}

bool is_name_char(char c) {
    return (c <= 'Z' && c >= 'A') || 
            (c <= 'z' && c >= 'a') ||
            (c <= '9' && c >= '0') ||
            c == '_';
}

bool is_name(std::string s) {
    if(s.empty()) return false;
    try { 
        std::stoi(std::string(1,s[0]));
        return false;
    }
    catch(...) {}
    for(auto i : s) {
        if(!is_name_char(i)) return false;
    }
    return true;
}

bool is_label_arglist(std::string s) {
    if(s.size() < 2) return false;
    if(s[0] != '[' || s.back() != ']') return false;
    s.pop_back();
    s.erase(s.begin());

    KittenLexer arglist_lexer = KittenLexer()
        .add_extract(',')
        .erase_empty();
    auto lexed = arglist_lexer.lex(s);
    if(lexed.empty()) return true;

    bool found_n = true;
    for(auto i : lexed) {
        if(found_n) {
            if(!is_name(i.src)) return false;
            found_n = false;
        }
        else if(!found_n) {
            if(i.src != ",") return false;
            found_n = true;
        }
    }
    return !found_n;
}

std::vector<std::string> parse_label_arglist(std::string s) {
    if(!is_label_arglist(s)) return {};
    s.pop_back();
    s.erase(s.begin());

    std::vector<std::string> ret;
    KittenLexer arglist_lexer = KittenLexer()
        .add_extract(',')
        .erase_empty();
    auto lexed = arglist_lexer.lex(s);

    bool found_n = true;
    for(auto i : lexed) {
        if(found_n) {
            if(!is_name(i.src)) return {};
            found_n = false;
            ret.push_back(i.src);
        }
        else if(!found_n) {
            if(i.src != ",") return {};
            found_n = true;
        }
    }
    return ret;
}

std::vector<ScriptVariable> parse_argumentlist(std::string source, ScriptSettings& settings) {
    KittenLexer arg_lexer = KittenLexer()
        .add_capsule('(',')')
        .add_stringq('"')
        .add_ignore(' ')
        .add_ignore('\t')
        .add_ignore('\n')
        .add_con_extract(is_operator_char)
        .add_extract(',')
        .erase_empty();

    source.erase(source.begin());
    source.pop_back();

    auto lexed = arg_lexer.lex(source);
    if(lexed.empty()) return std::vector<ScriptVariable>{};
    std::vector<std::string> args(1);
    for(auto i : lexed) {
        if(!i.str && i.src == ",") {
            args.push_back({});
        }
        else {
            if(i.str) i.src = "\"" + i.src + "\"";
            else if(script_macros.count(i.src) != 0) i.src = script_macros.at(i.src);

            args.back() += " " + i.src;
        }
    }

    std::vector<ScriptVariable> ret;
    for(auto i : args) {
        ret.push_back(evaluate_expression(i,settings));
        if(settings.error_msg != "") return {};
    }
    return ret;
}

bool is_operator(std::string src) {
    return script_operators.count(src) != 0;
}

void process_op(std::stack<ScriptVariable>& st, ScriptOperator op, ScriptSettings& settings) {
    if(op.unary) {
        auto l = st.top(); st.pop();
        st.push(op.run_unary(l,script_null,settings));
    } 
    else {
        auto r = st.top(); st.pop();
        auto l = st.top(); st.pop();
        st.push(op.run(l,r,settings));
    }
}

ScriptVariable evaluate_expression(std::string source, ScriptSettings& settings) {
    KittenLexer expression_lexer = KittenLexer()
        .add_stringq('"')
        .add_capsule('(',')')
        .add_con_extract(is_operator_char)
        .add_ignore(' ')
        .add_ignore('\t')
        .add_backslashopt('t','\t')
        .add_backslashopt('n','\n')
        .add_backslashopt('r','\r')
        .add_backslashopt('\\','\\')
        .add_backslashopt('"','\"')
        .erase_empty();
    auto lexed = expression_lexer.lex(source);

    ScriptVariable ret;

    std::stack<ScriptVariable> stack;
    std::stack<ScriptOperator> ops;

    bool may_be_unary = true;
    for(size_t i = 0; i < lexed.size(); ++i) {
        if(!lexed[i].str && is_operator(lexed[i].src)) {
            ScriptOperator cur_op = script_operators[lexed[i].src];
            if(may_be_unary && cur_op.unary != 0) cur_op.unary = 1;
            else cur_op.unary = 0;
            while (!ops.empty() && (
                    (cur_op.unary == 0 && ops.top().priority >= cur_op.priority) ||
                    (cur_op.unary == 1 && ops.top().priority > cur_op.priority)
                )) {
                process_op(stack, ops.top(),settings);
                if(settings.error_msg != "") return script_null;
                ops.pop();
            }
            ops.push(cur_op);
            may_be_unary = true;
        } 
        else {
            stack.push(to_var(lexed[i]));
            may_be_unary = false;
        }
    }

    while (!ops.empty()) {
        process_op(stack, ops.top(),settings);
        if(settings.error_msg != "") return script_null;
        ops.pop();
    }
    if(stack.size() != 1) {
        settings.error_msg = "invalid expression: \"" + source + "\"";
        return script_null; 
    }
    return stack.top();
}

void get_globalload(lexed_kittens source, ScriptSettings& settings) {
    std::vector<lexed_kittens> lines;
    int line = -1;
    for(auto i : source) {
        if(i.line != line) {
            line = i.line;
            lines.push_back({});
        }
        lines.back().push_back(i);
    }
    for(auto i : lines) {
        if(i.size() < 3 || i[0].str || i[1].str || !is_name(i[0].src) || i[1].src != "=") {
            settings.line = i[0].line;
            settings.error_msg = "invalid syntax. <name> = <expression...>";
            return;
        }
        settings.line = i[0].line;
        std::string name = i[0].src;
        std::string line;
        for(size_t j = 2; j < i.size(); ++j) {
            if(i[j].str) i[j].src = "\"" + i[j].src + "\"";
            line += i[j].src + " ";
        }
        line.pop_back();
        settings.variables[name] = evaluate_expression(line,settings);
        if(settings.error_msg != "") {
            settings.line = i[0].line;
            return;
        }
    }
}

std::map<std::string,ScriptLabel> into_labels(std::string source) {
    std::map<std::string,ScriptLabel> ret;
    KittenLexer lexer = KittenLexer()
        .add_stringq('"')
        .add_capsule('(',')')
        .add_capsule('[',']')
        .add_ignore(' ')
        .add_ignore('\t')
        .add_linebreak('\n')
        .add_lineskip('#')
        .add_extract('@')
        .erase_empty();
    
    auto lexed = lexer.lex(source);
    std::vector<lexed_kittens> lines;
    int line = -1;
    for(auto i : lexed) {
        if(i.line != line) {
            line = i.line;
            lines.push_back({});
        }
        lines.back().push_back(i);
    }

    std::string current_label = "main";
    for(auto i : lines) {
        if(i.size() == 3 && i[0].src == "@" && !i[0].str && is_name(i[1].src) && !i[1].str && is_label_arglist(i[2].src) && !i[2].str) {
            //if(ret.count(current_label) == 0 || ret[current_label].lines.empty()) {
                current_label = i[1].src;
                ret[current_label].arglist = parse_label_arglist(i[2].src);
            //}
        }
        else {
            for(auto j : i) ret[current_label].lines.push_back(j);
        }
    }

    return ret;
}

#define ARG_REQUIRES(arg,type_) do{ if(args[arg].type != type_) { return "requires as argument " + std::to_string(arg) \
            + " a " + scriptType2string(type_) + " but got a " \
                    + scriptType2string(args[arg].type); } }while(0)
#define NARG_REQUIRES(arg,type_) do{ if(args[arg].type == type_) { return "does not allow argument " + std::to_string(arg) \
            + " to be a " + scriptType2string(args[arg].type); } }while(0)
#define DONTRUN_WIF() do{if(!settings.should_run.empty() && !settings.should_run.top()) return ""; }while(0)


std::map<std::string,ScriptBuiltin> script_builtins = {
    {"set",{2,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::Name);
        NARG_REQUIRES(1,ScriptVariable::Type::Name);
        settings.variables[args[0].name] = args[1];
        return "";
    }}},
    {"if",{1,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        if(!settings.should_run.empty() && !settings.should_run.top()) {
            ++settings.ignore_endifs;
            return ""; 
        }
        
        ARG_REQUIRES(0,ScriptVariable::Type::Number);
        
        settings.should_run.push(args[0].number == true);
        return "";
    }}},
    {"else",{0,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        if(settings.should_run.empty()) {
            return "no if";
        }
        settings.should_run.top() = !settings.should_run.top();
        return "";
    }}},
    {"endif",{0,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        if(settings.ignore_endifs != 0) {
            --settings.ignore_endifs;
            return "";
        }
        if(settings.should_run.empty()) return "no if";
        settings.should_run.pop();
        return "";
    }}},
    
    {"echo",{-1,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        std::stringstream out;
        for(auto i : args) {
            if(i.type == ScriptVariable::Type::String)
                out << i.string;
            else if(i.type == ScriptVariable::Type::Number)
                out << i.number;
            else 
                return "unexpected symbol: " + i.name;
        }
        std::cout << out.str() << "\n";
        return "";
    }}},
    {"input",{2,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(1,ScriptVariable::Type::Name);
        ARG_REQUIRES(0,ScriptVariable::Type::String);
        std::cout << args[0].string; std::cout.flush();
        std::string inp;
        std::getline(std::cin,inp);
        settings.variables[args[1].name] = ScriptVariable{inp};
        return "";
    }}},

    {"tonumber",{1,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::Name);
        if(settings.variables.count(args[0].name) == 0) {
            return "unknown variable: " + args[0].name;
        }
        ScriptVariable var = settings.variables[args[0].name];
        if(var.type == ScriptVariable::Type::Number) return "";
        long double num = 0;
        try {
            num = std::stold(var.string);
        }
        catch(...) {
            return "invalid input: \"" + var.string + "\"";
        }

        settings.variables[args[0].name] = ScriptVariable{num};
        return "";
    }}},
    {"tostring",{1,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::Name);
        if(settings.variables.count(args[0].name) == 0) {
            return "unknown variable: " + args[0].name;
        }
        ScriptVariable var = settings.variables[args[0].name];
        if(var.type == ScriptVariable::Type::String) return "";

        settings.variables[args[0].name] = ScriptVariable{std::to_string(var.number)};
        return "";
    }}},

    {"exec",{1,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::String);
        std::string f;
        std::ifstream ifile;
        ifile.open(args[0].string,std::ios::in);
        while(ifile.good()) f += ifile.get();
        if(!f.empty()) f.pop_back();
        std::cout << f << " " << args[0].string << "\n";
        ifile.close();
        return run_script(f);
    }}},
    {"exit",{1,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::Number);
        std::exit((int)args[0].number);
        return "";
    }}},
    {"sleep",{1,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::Number);
        cat_sleep((long)args[0].number);
        return "";
    }}},
    {"system",{1,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::String);
        system(args[0].string.c_str());
        return "";
    }}},

    {"add",{2,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::Name);
        ARG_REQUIRES(1,ScriptVariable::Type::String);
        std::string type = args[0].name;
        std::string file = args[1].string;

        if(type == "FILE") {
            std::ofstream ofile(file);
            ofile.close();
        }
        else if(type == "DIRECTORY") {
            std::filesystem::create_directory(file);
        }
        else if(type == "DEPENDENCY") {
            file = to_lowercase(app_username(file));
            std::string err = download_repo(file);
            if(err != "") {
                return "failed to downlad dependency \"" + file + "\": " + err;
            }
        }
        else {
            return "unknown type: " + type;
        }

        return "";
    }}},
    {"remove",{2,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::Name);
        ARG_REQUIRES(1,ScriptVariable::Type::String);
        std::string type = args[0].name;
        std::string file = args[1].string;

        if(type == "FILE") {
            try {
                std::filesystem::remove(file);
            }
            catch(...) {
                return "no such file";
            }
        }
        else if(type == "DIRECTORY") {
            try {
                std::filesystem::remove_all(file);
            }
            catch(...) {
                return "no such directory";
            }
        }
        else if(type == "DEPENDENCY") {
            file = to_lowercase(app_username(file));
            auto [usr,proj] = get_username(file);
            if(!std::filesystem::exists(CATCARE_ROOT + CATCARE_DIRSLASH + proj) || !installed(file)) {
                return "no such repo installed: " + file;
            }
            std::filesystem::remove_all(CATCARE_ROOT + CATCARE_DIRSLASH + proj);
            
            remove_from_register(file);
            // remove_from_dependencylist(file);
        }
        else {
            return "unknown type: " + type;
        }

        return "";
    }}},
    {"exists",{3,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::Name);
        ARG_REQUIRES(1,ScriptVariable::Type::String);
        ARG_REQUIRES(2,ScriptVariable::Type::Name);
        std::string type = args[0].name;
        std::string file = args[1].string;
        std::string var = args[2].name;

        if(type == "FILE") {
            settings.variables[var] = std::filesystem::exists(file) ? script_true : script_false;
        }
        else if(type == "DIRECTORY") {
            settings.variables[var] = std::filesystem::exists(file) ? script_true : script_false;
        }
        else if(type == "DEPENDENCY") {
            file = to_lowercase(app_username(file));
            settings.variables[var] = installed(file) ? script_true : script_false;
        }
        else if(type == "VARIABLE") {
            settings.variables[var] = settings.variables.count(file) != 0 ? script_true : script_false;
        }
        else {
            return "unknown type: " + type;
        }

        return "";
    }}},

    {"inipp",{5,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::Name); // type
        ARG_REQUIRES(1,ScriptVariable::Type::String); // file
        ARG_REQUIRES(2,ScriptVariable::Type::String); // key
        ARG_REQUIRES(3,ScriptVariable::Type::String); // section

        std::string type = args[0].name;
        std::string file = args[1].string;
        std::string key = args[2].string;
        std::string section = args[3].string;

        if(type == "SET") {
            NARG_REQUIRES(4,ScriptVariable::Type::Name); // value
            ScriptVariable var = args[4];
            IniFile ifile = IniFile::from_file(file);
            if(var.type == ScriptVariable::Type::String) {
                ifile.set(key,var.string,section);
            }
            else {
                ifile.set(key,var.number,section);
            }
            ifile.to_file(file);
        }
        else if(type == "GET") {
            ARG_REQUIRES(4,ScriptVariable::Type::Name); // var
            IniFile ifile = IniFile::from_file(file);
            IniElement elem = ifile.get(key,section);
            ScriptVariable var;
            if(elem.get_type() == IniType::Int || elem.get_type() == IniType::Float) {
                var.type = ScriptVariable::Type::Number;
                var.number = std::stold((std::string)elem);
            }
            else {
                var.type = ScriptVariable::Type::String;
                var.string = (std::string)elem;
            }
            settings.variables[args[4].name] = var;
        }
        else {
            return "unknown action: " + type;
        }

        return "";
    }}},

    {"read",{2,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::String);
        ARG_REQUIRES(1,ScriptVariable::Type::Name);
        std::string r;
        std::ifstream ifile;
        ifile.open(args[0].string);
        while(ifile.good()) r += ifile.get();
        if(!r.empty()) r.pop_back();

        settings.variables[args[1].name] = ScriptVariable{r};
        return "";
    }}},
    {"write",{2,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::String);
        ARG_REQUIRES(1,ScriptVariable::Type::String);
        std::ofstream ofile;
        ofile.open(args[0].string, std::ios::trunc);
        ofile.close(); ofile.open(args[0].string);
        ofile << args[1].string;
        ofile.close();
        return "";
    }}},
    
    {"download",{3,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::Name);
        ARG_REQUIRES(1,ScriptVariable::Type::String);
        ARG_REQUIRES(2,ScriptVariable::Type::String);
        
        std::string type = args[0].name;
        if(type == "URL") {
            if(!download_page(args[1].string,args[2].string)) {
                return "download failed";
            }
        }
        else if(type == "FILE") {
            std::string repo = to_lowercase(args[1].string);
            std::string file = args[2].string;

            if(!download_page(CATCARE_REPOFILE(args[1].string,args[2].string),args[2].string)) {
                return "download failed";
            }
        }
        else {
            return "unknown type: " + type;
        }

        return "";
    }}},
    {"project_info",{2,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::Name);
        ARG_REQUIRES(1,ScriptVariable::Type::Name);
        
        std::string type = args[0].name;
        IniFile file = IniFile::from_file(CATCARE_CHECKLISTNAME);
        ScriptVariable var;
        var.type = ScriptVariable::Type::String;
        if(type == "NAME") {
            var.string = (std::string)file.get("name","Info");
        }
        else if(type == "VERSION") {
            var.string = (std::string)file.get("version","Info");
        }
        else {
            return "unknown type: " + type;
        }
        settings.variables[args[1].name] = var;

        return "";
    }}},
    {"config",{2,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        ARG_REQUIRES(0,ScriptVariable::Type::Name);
        ARG_REQUIRES(1,ScriptVariable::Type::Name);
        
        std::string entry = args[0].name;
        IniFile file = IniFile::from_file(CATCARE_CHECKLISTNAME);
        ScriptVariable var;
        var.type = ScriptVariable::Type::String;

        if(options.count(entry) != 0) {
            var.string = options[entry];
        }
        else {
            return "unknown entry: " + entry;
        }
        settings.variables[args[1].name] = var;

        return "";
    }}},

    {"call",{-1,[](ScriptArglist args, ScriptSettings& settings)->std::string {
        DONTRUN_WIF();
        if(args.size() == 0) {
            return "requires at least one argument";
        }
        ARG_REQUIRES(0,ScriptVariable::Type::Name);
        std::vector<ScriptVariable> run_args;
        for(size_t i = 1; i < args.size(); ++i) {
            NARG_REQUIRES(i,ScriptVariable::Type::Name);
            run_args.push_back(args[i]);
        }
        
        if(settings.labels.count(args[0].name) == 0) {
            return "no such label";
        }
        ScriptLabel label = settings.labels[args[0].name];
        if(label.arglist.size() > run_args.size()) {
            return "too few arguments";
        }
        else if(label.arglist.size() < run_args.size()) {
            return "too many arguments";
        }

        settings.error_msg = run_script(args[0].name,settings.labels,"",run_args);
        settings.raw_error = true;
        return "";
    }}},
};

#undef ARG_REQUIRES
#undef NARG_REQUIRES
#undef DONTRUN_WIF

#define SIDE_MUSTBE(side,type_,sidestr,opstr) do{if(side.type != type_){ settings.error_msg = opstr ": " sidestr " must be of type " + scriptType2string(type_) + " (got: " + scriptType2string(side.type) + ")"; return script_null; } }while(0)
#define NSIDE_MUSTBE(side,type_,sidestr,opstr) do{if(side.type == type_){ settings.error_msg = opstr ": " sidestr " must not be of type " + scriptType2string(type_); return script_null; } }while(0)
#define SIDE_SAME(sidea,sideb,sidestra,sidestrb,opstr) do{if(sidea.type != sideb.type){ settings.error_msg = opstr ": " sidestra " must be same type as " sidestrb " (got: " + scriptType2string(sidea.type) + " | " + scriptType2string(sideb.type) + ")"; return script_null; } }while(0)

std::map<std::string,ScriptOperator> script_operators = {
    {"+",{0,0,[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_SAME(right,left,"right","left","+");
        NSIDE_MUSTBE(right,ScriptVariable::Type::Name,"right","+");
        ScriptVariable ret;
        if(right.type == ScriptVariable::Type::Number) {
            ret.type = ScriptVariable::Type::Number;
            ret.number = left.number + right.number;
        }
        else {
            ret.type = ScriptVariable::Type::String;
            ret.string = left.string + right.string;
        }
        return ret;
    }}},
    {"-",{0,2,[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_SAME(right,left,"right","left","-");
        SIDE_MUSTBE(right,ScriptVariable::Type::Number,"right","-");
        ScriptVariable ret;
        ret.type = ScriptVariable::Type::Number;
        ret.number = left.number - right.number;
        return ret;
    },[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_MUSTBE(left,ScriptVariable::Type::Number,"left","-");
        ScriptVariable ret;
        ret.type = ScriptVariable::Type::Number;
        ret.number = left.number * -1;
        return ret;
    }}},
    {"*",{1,0,[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_SAME(right,left,"right","left","*");
        SIDE_MUSTBE(right,ScriptVariable::Type::Number,"right","*");
        ScriptVariable ret;
        ret.type = ScriptVariable::Type::Number;
        ret.number = left.number * right.number;

        return ret;
    }}},
    {"/",{1,0,[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_SAME(right,left,"right","left","/");
        SIDE_MUSTBE(right,ScriptVariable::Type::Number,"right","/");
        if(right.number == 0) {
            settings.error_msg = "/: division through 0 is not allowed";
            return script_null;
        }
        ScriptVariable ret;
        ret.type = ScriptVariable::Type::Number;
        ret.number = left.number / right.number;
        return ret;
    }}},
    {"^",{1,0,[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_SAME(right,left,"right","left","^");
        SIDE_MUSTBE(right,ScriptVariable::Type::Number,"right","^");
        ScriptVariable ret;
        ret.type = ScriptVariable::Type::Number;
        ret.number = std::pow(left.number,right.number);
        return ret;
    }}},
    
    {"is",{-1,0,[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_SAME(right,left,"right","left","is");

        ScriptVariable ret;
        ret.type = ScriptVariable::Type::Number;
        if(right.type == ScriptVariable::Type::Number) {
            ret.number = left.number == right.number;
        }
        else if(right.type == ScriptVariable::Type::String) {
            ret.number = left.string == right.string;
        }
        else if(right.type == ScriptVariable::Type::Name) {
            ret.number = left.name == right.name;
        }
        return ret;
    }}},
    {"isnt",{-1,0,[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_SAME(right,left,"right","left","isnt");

        ScriptVariable ret;
        ret.type = ScriptVariable::Type::Number;
        if(right.type == ScriptVariable::Type::Number) {
            ret.number = left.number != right.number;
        }
        else if(right.type == ScriptVariable::Type::String) {
            ret.number = left.string != right.string;
        }
        else if(right.type == ScriptVariable::Type::Name) {
            ret.number = left.name != right.name;
        }
        return ret;
    }}},
    {"and",{-2,0,[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_SAME(right,left,"right","left","and");
        SIDE_MUSTBE(right,ScriptVariable::Type::Number,"right","and");
        ScriptVariable ret;
        ret.type = ScriptVariable::Type::Number;
        ret.number = left.number && right.number;
        return ret;
    }}},
    {"or",{-2,0,[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_SAME(right,left,"right","left","and");
        SIDE_MUSTBE(right,ScriptVariable::Type::Number,"right","and");
        ScriptVariable ret;
        ret.type = ScriptVariable::Type::Number;
        ret.number = left.number || right.number;
        return ret;
    }}},
    {"more",{-2,0,[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_SAME(right,left,"right","left","and");
        SIDE_MUSTBE(right,ScriptVariable::Type::Number,"right","and");
        ScriptVariable ret;
        ret.type = ScriptVariable::Type::Number;
        ret.number = left.number > right.number;
        return ret;
    }}},
    {"less",{-2,0,[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_SAME(right,left,"right","left","and");
        SIDE_MUSTBE(right,ScriptVariable::Type::Number,"right","and");
        ScriptVariable ret;
        ret.type = ScriptVariable::Type::Number;
        ret.number = left.number < right.number;
        return ret;
    }}},
    
    {"not",{99,1,nullptr,[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_MUSTBE(left,ScriptVariable::Type::Number,"left","not");
        ScriptVariable ret;
        ret.type = ScriptVariable::Type::Number;
        ret.number = !left.number;
        return ret;
    }}},
    {"$",{999,1,nullptr,[](ScriptVariable left, ScriptVariable right, ScriptSettings& settings)->ScriptVariable {
        SIDE_MUSTBE(left,ScriptVariable::Type::Name,"left","$");
        ScriptVariable ret;
        if(settings.variables.count(left.name) == 0) {
            settings.error_msg = "$: left is not a registered variable!";
            return script_null; 
        }
        ret = settings.variables[left.name];
        return ret;
    }}},
};

#undef SIDE_MUSTBE
#undef NSIDE_MUSTBE
#undef SIDE_SAME