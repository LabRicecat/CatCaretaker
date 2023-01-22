#ifndef CARESCRIPT_HPP
#define CARESCRIPT_HPP

#include "../mods/kittenlexer.hpp"

#include <cmath>
#include <fstream>
#include <filesystem>

struct ScriptVariable {
    enum class Type {
        Number,
        String,
        Name,
        Null,
    }type;

    std::string string;
    long double number = 0;
    std::string name;

    ScriptVariable() {}
    ScriptVariable(long double num): number(num) { type = Type::Number; }
    ScriptVariable(std::string str): string(str) { type = Type::String; }
    ScriptVariable(Type ty): type(ty) { }
};

ScriptVariable to_var(KittenToken src);

const ScriptVariable script_null = ScriptVariable{ScriptVariable::Type::Null};
const ScriptVariable script_true = ScriptVariable{true};
const ScriptVariable script_false = ScriptVariable{false};

inline std::string scriptType2string(ScriptVariable::Type type) {
    return std::vector<std::string>{
        "Number",
        "String",
        "Name",
        "Null",
    }[static_cast<int>(type)];
}

struct ScriptLabel;
struct ScriptSettings {
    int line = 0;
    bool exit = false;
    std::stack<bool> should_run;
    std::map<std::string,ScriptVariable> variables;
    std::map<std::string,ScriptLabel> labels;
    std::filesystem::path parent_path;
    int ignore_endifs = 0;
    ScriptVariable return_value = script_null;

    std::string error_msg;
    bool raw_error = false;
};

const std::map<std::string,std::string> script_macros = {
#ifdef _WIN32
    {"WINDOWS","1"},
    {"LINUX","0"},
    {"UNKNWON","0"},
    {"OSNAME","\"WINDOWS\""},
#elif defined(__linux__)
    {"WINDOWS","0"},
    {"LINUX","1"},
    {"UNKNWON","0"},
    {"OSNAME","\"LINUX\""},
#elif
    {"WINDOWS","0"},
    {"LINUX","0"},
    {"UNKNWON","1"},
    {"OSNAME","\"UNKNOWN\""},
#endif
};

struct ScriptOperator {
    int priority = 0;
    // 0 => no
    // 1 => yes
    // 2 => both
    int unary = 0;
    ScriptVariable(*run)(ScriptVariable left, ScriptVariable right, ScriptSettings& settings);

    ScriptVariable(*run_unary)(ScriptVariable left, ScriptVariable right, ScriptSettings& settings);
};

using ScriptArglist = std::vector<ScriptVariable>;

struct ScriptBuiltin {
    int arg_count = -1;
    ScriptVariable(*exec)(ScriptArglist,ScriptSettings&);
};

struct ScriptLabel {
    std::vector<std::string> arglist;
    lexed_kittens lines;
};

std::string run_script(std::string source);
std::string run_script(std::string label_name, std::map<std::string,ScriptLabel> label, std::filesystem::path parent_path = "", std::vector<ScriptVariable> args = {});
std::string run_script(std::string label_name, std::map<std::string,ScriptLabel> labels, std::filesystem::path parent_path , std::vector<ScriptVariable> args, ScriptSettings& settings);
std::map<std::string,ScriptLabel> into_labels(std::string source);
std::vector<ScriptVariable> parse_argumentlist(std::string source, ScriptSettings& settings);
ScriptVariable evaluate_expression(std::string source, ScriptSettings& settings);
void get_globalload(lexed_kittens source, ScriptSettings& settings);

extern inline std::map<std::string,ScriptBuiltin> script_builtins;
extern inline std::map<std::string,ScriptOperator> script_operators;

#endif