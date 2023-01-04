#ifndef CARESCRIPT_HPP
#define CARESCRIPT_HPP

#include "../mods/kittenlexer.hpp"

#include <cmath>
#include <fstream>

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
};

ScriptVariable to_var(KittenToken src);

const ScriptVariable script_null = ScriptVariable{ScriptVariable::Type::Null};
const ScriptVariable script_true = ScriptVariable{ScriptVariable::Type::Number,"",true};
const ScriptVariable script_false = ScriptVariable{ScriptVariable::Type::Number,"",false};

inline std::string scriptType2string(ScriptVariable::Type type) {
    return std::vector<std::string>{
        "Number",
        "String",
        "Name",
        "Null",
    }[static_cast<int>(type)];
}

struct ScriptSettings {
    int line = 0;
    bool exit = false;
    std::stack<bool> should_run;
    std::map<std::string,ScriptVariable> variables;
    int ignore_endifs = 0;

    std::string error_msg;
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
    {"UNKNOWN","\"WINDOWS\""},
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
    std::string(*exec)(ScriptArglist,ScriptSettings&);
};

std::string run_script(std::string source);
std::vector<ScriptVariable> parse_argumentlist(std::string source, ScriptSettings& settings);
ScriptVariable evaluate_expression(std::string source, ScriptSettings& settings);


extern inline std::map<std::string,ScriptBuiltin> builtins;

extern inline std::map<std::string,ScriptOperator> operators;

#endif