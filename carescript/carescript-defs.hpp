#ifndef CARESCRIPT_DEFS_HPP
#define CARESCRIPT_DEFS_HPP

#include <cmath>
#include <fstream>
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <filesystem>
#include <exception>
#include <functional>
#include <any>

#include "../mods/kittenlexer.hpp"

#include "carescript-types.hpp"
#include "carescript-macromagic.hpp"

namespace carescript {

template<typename _Tp>
concept ScriptValueType = std::is_base_of<carescript::ScriptValue,_Tp>::value;

// Wrapper class to perfom tasks on a
// subclass of the abstract class "ScriptValue"
struct ScriptVariable {
    std::unique_ptr<ScriptValue> value = nullptr;

    bool operator==(const ScriptVariable& sv) const {
        return *sv.value == *value;
    }
    
    ScriptVariable() {}
    ScriptVariable(ScriptValue* ptr): value(ptr) {}
    ScriptVariable(const ScriptVariable& var) {
        if(var.value.get() != nullptr) value.reset(var.value->copy());
    }

    template<typename _Tp>
    ScriptVariable(_Tp a) {
        // note here: we use `from` but it's class internally only
        // defined for ScriptValue pointers.
        // the user can define other `from` functions inside the
        // `carescript` namespace to effectivly overload the
        // constructor of this class
        from(*this,a);
    }

    ScriptVariable& operator=(const ScriptVariable& var) {
        if(var.value.get() != nullptr) value.reset(var.value->copy());
        return *this;
    }

    std::string get_type() const {
        return value.get()->get_type();
    }
    std::string printable() const {
        return value.get()->to_printable();
    }
    std::string string() const {
        return value.get()->to_string();
    }

    template<typename _Tp>
    operator _Tp() {
        return get_value<_Tp>(*this);
    }

    inline friend void from(ScriptVariable& var, ScriptValue* a) {
        var.value.reset(a);
    }
};


// checks if a variable has a specific type
template<ScriptValueType _Tval>
inline bool is_typeof(const carescript::ScriptVariable& var) {
    _Tval inst;
    return var.get_type() == inst.get_type();
}

// checks if two subclasses of ScriptValue are the same
template<ScriptValueType _Tp1, ScriptValueType _Tp2>
inline bool is_same_type() {
    _Tp1 v1;
    _Tp2 v2;
    return v1->get_type() == v2->get_type();
}

// checks if two ScriptVariable instances have the same type
inline bool is_same_type(const ScriptVariable& v1,const ScriptVariable& v2) {
    return v1.get_type() == v2.get_type();
}

// checks if a variable is null
inline bool is_null(const ScriptVariable& v) {
    return v.value.get() == nullptr || is_typeof<ScriptNullValue>(v);
}

// returns the unwrapped type of a variable
template<typename _Tp>
inline auto get_value(const carescript::ScriptVariable& v) {
    return ((const _Tp*)v.value.get())->get_value();
}

const ScriptVariable script_null = new ScriptNullValue();
const ScriptVariable script_true = new ScriptNumberValue(true);
const ScriptVariable script_false = new ScriptNumberValue(false);

struct Interpreter;
struct ScriptLabel;
// general storage class for the current state of execution
struct ScriptSettings {
    Interpreter& interpreter;
    int line = 0;
    bool exit = false;
    std::stack<bool> should_run;
    std::map<std::string,ScriptVariable> variables;
    std::map<std::string,ScriptVariable> constants;
    std::map<std::string,ScriptLabel> labels;
    std::filesystem::path parent_path;
    int ignore_endifs = 0;
    ScriptVariable return_value = script_null;

    std::string error_msg;
    bool raw_error = false;
    std::stack<std::string> label;

    std::map<std::string,std::any> storage;

    ScriptSettings(Interpreter& i): interpreter(i) {}
};

using ScriptTypeCheck = ScriptValue*(*)(KittenToken src, ScriptSettings& settings);

// storage class for an operator
struct ScriptOperator {
    int priority = 0;
    enum {UNARY, BINARY, UNKNOWN} type;
    ScriptVariable(*run)(ScriptVariable left, ScriptVariable right, ScriptSettings& settings);
};

using ScriptArglist = std::vector<ScriptVariable>;
using ScriptMacro = std::pair<std::string,std::string>;

// storage class for a builtin function
struct ScriptBuiltin {
    int arg_count = -1;
    ScriptVariable(*exec)(const ScriptArglist&,ScriptSettings&);
};

// storage class for a label
struct ScriptLabel {
    std::vector<std::string> arglist;
    lexed_kittens lines;
    int line = 0;
};

extern std::map<std::string,ScriptBuiltin> default_script_builtins;
extern std::map<std::string,std::vector<ScriptOperator>> default_script_operators;
extern std::vector<ScriptTypeCheck> default_script_typechecks;
extern std::unordered_map<std::string,std::string> default_script_macros;

// bakes an extension into the interpreter
bool bake_extension(std::string name, ScriptSettings& settings);
class Extension;
bool bake_extension(Extension* extension, ScriptSettings& settings);

// runs a "main" function of a script
std::string run_script(std::string source, ScriptSettings& settings);
// runs a specific label with the given parameters
std::string run_label(std::string label_name, std::map<std::string,ScriptLabel> labels, ScriptSettings& settings, std::filesystem::path parent_path , std::vector<ScriptVariable> args);

// preprocesses the file into the interpreter
std::map<std::string,ScriptLabel> pre_process(std::string source, ScriptSettings& settings);
std::vector<ScriptVariable> parse_argumentlist(std::string source, ScriptSettings& settings);
// evaluates an expression and returns the result
ScriptVariable evaluate_expression(std::string source, ScriptSettings& settings);
void parse_const_preprog(std::string source, ScriptSettings& settings);

class Interpreter;
// storage class to temporarily store states of the interpreter
struct InterpreterState {
    std::map<std::string,ScriptBuiltin> script_builtins;
    std::map<std::string,std::vector<ScriptOperator>> script_operators;
    std::vector<ScriptTypeCheck> script_typechecks;
    std::unordered_map<std::string,std::string> script_macros;

    InterpreterState() {}
    InterpreterState(const Interpreter& interp) { save(interp); }
    InterpreterState(
        const std::map<std::string,ScriptBuiltin>& a,
        const std::map<std::string,std::vector<ScriptOperator>>& b,
        const std::vector<ScriptTypeCheck>& c,
        const std::unordered_map<std::string,std::string>& d):
        script_builtins(a),
        script_operators(b),
        script_typechecks(c),
        script_macros(d) {}

    void load(Interpreter& interp) const;
    void save(const Interpreter& interp);

    InterpreterState& operator=(const std::map<std::string,ScriptBuiltin>& a) {
        script_builtins = a;
        return *this;
    }
    InterpreterState& operator=(const std::map<std::string,std::vector<ScriptOperator>>& a) {
        script_operators = a;
        return *this;
    }
    InterpreterState& operator=(const std::vector<ScriptTypeCheck>& a) {
        script_typechecks = a;
        return *this;
    }
    InterpreterState& operator=(const std::unordered_map<std::string,std::string>& a) {
        script_macros = a;
        return *this;
    }

    InterpreterState& add(const std::map<std::string,ScriptBuiltin>& a) {
        script_builtins.insert(a.begin(),a.end());
        return *this;
    }
    InterpreterState& add(const std::map<std::string,std::vector<ScriptOperator>>& a) {
        for(auto& i : script_operators) {
            for(auto j : a.at(i.first)) {
                i.second.push_back(j);
            }
        }
        return *this;
    }
    InterpreterState& add(const std::vector<ScriptTypeCheck>& a) {
        for(auto i : a) {
            script_typechecks.push_back(i);
        }
        return *this;
    }
    InterpreterState& add(const std::unordered_map<std::string,std::string>& a) {
        script_macros.insert(a.begin(),a.end());
        return *this;
    }
};

// helper class for handling errors
struct InterpreterError  {
private:
    bool has_value = false;
    ScriptVariable value;
public:
    Interpreter& interpreter;
    InterpreterError(Interpreter& i): interpreter(i) {}
    InterpreterError(Interpreter& i, ScriptVariable val): interpreter(i), value(val) { has_value = true; }
    InterpreterError& on_error(std::function<void(Interpreter&)>);
    InterpreterError& otherwise(std::function<void(Interpreter&)>);
    InterpreterError& throw_error();
    ScriptVariable get_value() { return value; }
    ScriptVariable get_value_or(ScriptVariable var) { return has_value ? value : var; }
    Interpreter& chain() { return interpreter; }
};

// wrapper and storage class for a simpler API usage
class Interpreter {
    std::map<int,InterpreterState> states;
    std::function<void(Interpreter&)> on_error_f;

    void error_check() {
        if(settings.error_msg != "" && on_error_f) on_error_f(*this);
    }
public:
    std::map<std::string,ScriptBuiltin> script_builtins = default_script_builtins;
    std::map<std::string,std::vector<ScriptOperator>> script_operators = default_script_operators;
    std::vector<ScriptTypeCheck> script_typechecks = default_script_typechecks;
    std::unordered_map<std::string,std::string> script_macros = default_script_macros;
    ScriptSettings settings = ScriptSettings(*this);
    
    void save(int id) {
        states[id].save(*this);
    }

    void load(int id) {
        states[id].load(*this);
    }

    void clear() {
        script_builtins.clear();
        script_operators.clear();
        script_typechecks.clear();
        script_macros.clear();
    }

    operator bool() {
        return settings.error_msg == "";
    }
    operator ScriptSettings&() {
        return settings;
    }

    InterpreterError pre_process(std::string source) {
        settings.error_msg = "";
        settings.labels = ::carescript::pre_process(source,settings);
        error_check();
        return *this;
    }

    InterpreterError run() {
        settings.return_value = script_null;
        settings.line = 1;
        settings.error_msg = run_label("main",settings.labels,settings,"",{});
        settings.exit = false;
        error_check();
        return is_null(settings.return_value) ? *this : InterpreterError(*this,settings.return_value);
    }

    template<typename... _Targs>
    InterpreterError run(std::string label, _Targs ...targs) {
        std::vector<ScriptVariable> args = {targs...};
        return run(label,args);
    }
    InterpreterError run(std::string label, std::vector<ScriptVariable> args) {
        settings.return_value = script_null;
        settings.line = 1;
        settings.exit = false;
        settings.error_msg = run_label(label,settings.labels,settings,"",args);
        error_check();
        return is_null(settings.return_value) ? *this : InterpreterError(*this,settings.return_value);
    }

    InterpreterError eval(std::string source) {
        settings.return_value = script_null;
        settings.error_msg = run_script(source,settings);
        settings.exit = false;
        error_check();
        return is_null(settings.return_value) ? *this : InterpreterError(*this,settings.return_value);
    }

    int to_local_line(int line) { return line - settings.labels[settings.label.top()].line; }
    int to_global_line(int line) { return line + settings.labels[settings.label.top()].line; }

    void on_error(std::function<void(Interpreter&)> fun) {
        on_error_f = fun;
    }

    std::string error() const { return settings.error_msg; }

    Interpreter& add_builtin(std::string name, const ScriptBuiltin& builtin) {
        script_builtins[name] = builtin;
        return *this;
    }
    Interpreter& add_operator(std::string name, const ScriptOperator& _operator) {
        script_operators[name].push_back(_operator);
        return *this;
    }
    Interpreter& add_typecheck(const ScriptTypeCheck& typecheck) {
        script_typechecks.push_back(typecheck);
        return *this;
    }
    Interpreter& add_macro(std::string macro, std::string replacement) {
        script_macros[macro] = replacement;
        return *this;
    }
    InterpreterError bake(std::string file) {
        if(!bake_extension(file,settings)) 
            settings.error_msg = "error while baking: " + file;
        return *this;
    }
    InterpreterError bake(Extension* ext) {
        if(!bake_extension(ext,settings)) 
            settings.error_msg = "error while baking: <compiled>";
        return *this;
    }

    bool has_builtin(std::string name) {
        return script_builtins.find(name) != script_builtins.end();
    }
    ScriptBuiltin& get_builtin(std::string name) {
        return script_builtins[name];
    }
    bool has_macro(std::string name) {
        return script_macros.find(name) != script_macros.end();
    }
    std::string& get_macro(std::string name) {
        return script_macros[name];
    }
    bool has_operator(std::string name) {
        return script_operators.find(name) != script_operators.end();
    }
    std::vector<ScriptOperator>& get_operator(std::string name) {
        return script_operators[name];
    }
};

inline InterpreterError& InterpreterError::on_error(std::function<void(Interpreter&)> fun) {
    if(!interpreter) fun(interpreter);
    return *this;
}

inline InterpreterError& InterpreterError::otherwise(std::function<void(Interpreter&)> fun) {
    if(interpreter) fun(interpreter);
    return *this;
}

inline InterpreterError& InterpreterError::throw_error() {
    if(!interpreter) throw interpreter;
    return *this;
}

inline void InterpreterState::load(Interpreter& interp) const {
    interp.clear();
    interp.script_builtins = this->script_builtins;
    interp.script_operators = this->script_operators;
    interp.script_typechecks = this->script_typechecks;
    interp.script_macros = this->script_macros;
}
inline void InterpreterState::save(const Interpreter& interp) {
    script_builtins = interp.script_builtins;
    script_operators = interp.script_operators;
    script_typechecks = interp.script_typechecks;
    script_macros = interp.script_macros;
}

// converts a literal into a variable
inline ScriptVariable to_var(KittenToken src, ScriptSettings& settings) {
    ScriptVariable ret;
    for(auto i : settings.interpreter.script_typechecks) {
        ScriptValue* v = i(src,settings);
        if(v != nullptr) {
            return std::move(ScriptVariable(v));
        }
    }
    return script_null;
}

// checks if the token is a valid literal
inline bool valid_literal(KittenToken src,ScriptSettings& settings) {
    for(auto i : settings.interpreter.script_typechecks) {
        ScriptValue* v = i(src,settings);
        if(v != nullptr) {
            delete v;
            return true;
        }
    }
    return false;
}

// sets the error flag is the variable is null
inline ScriptVariable not_null_check(ScriptVariable var, ScriptSettings& settings) {
    if(is_null(var)) {
        settings.error_msg = "not allowed to be null!";
    }
    return var;
}

#define CARESCRIPT_EXTENSION using namespace carescript;
#define CARESCRIPT_EXTENSION_GETEXT_INLINE(...) extern "C" { inline carescript::Extension* get_extension() { __VA_ARGS__ } }
#define CARESCRIPT_EXTENSION_GETEXT(...) extern "C" { carescript::Extension* get_extension() { __VA_ARGS__ } }

using BuiltinList = std::unordered_map<std::string,ScriptBuiltin>;
using OperatorList = std::unordered_map<std::string,std::vector<ScriptOperator>>;
using MacroList = std::unordered_map<std::string,std::string>;
using TypeList = std::vector<ScriptTypeCheck>;

// abstract class to provide an interface for extensions
class Extension {
public:
    virtual BuiltinList get_builtins() = 0;
    virtual OperatorList get_operators() = 0;
    virtual MacroList get_macros() = 0;
    virtual TypeList get_types() = 0;
};

using get_extension_fun = Extension*(*)();

// external overloads for the ScriptVariable constructor

template<typename _Tp>
concept IntegralType = std::is_integral<_Tp>::value;
template<IntegralType _Tp>
inline void from(carescript::ScriptVariable& var, _Tp integral) {
    var = new carescript::ScriptNumberValue(integral);
}
inline void from(carescript::ScriptVariable& var, std::string string) {
    var = new carescript::ScriptStringValue(string);
}

} /* namespace carescript */

#endif