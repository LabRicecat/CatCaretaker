#ifndef CARESCRIPT_TYPES_HPP
#define CARESCRIPT_TYPES_HPP

#include <string>
#include <vector>

namespace carescript {

// abstract class to provide an interface for all types
struct ScriptValue {
    using type = void;
    virtual const std::string get_type() const = 0;
    virtual bool operator==(const ScriptValue*) const = 0;
    virtual bool operator==(const ScriptValue&v) const { return operator==(&v); }
    virtual std::string to_printable() const = 0;
    virtual std::string to_string() const = 0;
    virtual ScriptValue* copy() const = 0;
    void get_value() const {}

    virtual ~ScriptValue() {};
};

// default number type implementation
struct ScriptNumberValue : public ScriptValue {
    const std::string get_type() const override { return "Number"; }
    long double number = 0.0;

    bool operator==(const ScriptValue* val) const override {
        return val->get_type() == get_type() && ((ScriptNumberValue*)val)->number == number;
    }

    std::string to_printable() const override {
        std::string str = std::to_string(number);
        str.erase(str.find_last_not_of('0') + 1, std::string::npos);
        str.erase(str.find_last_not_of('.') + 1, std::string::npos);
        return str;
    }
    std::string to_string() const override {
        return to_printable();
    }

    long double get_value() const { return number; }
    ScriptValue* copy() const override { return new ScriptNumberValue(number); }

    ScriptNumberValue() {}
    ScriptNumberValue(long double num): number(num) {}

    operator long double() { return get_value(); }
};

// default string type implementation
struct ScriptStringValue : public ScriptValue {
    const std::string get_type() const override { return "String"; }
    std::string string = "";
    
    bool operator==(const ScriptValue* val) const override {
        return val->get_type() == get_type() && ((ScriptStringValue*)val)->string == string;
    }

    std::string to_printable() const override {
        return string;
    }
    std::string to_string() const override {
        return "\"" + string + "\"";
    }

    std::string get_value() const { return string; }
    ScriptValue* copy() const override { return new ScriptStringValue(string); }

    ScriptStringValue() {}
    ScriptStringValue(std::string str): string(str) {}

    operator std::string() { return get_value(); }
};

// default name type implementation
struct ScriptNameValue : public ScriptValue {
    const std::string get_type() const override { return "Name"; }
    std::string name = "";
    
    bool operator==(const ScriptValue* val) const override {
        return val->get_type() == get_type() && ((ScriptNameValue*)val)->name == name;
    }

    std::string to_printable() const override {
        return name;
    }
    std::string to_string() const override {
        return to_printable();
    }

    std::string get_value() const { return name; }
    ScriptValue* copy() const override { return new ScriptNameValue(name); }

    ScriptNameValue() {}
    ScriptNameValue(std::string name): name(name) {}
};

// default null type implementation
struct ScriptNullValue : public ScriptValue {
    const std::string get_type() const override { return "Null"; }
    
    bool operator==(const ScriptValue* val) const override {
        return val->get_type() == get_type();
    }

    std::string to_printable() const override {
        return "null";
    }
    std::string to_string() const override {
        return to_printable();
    }

    void get_value() const { return; }
    ScriptValue* copy() const override { return new ScriptNullValue(); }

    ScriptNullValue() {}
};

} /* namespace carescript */

#endif