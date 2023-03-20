#ifndef CARESCRIPT_PARSING_HPP
#define CARESCRIPT_PARSING_HPP

#include "carescript-defs.hpp"
#include "carescript-defaults.hpp"

#include <string.h>
#include <filesystem>
#include <variant>

// Implementation for the functions declared in "carescript-defs.hpp"

///
//TODO add get_ext() for windows (ugh)
//TODO way to define rules using tags
///

#ifdef _WIN32
# include <windows.h>
namespace carescript {
inline static Extension* get_ext(std::filesystem::path name) { return nullptr; }
#elif defined(__linux__)
# include <dlfcn.h>
namespace carescript {
inline static Extension* get_ext(std::filesystem::path name) {
    if(!name.has_extension()) name += ".so";
    if(name.is_relative())
        name = "./" + name.string();
    void* handler = dlopen(name.c_str(),RTLD_NOW);
    if(handler == nullptr) return nullptr;
    get_extension_fun f = (get_extension_fun)dlsym(handler,"get_extension");
    if(f == nullptr) return nullptr;
    return f();
}
#endif

inline bool bake_extension(std::string name, ScriptSettings& settings) {
    Extension* ext = get_ext(name);
    return bake_extension(ext,settings);
}

inline bool bake_extension(Extension* ext, ScriptSettings& settings) {
    if(ext == nullptr) return false;

    BuiltinList b_list = ext->get_builtins();
    settings.interpreter.script_builtins.insert(b_list.begin(),b_list.end());
    TypeList t_list = ext->get_types();
    for(auto i : t_list) settings.interpreter.script_typechecks.push_back(i);
    OperatorList o_list = ext->get_operators();
    for(auto i : o_list) {
        for(auto j : i.second) {
            settings.interpreter.script_operators[i.first].push_back(j);
        }
    }
    MacroList m_list = ext->get_macros();
    settings.interpreter.script_macros.insert(m_list.begin(),m_list.end());
    return true;
}

inline std::string run_script(std::string source,ScriptSettings& settings) {
    auto labels = pre_process(source,settings);
    if(settings.error_msg != "") {
        return settings.error_msg;
    }
    settings.line = 1;
    std::string ret = run_label("main",labels,settings,std::filesystem::current_path().parent_path(),{});
    settings.exit = false;
    return ret;
}

inline std::string run_label(std::string label_name, std::map<std::string,ScriptLabel> labels, ScriptSettings& settings, std::filesystem::path parent_path, std::vector<ScriptVariable> args) {
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
    settings.label.push(label_name);
    for(auto i : lines) 
        if(i.size() != 2 || i[0].str || i[1].str || i[1].src.front() != '(') { 
            settings.label.pop();
            return "line " + std::to_string(i.front().line) + " is invalid (in label " + label_name + ")"; 
        }

    settings.parent_path = parent_path;
    settings.labels = labels;

    for(size_t i = 0; i < args.size(); ++i) {
        settings.variables[label.arglist[i]] = args[i];
    }
    if(settings.line == 0) settings.line = 1;
    for(size_t i = settings.line-1; i < lines.size(); ++i) {
        if(settings.exit) return "";
        i = settings.line-1;
        std::string name = lines[i][0].src;
        auto arglist = parse_argumentlist(lines[i][1].src,settings);
        if(settings.error_msg != "") {
            settings.label.pop();
            if(settings.raw_error) return settings.error_msg;
            return "line " + std::to_string(settings.line + label.line) + ": " + settings.error_msg + " (in label " + label_name + ")";
        }
        if(settings.interpreter.script_builtins.count(name) == 0) {
            settings.label.pop();
            return "line " + std::to_string(settings.line + label.line) + ": unknown function: " + name + " (in label " + label_name + ")";
        }
        ScriptBuiltin builtin = settings.interpreter.script_builtins[name];
        if(builtin.arg_count != arglist.size() && builtin.arg_count >= 0) {
            settings.label.pop();
            return "line " + std::to_string(lines[i][0].line + label.line) + " " + name + " has invalid argument count " + " (in label " + label_name + ")";
        }
        builtin.exec(arglist,settings);
        if(settings.error_msg != "") {
            settings.label.pop();
            if(settings.raw_error) return settings.error_msg;
            return "line " + std::to_string(settings.line + label.line) + ": " + name + ": " + settings.error_msg + " (in label " + label_name + ")";
        }
        ++settings.line;
    }
    settings.label.pop();
    return "";
}

inline static bool is_operator_char(char c) {
    return c == '+' ||
           c == '-' ||
           c == '*' ||
           c == '/' ||
           c == '^' ||
           c == '%' ||
           c == '$';
}

inline static bool is_name_char(char c) {
    return (c <= 'Z' && c >= 'A') || 
            (c <= 'z' && c >= 'a') ||
            (c <= '9' && c >= '0') ||
            c == '_';
}

inline static bool is_name(std::string s) {
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

inline static bool is_label_arglist(std::string s) {
    if(s.size() < 2) return false;
    if(s[0] != '[' || s.back() != ']') return false;
    s.pop_back();
    s.erase(s.begin());

    KittenLexer arglist_lexer = KittenLexer()
        .add_extract(',')
        .ignore_backslash_opts()
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

inline static std::vector<std::string> parse_label_arglist(std::string s) {
    if(!is_label_arglist(s)) return {};
    s.pop_back();
    s.erase(s.begin());

    std::vector<std::string> ret;
    KittenLexer arglist_lexer = KittenLexer()
        .add_extract(',')
        .add_ignore(' ')
        .add_ignore('\t')
        .ignore_backslash_opts()
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

inline std::vector<ScriptVariable> parse_argumentlist(std::string source, ScriptSettings& settings) {
    KittenLexer arg_lexer = KittenLexer()
        .add_capsule('(',')')
        .add_capsule('[',']')
        .add_capsule('{','}')
        .add_stringq('"')
        .add_ignore(' ')
        .add_ignore('\t')
        .add_ignore('\n')
        .ignore_backslash_opts()
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
            else if(settings.interpreter.script_macros.count(i.src) != 0) i.src = settings.interpreter.script_macros.at(i.src);

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

inline static bool is_operator(std::string src, ScriptSettings& settings) {
    return settings.interpreter.script_operators.count(src) != 0;
}

// TODO
/*inline ScriptVariable try_merging(lexed_kittens tokens, ScriptSettings& settings, std::vector<std::string>& error_msgs) {
    ScriptVariable r;
    lexed_kittens current = tokens;
    struct mergable { size_t pos = 0, length = 0; };
    std::vector<mergable> mergables;

    bool f = false;
    for(size_t i = 0; i < current.size(); ++i) {
        if(!current[i].str && is_operator(current[i].src,settings)) {
            if(f) ++mergables.back().length;
            else mergables.push_back({i,1});
            f = true;
        }
        else f = false;
    }

    for(size_t i = 0; i < mergables.size(); ++i) {
        for(size_t j = i; j < mergables.size(); ++j) {
            for(size_t k = 0; k < mergables[i].length; ++k) {
                for(size_t ki = k; ki < mergables[i].length; ++ki) {

                }
            }
        }
    }
}*/

struct _ExpressionErrors {
    std::vector<std::string> messages;
    bool has_new = false;
    
    _ExpressionErrors& push(std::string msg) {
        messages.push_back(msg);
        has_new = true;
        return *this;
    }

    bool changed() { return has_new; }
    void reset() { has_new = false; }

    operator std::vector<std::string>() { return messages; }
};
struct _ExpressionToken { std::string tk; ScriptOperator op; };
struct _ExpressionFuncall { 
    std::string function; 
    std::string arguments; 

    ScriptVariable call(ScriptSettings& settings, _ExpressionErrors& errors) {
        ScriptArglist args = parse_argumentlist(arguments,settings);
        ScriptBuiltin fun = settings.interpreter.get_builtin(function);
        if(settings.error_msg != "") {
            errors.push("error parsing argumentlist: " + settings.error_msg);
            settings.error_msg = "";
            return script_null;
        }
        if(fun.arg_count >= 0) {
            if(args.size() < fun.arg_count) {
                errors.push("function call with too little arguments: " + function + arguments + 
                    "\n- needs: " + std::to_string(fun.arg_count) + " got: " + std::to_string(args.size()));
                return script_null;
            }
            if(args.size() > fun.arg_count) {
                errors.push("function call with too many arguments: " + function + arguments +
                    "\n- needs: " + std::to_string(fun.arg_count) + " got: " + std::to_string(args.size()));
                return script_null;
            }
        }
        settings.error_msg = "";
        ScriptVariable ret =  fun.exec(args,settings);
        if(settings.error_msg != "") errors.push(settings.error_msg);
        return ret;
    }
};
struct _OperatorToken { 
    ScriptVariable val;
    _ExpressionToken op;
    _ExpressionFuncall call;
    std::string capsule;
    enum { OP, VAL, CALL, CAPSULE } type;

    _OperatorToken(ScriptVariable v): val(v) { type = VAL; }
    _OperatorToken(_ExpressionToken v): op(v) { type = OP; }
    _OperatorToken(_ExpressionFuncall v): call(v) { type = CALL; }
    _OperatorToken(std::string v): capsule(v) { type = CAPSULE; }
    _OperatorToken() = delete;

    ScriptVariable get_val(ScriptSettings& settings, _ExpressionErrors& errors) {
        switch(type) {
            case VAL:
                return val;
            case CALL:
                return call.call(settings,errors);
            case CAPSULE:
                {
                    std::string capsule_copy = capsule;
                    capsule_copy.erase(capsule_copy.begin());
                    capsule_copy.pop_back();
                    ScriptVariable value = evaluate_expression(capsule_copy,settings);
                    if(settings.error_msg != "") {
                        errors.push("Error while parsing " + capsule + ": " + settings.error_msg);
                        settings.error_msg = "";
                        return script_null;
                    }
                    return value;
                }
        }
        return script_null;
    }
};

inline std::vector<_OperatorToken> expression_prepare_tokens(lexed_kittens& tokens, ScriptSettings& settings, _ExpressionErrors& errors) {
    std::vector<_OperatorToken> ret;
    for(size_t i = 0; i < tokens.size(); ++i) {
        auto token = tokens[i];
        std::string r = token.src;
        if(!token.str && is_operator(token.src,settings)) {
            ret.push_back(_ExpressionToken{r,ScriptOperator()});
        }
        else if(!token.str && token.src[0] == '(') {
            ret.push_back(token.src);
        }
        else if(!token.str && settings.interpreter.has_builtin(token.src)) {
            ScriptBuiltin builtin = settings.interpreter.get_builtin(token.src);
            if(i + 1 >= tokens.size() || tokens[i+1].str) {
                errors.push("function call without argument list");
                return {};
            }
            KittenToken arguments = tokens[i+1];
            if(arguments.str || arguments.src.front() != '(') {
                errors.push("function call with invalid argument list: " + tokens[i].src + " " +tokens[i+1].src);
                return {};
            }

            ret.push_back(_ExpressionFuncall{tokens[i].src, tokens[i+1].src});
            ++i;
        }
        else {
            ret.push_back(to_var(token,settings));
            if(is_null(ret.back().val)) {
                if(token.str) token.src = "\"" + token.src + "\"";
                errors.push("invalid literal: " + token.src);
            }
        }
    }

    return ret;
}

inline static ScriptVariable expression_check_prec(std::vector<_OperatorToken> markedupTokens, int& state, const int maxprec, ScriptSettings& settings, _ExpressionErrors& errors) {
    if(errors.changed()) return script_null;
    if(state >= markedupTokens.size()) {
        errors.push("Unexpected end of expression");
        return script_null;
    }
    _OperatorToken lhs = markedupTokens[state++];
        
    if(lhs.type == lhs.OP) {
        lhs = lhs.op.op.run(expression_check_prec(markedupTokens, state, lhs.op.op.priority, settings, errors),script_null,settings);
        if(settings.error_msg != "") 
            errors.push(settings.error_msg);  
        if(errors.changed()) return script_null;
    }

    while(state < markedupTokens.size()) {
        _OperatorToken vop = markedupTokens[state];
        if(vop.type != vop.OP) { 
            errors.push("expected operator: " + vop.val.printable());
            return script_null; 
        }
        auto op = vop.op.op;
        if(op.type != op.BINARY) { 
            errors.push("expected binary operator: " + vop.op.tk);
            return script_null; 
        }
        if(op.priority >= maxprec)
            break;
        ++state;
        ScriptVariable rhs = expression_check_prec(markedupTokens, state, op.priority, settings, errors);
        if(errors.changed()) return script_null;

        ScriptVariable old_lhs = lhs.get_val(settings,errors);
        if(errors.changed()) return script_null;
        lhs.type = lhs.VAL;
        lhs.val = op.run(old_lhs, rhs, settings);
        if(settings.error_msg != "") {
            errors.push(old_lhs.printable() + " " + vop.op.tk + " " + rhs.printable() + ": " + settings.error_msg);
            settings.error_msg = "";
            return script_null;
        }
    }
    return lhs.get_val(settings,errors);
}

inline bool valid_expression(std::vector<_OperatorToken> markedupTokens, ScriptSettings& settings) {
    const static int max_prec = 999999999;
    _ExpressionErrors errs;
    try {
        int i = 0;
        expression_check_prec(markedupTokens,i,max_prec,settings,errs);
    }
    catch(...) {
      return false;
    }
    return errs.changed();
}

inline ScriptVariable expression_force_parse(std::vector<_OperatorToken> markedupTokens, ScriptSettings& settings, _ExpressionErrors& errors, int i = 0) {
    const static int max_prec = 999999999;
    if(errors.changed()) return script_null;
    if(markedupTokens.size() == 1) {
        if(markedupTokens[0].type == markedupTokens[0].OP) {
            errors.push("standalone operator detected!");
            return script_null;
        }
        return markedupTokens[0].get_val(settings,errors);
    }
    if(i >= markedupTokens.size()) {
        int i = 0;
        return expression_check_prec(markedupTokens,i,max_prec,settings,errors);
    }

    if(markedupTokens[i].type == markedupTokens[i].OP) {
        markedupTokens[i].op.op.type = ScriptOperator::UNARY;
    } 
    else {
        ++i;
        if(i >= markedupTokens.size()) {
            int i = 0;
            return expression_check_prec(markedupTokens,i,max_prec,settings,errors); 
        }
        markedupTokens[i].op.op.type = ScriptOperator::BINARY;
    }

    for(auto option : settings.interpreter.script_operators[markedupTokens[i].op.tk]) {
        auto op = markedupTokens[i].op.op;
        if(option.type != op.type)
            continue;
        auto newMarkedupTokens = markedupTokens;
        newMarkedupTokens[i].op.op.priority = option.priority;
        newMarkedupTokens[i].op.op.run = option.run;
        auto test = expression_force_parse(newMarkedupTokens, settings, errors, i + 1);
        if(!is_null(test) && !errors.changed()) {
            return test;
        }
        errors.reset();
    }
    return script_null;
}

inline ScriptVariable evaluate_expression(std::string source, ScriptSettings& settings) {
    KittenLexer expression_lexer = KittenLexer()
        .add_stringq('"')
        .add_capsule('(',')')
        .add_capsule('[',']')
        .add_capsule('{','}')
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
    _ExpressionErrors errors;

    auto result = expression_force_parse(expression_prepare_tokens(lexed,settings,errors),settings,errors);

    if(errors.changed() || is_null(result)) {
        settings.error_msg = "\nError in expression: " + source + "\n";
        for(auto i : errors.messages) {
            settings.error_msg += i + "\n";
        }
        if(!errors.messages.empty()) settings.error_msg.pop_back();
        return script_null;
    }
    return result;
}


inline void parse_const_preprog(std::string source, ScriptSettings& settings) {
    std::vector<lexed_kittens> lines;
    KittenLexer lexer = KittenLexer()
        .add_stringq('"')
        .add_capsule('(',')')
        .add_capsule('[',']')
        .add_ignore(' ')
        .add_ignore('\t')
        .add_linebreak('\n')
        .add_lineskip('#')
        .add_con_extract(is_operator_char)
        .ignore_backslash_opts()
        .erase_empty();

    auto lexed = lexer.lex(source);
    int line = -1;
    for(auto i : lexed) {
        if(i.line != line) {
            line = i.line;
            lines.push_back({});
        }
        lines.back().push_back(i);
    }
    for(auto i : lines) {
        if(i.size() < 3 || i[0].str || i[1].str || !is_name(i[0].src) || i[1].src != "=") {
            settings.error_msg = "invalid syntax. <name> = <expression...>";
            return;
        }
        std::string name = i[0].src;
        std::string line;
        for(size_t j = 2; j < i.size(); ++j) {
            if(i[j].str) i[j].src = "\"" + i[j].src + "\"";
            line += i[j].src + " ";
        }
        line.pop_back();
        settings.constants[name] = evaluate_expression(line,settings);
        if(settings.error_msg != "") {
            return;
        }
    }
}

inline std::map<std::string,ScriptLabel> pre_process(std::string source, ScriptSettings& settings) {
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
        .ignore_backslash_opts()
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
    for(size_t i = 0; i < lines.size(); ++i) {
        auto& line = lines[i];
        if(line.size() != 0 && line[0].src == "@" && !line[0].str) {
            if(line.size() != 3) {
                settings.error_msg = "line " + std::to_string(i+1) + ": invalid pre processor instruction: must have 2 arguments (got: " + std::to_string(line.size()-1) + ")";
                return {};
            }
            if(!is_name(line[1].src) || line[1].str) {
                settings.error_msg = "line " + std::to_string(i+1) + ": invalid pre processor instruction: expected instruction";
                return {};
            }
            std::string inst = line[1].src;

            if(inst == "const") {
                auto body = line[2].src;
                if(line[2].str) {
                    settings.error_msg = "line " + std::to_string(i+1) + ": const: unexpected string";
                    return {};
                }
                if(body.size() < 2) {
                    settings.error_msg = "line " + std::to_string(i+1) + ": const: unexpected token";
                    return {};
                }
                if(body.front() != '[' || body.back() != ']') {
                    settings.error_msg = "line " + std::to_string(i+1) + ": const: expected body";
                    return {};
                }
                body.erase(body.begin());
                body.erase(body.end()-1);
                parse_const_preprog(body,settings);
                if(settings.error_msg != "") {
                    settings.error_msg = "line " + std::to_string(i+1) + ": const: line " + std::to_string(settings.line) + ": " + settings.error_msg;
                    return {};
                }
            }
            else if(inst == "bake") {
                KittenLexer bake_lexer = KittenLexer()
                    .add_stringq('"')
                    .erase_empty()
                    .add_ignore(' ')
                    .add_ignore('\t')
                    .add_ignore('\n')
                    ;
                auto body = line[2].src;
                if(line[2].str) {
                    settings.error_msg = "line " + std::to_string(i+1) + ": bake: unexpected string";
                    return {};
                }
                if(body.size() < 2) {
                    settings.error_msg = "line " + std::to_string(i+1) + ": bake: unexpected token";
                    return {};
                }
                if(body.front() != '[' || body.back() != ']') {
                    settings.error_msg = "line " + std::to_string(i+1) + ": bake: expected body";
                    return {};
                }
                body.erase(body.begin());
                body.erase(body.end()-1);
                auto lexed = bake_lexer.lex(body);
                for(auto b : lexed) {
                    if(!b.str) {
                        settings.error_msg = "line " + std::to_string(i+1) + ": bake: expected value: " + b.src;
                        return {};
                    }
                    if(!bake_extension(b.src,settings)) {
                        settings.error_msg = "line " + std::to_string(i+1) + ": bake: error baking extension: " + b.src + "\n" + dlerror(); 
                        return {};
                    }
                }
            }
            else if(is_label_arglist(line[2].src) && !line[2].str) {
                if(ret.count(line[1].src) != 0) {
                    settings.error_msg = "line " + std::to_string(i+1) + ": can't open label twice: " + line[1].src;
                }
                current_label = line[1].src;
                ret[current_label].arglist = parse_label_arglist(line[2].src);
                ret[current_label].line = line[1].line;
            }
            else {
                settings.error_msg = "line " + std::to_string(i+1) + ": invalid pre processor instruction: no match for: " + inst;
                return {};
            }
        }
        else {
            for(auto j : line) ret[current_label].lines.push_back(j);
        }
    }

    return ret;
}

} /* namespace carescript */

#endif