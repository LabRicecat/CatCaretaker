#ifndef INI_HANDLER_HPP
#define INI_HANDLER_HPP

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>

#include "kittenlexer.hpp"

#define IS_INI_TYPE //marks which is a type and which is not

enum class IniType {
    Null,
    String,
    List,
    Int,
    Float,
    Vector,
    Dictionary,
    Link
};

inline std::string IniType2str(IniType type) {
    std::string types[] = {
        "Null", "String", "List", "Int", "Float","Vector","Dictionary"
    };
    return types[static_cast<int>(type)];
}

// Not a type but container for any type
class IniElement;
class IniFile;
using IniDictionary     IS_INI_TYPE  =  std::map<std::string,IniElement>;
using IniList           IS_INI_TYPE  =  std::vector<IniElement>;
struct IniVector        IS_INI_TYPE 
{ // Vector3, not std::vector
    int x = 0,y = 0,z = 0;

    inline void operator=(IniVector v) {
        x = v.x;
        y = v.y;
        z = v.z;
    }

    inline std::string to_string() const { return "(" + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z) + ")"; }
};
struct IniLink          IS_INI_TYPE;

namespace IniHelper {
    inline bool all_numbers(std::string str, bool& dot) {
        dot = false;
        for(auto i : str) {
            if(!isalnum(i)) {
                if(i == '.' && !dot) dot = true;
                else return false;
            }
        }
        return true;
    }

    // Sets OBJ to and empty version of TYPE
    inline void set_type(IniElement& obj, IniType type);

    // @exception will return IniDictionary()
    inline IniDictionary to_dictionary(std::string source);
    // @exception will return IniList()
    inline IniList to_list(std::string source);
    // @exception will return IniVector()
    inline IniVector to_vector(std::string source);
    // @exception will return IniElement() (aka Null)
    inline IniElement to_element(std::string source);

    inline IniLink make_link(std::string to, std::string section, IniFile& file);

    // Section is "Main"
    inline IniLink make_link(std::string to, IniFile& file);

    inline std::string to_string(IniList list);
    inline std::string to_string(IniDictionary dictionary);
    inline std::string to_string(IniVector vector);

    inline bool brace_check(std::string str, char open, char close) {
        KittenLexer lexer = KittenLexer()
            .add_stringq('"')
            .add_stringq('\'')
            .add_capsule('(',')')
            .add_capsule('[',']')
            .add_capsule('{','}')
            .add_ignore(' ')
            .add_ignore('\t')
            .add_linebreak('\n')
            .add_lineskip('#')
            .erase_empty();
        auto lexed = lexer.lex(str);

        return lexer && lexed.size() == 1 && !lexed.front().str
            && lexed.front().src.front() == open 
            && lexed.front().src.back() == close;
    }
}

class IniElement {
    IniType type;
    std::string src;
public:
    inline IniType get_type() const { return type; }

    IniElement(IniType type, std::string src) // unsave, don't use if you don't know what you are doing!
        : type(type), src(src) { } 
    
    // Warning! This doesn't construct a string for you! It will check what construct type it is. so "12" is NOT a string but an int!
    // Use operator=(std::string) instead!
    IniElement(std::string src) // always use this to constructfrom a type!
         { this->src = IniHelper::to_element(src).src; }

    IniElement(IniType type) 
        { IniHelper::set_type(*this,type); }
    
    IniElement(int integer)
        : type(IniType::Int), src(std::to_string(integer)) { }
    
    IniElement(float floatp)
        : type(IniType::Float), src(std::to_string(floatp)) { }
    
    IniElement(IniList list)
        { operator=(list); }
    
    IniElement(IniDictionary dictionary)
        { operator=(dictionary); }

    IniElement(IniVector vector)
        { operator=(vector); }

    //IniElement(IniElement& element) 
    //    : type(element.type), src(element.src) {}
    
    IniElement() {}

    // returns the construction string
    inline std::string to_string() const { return src; }
    // returns IniVector() on error
    inline IniVector to_vector() const { return IniHelper::to_vector(src); }
    // returns IniList() on error
    inline IniList to_list() const { return IniHelper::to_list(src); }
    // returns IniDictionary() on error
    inline IniDictionary to_dictionary() const { return IniHelper::to_dictionary(src); }
    // returns a broken link on error
    inline IniLink to_link(IniFile& file) const; /*{ return IniHelper::make_link(src,file); }*/

    static IniElement from_vector(IniVector vec) 
    { return IniElement(IniType::Vector,vec.to_string()); }
    static IniElement from_list(IniList list)
    { return IniElement(IniType::List,IniHelper::to_string(list)); }
    static IniElement from_dictionary(IniDictionary dictionary) 
    { return IniElement(IniType::Dictionary,IniHelper::to_string(dictionary)); }

    // checks only the type
    inline bool is_list() const { return type == IniType::List; }
    // checks only the type
    inline bool is_dictionary() const { return type == IniType::Dictionary; }
    // checks only the type
    inline bool is_vector() const { return type == IniType::Vector; }
    // doesnt just return type == IniType::Link
    // also checks with IniLink::is_valid() !
    inline bool is_link() const; /*{ return type == IniType::Link && IniLink::valid(src); }*/

    inline IniElement operator=(IniList list) {
        src = IniHelper::to_string(list);
        type = IniType::List;
        return *this;
    }
    inline IniElement operator=(IniVector vector) {
        src = IniHelper::to_string(vector);
        type = IniType::Vector;
        return *this;
    }
    inline IniElement operator=(IniDictionary dictionary) {
        src = IniHelper::to_string(dictionary);
        type = IniType::Dictionary;
        return *this;
    }
    inline IniElement operator=(IniElement element) {
        src = element.to_string();
        type = element.type;
        return *this;
    }
    
    // This, other then IniElement(std::string), constructs a string!
    inline IniElement operator=(std::string str) {
        src = "\"" + str + "\"";
        type = IniType::String;
        return *this;
    }
    inline IniElement operator=(int integer) {
        src = std::to_string(integer);
        type = IniType::Int;
        return *this;
    }
    inline IniElement operator=(long double floatp) {
        src = std::to_string(floatp);
        type = IniType::Float;
        return *this;
    }

    inline operator IniList() { return IniHelper::to_list(src); }
    inline operator IniVector() { return IniHelper::to_vector(src); }
    inline operator IniDictionary() { return IniHelper::to_dictionary(src); }
    inline operator std::string() { 
        if(type == IniType::String) return src.substr(1,src.size()-2);
        return src;
    }
};

inline std::ostream &operator<<(std::ostream& os, IniElement element) {
    std::string pr = element.to_string();
    if(element.get_type() == IniType::String) {
        pr.erase(pr.begin());
        pr.pop_back();
    }
    
    os << pr;
    return os;
}

enum class IniError {
    OK,
    READ_ERROR,
    WRITE_ERROR,
    SYNTAX_ERROR
};

struct IniPair {
    std::string key;
    IniElement element = IniElement(IniType::Null);

    IniPair() {}
    IniPair(std::string key) : key(key) {}
    IniPair(std::string key, IniElement element)
        : key(key), element(element) {}
    //IniPair(IniPair& pair)
    //    : key(pair.key), element(pair.element) {}
};

struct IniSection {
    std::string name;
    std::vector<IniPair> members;

    IniSection(std::string name)
        : name(name) {}

    IniElement& operator[](std::string key) {
        for(auto& i : members) 
            if(i.key == key) return i.element;
        
        members.push_back(IniPair(key)); // creates new key
        return members.back().element;
    }

    inline bool has(std::string key) const {
        for(auto& i : members) 
            if(i.key == key) return true;  
        return false;
    }

    inline std::string to_string() const {
        std::string str = "[" + name + "]\n";
        for(auto& i : members) 
            str += i.key + " = " + i.element.to_string() + "\n";
        return str;
    }
};

class IniFile {
    IniError err = IniError::OK;
    std::string err_desc = "";
public:
    std::vector<IniSection> sections;

    inline bool has(std::string key) const {
        return has(key,"");
    }
    inline bool has(std::string key, std::string section) const {
        if(section == "") section = "Main";
        for(auto i : sections) 
            if(i.has(key) && i.name == section) return true;
        return false;
    }
    inline bool has_section(std::string section) const {
        for(auto i : sections) 
            if(i.name == section) return true;
        return false;
    }

    // Writes all sections into the file.
    inline void to_file(std::string file) {
        std::string write = "";
        for(size_t i = 0; i < sections.size(); ++i)
            write += sections[i].to_string() + "\n";

        std::fstream f;
        f.open(file, std::ofstream::out | std::ofstream::trunc); f.close();
        f.open(file, std::ios::binary | std::ios::app | std::ios::in);
        f << write; f.close();
    }

    // Use IniFile::from_file() when you know that the file already exists.
    // Use IniFile(std::string) when you want the fire beeing constructed if it doesn't exist
    static IniFile from_file(std::string file) {
        IniFile ret;
        ret.sections.push_back(IniSection("Main"));
        int current_section = 0;

        std::string rd;
        std::ifstream ifs;
        ifs.open(file);
        while(ifs.good()) rd += ifs.get();
        if(!rd.empty()) rd.pop_back();

        KittenLexer lexer = KittenLexer()
            .add_stringq('"')
            .add_stringq('\'')
            .add_capsule('(',')')
            .add_capsule('[',']')
            .add_capsule('{','}')
            .add_backslashopt('t','\t')
            .add_backslashopt('n','\n')
            .add_backslashopt('r','\r')
            .add_backslashopt('\\','\\')
            .add_backslashopt('"','"')
            .add_ignore(' ')
            .add_ignore('\t')
            .add_linebreak('\n')
            .add_extract('=')
            .add_extract(',')
            .erase_empty()
            .add_lineskip('#');
        auto lexed = lexer.lex(rd);

        if(lexed.empty() || !lexer) {
            ret.err = IniError::READ_ERROR; 
            if(!lexer) ret.err_desc = "Lexing Error\n";
            else ret.err_desc = "Empty file\n";
            return ret;
        }

        std::vector<lexed_kittens> lines;
        int cline = -1;
        for(auto i : lexed) {
            if(cline != i.line) {
                lines.push_back({});
                cline = i.line;
            }
            lines.back().push_back(i);
        }

        for(size_t i = 0; i < lines.size(); ++i) {
            if(lines[i].empty()) continue;
            if(lines[i][0].src.front() == '[' && !lines[i][0].str) {
                if(lines[i].size() != 1) {
                    ret.err = IniError::SYNTAX_ERROR;
                    ret.err_desc = "Invalid section declaration!\n";
                    return ret;
                }
                std::string section = lines[i][0].src;
                section.erase(section.begin());
                section.pop_back();
                bool found = false;
                    for(size_t j = 0; j < ret.sections.size(); ++j) {
                    if(ret.sections[j].name == section) {
                        current_section = j;
                        found = true;
                        break;
                    }
                }

                if(!found) {
                    ret.sections.push_back(IniSection(section));
                    current_section = ret.sections.size()-1;
                }
            }
            else {
                std::string left;
                std::string right;

                if(lines[i].size() != 3 || lines[i][1].src != "=" || lines[i][1].str) {
                    ret.err = IniError::SYNTAX_ERROR; 
                    ret.err_desc = "Invalid format in line " + std::to_string(lines[i][0].line+1) + "\n";
                    return ret;
                }

                left = lines[i][0].src;
                if(lines[i][2].str) right = "\"" + lines[i][2].src + "\"";
                else right = lines[i][2].src;

                if(ret.sections[current_section].has(left)) 
                    ret.sections[current_section][left] = IniHelper::to_element(right);
                else 
                    ret.sections[current_section].members.push_back(IniPair(left,IniHelper::to_element(right)));
            }
        }

        if(ret.sections.size() == 0) {
            ret.err = IniError::READ_ERROR; 
            ret.err_desc = "No sections found!\n";
        }
        return ret;
    }

    inline void operator=(IniFile file) { sections = file.sections; }
    inline operator bool() { return err == IniError::OK; }
    inline IniError error() const { return err; }
    inline std::string error_msg() const { return err_desc; }

    // Returns true if the IniFile failed before the clear.
    inline bool clearerr() { bool r = err == IniError::OK; err = IniError::OK; err_desc = ""; return r; }

    // @exception will return sections.front().members.front().element and sets err/err_desc
    inline IniElement& get(std::string key, std::string section_ = "Main") {
        if(section_ == "") section_ = "Main";
        
        if(!has_section(section_)) {
            sections.push_back(IniSection(section_));
        }

        auto& s = section(section_);
        return s[key];
    }

    // @exception will return sections.front() and sets err/err_desc
    inline IniSection& section(std::string name) {
        if(!has_section(name)) {
            err = IniError::READ_ERROR;
            err_desc = "Unable to get not existing section " + name + " !";
            return sections.front();
        }
        for(auto& i : sections) 
            if(i.name == name) return i;

        err = IniError::READ_ERROR;
        err_desc = "Unable to get not existing section " + name + " !";
        return sections.front();
    }

    inline void set(std::string key, IniElement value, std::string section = "Main") { get(key,section) = value; }
    inline void set(std::string key, IniList value, std::string section = "Main") { get(key,section) = value; }
    inline void set(std::string key, IniDictionary value, std::string section = "Main") { get(key,section) = value; }
    inline void set(std::string key, IniVector value, std::string section = "Main") { get(key,section) = value; }

    inline void set(std::string key, std::string value, std::string section = "Main") { get(key,section) = value; }
    inline void set(std::string key, int value, std::string section = "Main") { get(key,section) = value; }
    inline void set(std::string key, long double value, std::string section = "Main") { get(key,section) = value; }

    inline void construct(std::string key, std::string source, std::string section = "Main") {
        IniFile::set(key,IniElement(source),section);
    }

    IniFile(std::string file) { operator=(from_file(file)); }

    IniFile() {}
    // checks if `elem` is a IniLink, and if yes, returns it's value.
    inline IniElement operator[](IniElement elem);
    // Also sets `last` from `link`
    inline IniElement operator[](IniLink link);
};

 // a read only pointer to another element
class IniLink {
    IniFile* last;
    std::string build;
    IniElement source;

    inline void construct(IniFile& file) {
        std::string b = build;
        b.erase(b.begin());
        int p = -1;
        for(size_t i = 0; i < b.size(); ++i) {
            if(b[i] == ':') {
                p = i;
                break;
            }
        }
        if(p == b.size()-1 && p == 0) return;

        std::string key;
        std::string section;
        if(p == -1) {
            key = b;
            section = "Main";
        }
        else {
            key = b.substr(0,p);
            section = b.substr(p+1,b.size()-1);
        }
        if(file.has(key,section)) {
            source = file.get(key,section);
        }
    }
public:
    // refreshes
    inline void refresh(IniFile& file) {
        if(valid(build)) {
            last = &file;
            construct(file);
        }
    }
    inline IniElement get() { return source; }
    // calls also `reload` with the last used IniFile
    inline IniElement getr() { return getr(*last); }
    // calls also `reload()`
    inline IniElement getr(IniFile& file) {
        refresh(file);
        return get();
    }
    IniLink(std::string src) : build(src) { }

    static bool valid(std::string str) { return (str.size() > 2 && str[0] == '$'); }

    inline IniElement operator*() { return source; }
    inline operator IniElement() { return IniElement(IniType::Link,build); }
};

namespace IniHelper {
    inline void set_type(IniElement& obj, IniType type) {
        switch(type) {
            case IniType::Int:
                obj = IniElement(type,"0");
            break;
            case IniType::Float:
                obj = IniElement(type,"0.0");
            break;
            case IniType::String:
                obj = IniElement(type,"\"\"");
            break;
            case IniType::Null:
                obj = IniElement(type,"NULL");
            break;
            case IniType::List:
                obj = IniElement(type,"[]");
            break;
            case IniType::Dictionary:
                obj = IniElement(type,"{}");
            break;
            case IniType::Vector:
                obj = IniElement(type,"(0,0,0)");
            break;
            case IniType::Link:
                obj = IniLink("$null:Main");
            break;
            default:
                obj = IniElement(type,"NULL");
        }
    }
        // @exception will return IniDictionary()
    inline IniDictionary to_dictionary(std::string source) {
        if(source.front() != '{' || source.back() != '}') return IniDictionary();
        source.erase(source.begin());
        source.pop_back();
        if(source == "") return IniDictionary();
        
        IniDictionary dic;
        KittenLexer lexer = KittenLexer()
            .add_stringq('"')
            .add_stringq('\'')
            .add_capsule('(',')')
            .add_capsule('[',']')
            .add_capsule('{','}')
            .add_backslashopt('t','\t')
            .add_backslashopt('n','\n')
            .add_backslashopt('r','\r')
            .add_backslashopt('\\','\\')
            .add_extract(',')
            .add_ignore(' ')
            .add_ignore('\t')
            .add_extract(':')
            .add_linebreak('\n')
            .add_lineskip('#')
            .erase_empty();

        auto vec = lexer.lex(source);

        std::string key;
        IniElement value;
        bool got_dd = false, set_v = false;
        for(auto i : vec) {
            if(i.src == ":" && !i.str) {
                if(got_dd || set_v) return IniDictionary();
                got_dd = true;
            }
            else if(i.src == "," && !i.str) {
                if(!set_v) return IniDictionary();
                dic[key] = value;
                key = ""; value = ""; got_dd = false; set_v = false;
            }
            else {
                if((key != "" && !got_dd) || set_v) return IniDictionary();
                if(i.str) i.src = "\"" + i.src + "\"";

                if(got_dd) { value = IniHelper::to_element(i.src); set_v = true; }
                else key = i.src;
            }
        }
        if(set_v) dic[key] = value;
        return dic;
    }
    
    // @exception will return IniList()
    inline IniList to_list(std::string source) {
        IniList list;
        if(!brace_check(source,'[',']'))return IniList();
        source.pop_back();
        source.erase(source.begin());

        KittenLexer lexer = KittenLexer()
            .add_stringq('"')
            .add_stringq('\'')
            .add_capsule('(',')')
            .add_capsule('[',']')
            .add_capsule('{','}')
            .add_backslashopt('t','\t')
            .add_backslashopt('n','\n')
            .add_backslashopt('r','\r')
            .add_backslashopt('\\','\\')
            .add_ignore(',')
            .add_linebreak('\n')
            .add_lineskip('#')
            .erase_empty();
        auto vec = lexer.lex(source);

        for(auto i : vec) {
            if(i.str) i.src = "\"" + i.src + "\"";
            list.push_back(to_element(i.src));
        }
        return list;
    }
    
    // @exception will return IniVector()
    inline IniVector to_vector(std::string source) {
        if(source.front() != '(' && source.back() != ')') return IniVector();
        IniVector vec;
        source.erase(source.begin());
        source.pop_back();

        KittenLexer lexer = KittenLexer()
            .add_stringq('"')
            .add_stringq('\'')
            .add_capsule('(',')')
            .add_capsule('[',']')
            .add_capsule('{','}')
            .add_backslashopt('t','\t')
            .add_backslashopt('n','\n')
            .add_backslashopt('r','\r')
            .add_backslashopt('\\','\\')
            .add_ignore(',')
            .add_linebreak('\n')
            .add_lineskip('#')
            .erase_empty();
        auto sp = lexer.lex(source);

        if(sp.size() != 3) {
            return IniVector();
        }

        for(int i = 0; i < 3; ++i) if(sp[i].str) sp[i].src = "\"" + sp[i].src + "\"";
        try {
            vec.x = std::stoi(sp[0].src);
            vec.y = std::stoi(sp[1].src);
            vec.z = std::stoi(sp[2].src);
        }
        catch(...) { return IniVector(); }

        return vec;
    }
    
    // @exception will return IniElement() (aka Null)
    inline IniElement to_element(std::string source) {
        if(source == "" || source == "NULL") {
            return IniElement(IniType::Null,"NULL");
        }
        if(source.front() == '"' && source.back() == '"') {
            return IniElement(IniType::String,source);
        }
        
        bool dot = false;
        bool intt = all_numbers(source,dot);
        if(intt) {
            return IniElement(dot ? IniType::Float : IniType::Int,source);
        }

        if(brace_check(source,'{','}')) {
            return IniElement(IniType::Dictionary,source);
        }
        if(brace_check(source,'[',']')) {
            return IniElement(IniType::List,source);
        }
        if(brace_check(source,'(',')')) {
            return IniElement(IniType::Vector,source);
        }

        if(IniLink::valid(source)) {
            return IniElement(IniType::Link,source);
        }

        bool has = false;
        for(auto i : source) {
            if(i == '\"') {
                has = true;
                break;
            }
        }
        if(!has) { // converts it to string non the less
            return IniElement(IniType::String,"\""+source+"\"");
        }

        return IniElement();
    }

    inline IniLink make_link(std::string to, std::string section, IniFile& file) {
        IniLink lk("$" + to + ":" + section);
        lk.getr(file);
        return lk;
    }

    // Section is "Main"
    inline IniLink make_link(std::string to, IniFile& file) {
        IniLink lk(to);
        lk.getr(file);
        return lk;
    }

    inline std::string to_string(IniList list) {
        std::string ret = "[";
        for(auto i : list) 
            ret += i.to_string() + ",";
        if(list.size() != 0) ret.pop_back();
        ret += ']';
        return ret;
    }
    inline std::string to_string(IniDictionary dictionary) {
        std::string ret = "{";
        for(auto i : dictionary) {
            ret += i.first + ":" + i.second.to_string() + ",";
        }
        if(ret.size() != 1) ret.pop_back();
        ret += '}';
        return ret;
    }
    inline std::string to_string(IniVector vector) {
        return vector.to_string();
    }
}

// Moved down here because of incomplete definition problems

inline IniLink IniElement::to_link(IniFile& file) const { return IniHelper::make_link(src,file); }
inline bool IniElement::is_link() const { return type == IniType::Link && IniLink::valid(src); }

inline IniElement IniFile::operator[](IniElement elem) {
    if(elem.is_link()) {
        IniLink nlk = elem.to_link(*this);
        return nlk.getr(*this);
    }
    return elem;
}
inline IniElement IniFile::operator[](IniLink link) { return link.getr(*this); }

#endif