#include "Inipp.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>

// help functions
static std::streamsize get_flength(std::ifstream& file) {
	if(!file.is_open()) {
		return 0;
	}
	std::streampos temp_1 = file.tellg();
	file.seekg(0, std::fstream::end);
	std::streampos temp_2 = file.tellg();
	file.seekg(temp_1, std::fstream::beg);

	return temp_2;
}

static std::string read(std::string path) {
	std::ifstream ifile;
	ifile.open(path, std::ios::binary);
	std::streamsize len = get_flength(ifile);
	char* dummy = new char[len];

	try {
		ifile.read(dummy, len);
	}
	catch(std::exception& err) {
		ifile.close();
        return "";
	}
	if (dummy == nullptr || strlen(dummy) == 0) {
		ifile.close();
        return "";
	}
	ifile.close();
	//dummy += '\0';
	std::string re;
	re.assign(dummy, len);

	delete[] dummy;
	dummy = nullptr;

	return re;
}

static bool all_numbers(std::string str, bool& dot) {
    dot = false;
    for(auto i : str) {
        if(!IniHelper::tls::isIn<char>(i,{'0','1','2','3','4','5','6','7','8','9'})) {
            if(i == '.') {
                if(dot) {
                    return false;
                }
                dot = true;
                continue;
            }
            return false;
        }
    }
    return true;
}

static bool br_check(std::string str, char open, char close) {
    int br_count = 0;
    if(str.size() < 2) {
        return false;
    }
    for(size_t i = 0; i < str.size(); ++i) {
        if(str[i] == open) {
            ++br_count;
        }
        if(str[i] == close) {
            --br_count;
        }

        if(br_count == 0 && i+1 != str.size()) {
            return false;
        }
    }
    if(br_count == 0) {
        return true;
    }
    return false;
}

static std::vector<std::string> to_lines(std::string src) {
    std::vector<std::string> ret;
    std::string tmp;
    for(auto i : src) {
        if(i == '\n') {
            if(tmp != "") {
                ret.push_back(tmp);
            }
            tmp = "";
        }
        else {
            tmp += i;
        }
    }
    if(tmp != "") {
        ret.push_back(tmp);
    }

    return ret;
}

static bool is_empty(std::string str) {
    if(str == "") {
        return true;
    }
    for(auto i : str) {
        if(!IniHelper::tls::isIn(i,{' ','\t','\r','\n'})) {
            return false;
        }
    }
    return true;
}

// IniFile

bool IniFile::has(std::string key) const {
    for(auto i : sections) {
        if(i.has(key)) {
            return true;
        }
    }
    return false;
}

bool IniFile::has(std::string key, std::string section) const {
    for(auto i : sections) {
        if(i.has(key) && i.name == section) {
            return true;
        }
    }
    return false;
}

bool IniFile::has_section(std::string section) const {
    for(auto i : sections) {
        if(i.name == section) {
            return true;
        }
    }
    return false;
}

IniError IniFile::to_file(std::string file) {
    std::string write = "";
    for(size_t i = 0; i < sections.size(); ++i) {
        write += sections[i].to_string() + "\n";
    }

    std::fstream f;
    std::ofstream ofs;
    ofs.open(file, std::ofstream::out | std::ofstream::trunc);
    ofs.close();
    f.open(file, std::ios::binary | std::ios::app);
    f << write;
    f.close();
    return IniError::OK;
}

IniFile IniFile::from_file(std::string file) {
    IniFile ret;
    ret.sections.push_back(IniSection("Main"));
    int current_section = 0;
    std::string rd = read(file);
    if(is_empty(rd)) {
        ret.err = IniError::READ_ERROR; 
        ret.err_desc = "Empty file\n";
        return ret;
    }

    auto lines = to_lines(rd);

    for(size_t i = 0; i < lines.size(); ++i) {
        if(lines[i].front() == '[') {
            if(lines[i].back() == ']') {
                std::string section = lines[i];
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
               ret.err = IniError::SYNTAX_ERROR; 
               ret.err_desc = "Unfinished section in line " + std::to_string(i+1) + "\n";
               return ret;
            } 
        }
        else if(lines[i].front() == '#') {
            ; // comment
        }
        else {
            std::string left;
            std::string right;
            auto split = IniHelper::tls::split_by(lines[i],{},{},{'='},true,true);

            if(split.size() != 3 || split[1] != "=") {
                ret.err = IniError::SYNTAX_ERROR; 
                ret.err_desc = "Invalid format in line " + std::to_string(i+1) + "\n";
                return ret;
            }

            left = split[0]; // split[1] is "="
            right = split[2];

            bool found = false;
            for(auto& j : ret.sections[current_section].members) {
                if(j.key == left) {
                    j.element = IniHelper::to_element(right);
                    found = true;
                    break;
                }
            }
            if(!found) {
                ret.sections[current_section].members.push_back(IniPair(left,IniHelper::to_element(right)));
            }
        }
    }

    if(ret.sections.size() == 0) {
        ret.err = IniError::READ_ERROR; 
        ret.err_desc = "No sections found!\n";
    }
    return ret;
}

void IniFile::operator=(IniFile file) {
    sections = file.sections;
}
    
IniFile::operator bool() {
    return err == IniError::OK;
}

IniError IniFile::error() const {
    return err;
}
std::string IniFile::error_msg() const {
    return err_desc;
}

bool IniFile::clearerr() {
    bool r = err == IniError::OK;
    err = IniError::OK;
    err_desc = "";
    return r;
}
#include <optional>

IniElement& IniFile::get(std::string key, std::string sec) {
    if(sec == "") {
        sec = "Main";
    }
    if(!has_section(sec)) {
        err = IniError::READ_ERROR;
        err_desc = "Unable to get not existing variable: " + key + " in not existing section " + sec + " !";
        return sections.front().members.front().element;;
    }

    auto& s = section(sec);
    for(auto& i : s.members) {
        if(i.key == key) {
            return i.element;
        }
    }

    err = IniError::READ_ERROR;
    err_desc = "Unable to get not existing variable: " + key + " in " + sec + " !";
    return sections.front().members.front().element;;
}

IniSection& IniFile::section(std::string name) {
    if(!has_section(name)) {
        err = IniError::READ_ERROR;
        err_desc = "Unable to get not existing section " + name + " !";
        return sections.front();
    }
    for(auto& i : sections) {
        if(i.name == name) {
            return i;
        }
    }

    err = IniError::READ_ERROR;
    err_desc = "Unable to get not existing section " + name + " !";
    return sections.front();
}

void IniFile::set(std::string key, IniElement value, std::string sec) {
    if(!has_section(sec)) {
        sections.push_back(IniSection(sec));
        sections.back()[key] = value;
        return;
    }
    else if(!has(key,sec)) {
        auto& s = section(sec);
        s[key] = value;
        return;
    }

    auto& g = get(key,sec);
    g = value;
    return;
}

void IniFile::set(std::string key, IniList value, std::string section) {
    IniFile::set(key,IniElement(IniType::List,IniHelper::to_string(value)),section);
}

void IniFile::set(std::string key, IniDictionary value, std::string section) {
    IniFile::set(key,IniElement(IniType::Dictionary,IniHelper::to_string(value)),section);
}

void IniFile::set(std::string key, IniVector value, std::string section) {
    IniFile::set(key,IniElement(IniType::Vector,IniHelper::to_string(value)),section);
}

void IniFile::set(std::string key, std::string value, std::string section) {
    IniFile::set(key,IniElement(IniType::String,"\"" + value + "\""),section);
}

void IniFile::set(std::string key, int value, std::string section) {
    IniFile::set(key,IniElement(IniType::Int,std::to_string(value)),section);
}

void IniFile::set(std::string key, float value, std::string section) {
    IniFile::set(key,IniElement(IniType::Float,std::to_string(value)),section);
}

void IniFile::construct(std::string key, std::string source, std::string section) {
    IniFile::set(key,IniElement(source),section);
}

IniElement IniFile::operator[](IniElement elem) {
    if(elem.is_link()) {
        IniLink nlk = elem.to_link(*this);
        return nlk.getr(*this);
    }
    return elem;
}

IniElement IniFile::operator[](IniLink link) {
    return link.getr(*this);
}

// IniSection


IniElement& IniSection::operator[](std::string key) {
    for(auto& i : members) {
        if(i.key == key) {
            return i.element;
        }
    }

    members.push_back(IniPair(key)); // creates new key
    return members.back().element;
}

bool IniSection::has(std::string key) const {
    for(auto& i : members) {
        if(i.key == key) {
            return true;
        }
    }
    return false;
}

std::string IniSection::to_string() const {
    std::string str = "[" + name + "]\n";
    for(auto& i : members) {
        str += i.key + "=" + i.element.to_string() + "\n";
    }
    return str;
}

// IniElement

IniType IniElement::getType() const {
    return type;
}

std::string IniElement::to_string() const {
    return src;
}

IniVector IniElement::to_vector() const {
    return IniHelper::to_vector(src);
}

IniList IniElement::to_list() const {
    return IniHelper::to_list(src);
}

IniDictionary IniElement::to_dictionary() const {
    return IniHelper::to_dictionary(src);
}

IniLink IniElement::to_link(IniFile& file) const {
    return IniHelper::make_link(src,file);
}

IniElement IniElement::from_vector(IniVector vec) {
    return IniElement(IniType::Vector,vec.to_string());
}

IniElement IniElement::from_list(IniList list) {
    return IniElement(IniType::List,IniHelper::to_string(list));
}

IniElement IniElement::from_dictionary(IniDictionary dictionary) {
    return IniElement(IniType::Dictionary,IniHelper::to_string(dictionary));
}

bool IniElement::is_list() const {
    return type == IniType::List;
}

bool IniElement::is_dictionary() const {
    return type == IniType::Dictionary;
}

bool IniElement::is_vector() const {
    return type == IniType::Vector;
}

bool IniElement::is_link() const {
    return IniLink::valid(src) && type == IniType::Link;
}

IniElement IniElement::operator=(IniList list) {
    src = IniHelper::to_string(list);
    type = IniType::List;
    return *this;
}

IniElement IniElement::operator=(IniVector vector) {
    src = IniHelper::to_string(vector);
    type = IniType::Vector;
    return *this;
}

IniElement IniElement::operator=(IniDictionary dictionary) {
    src = IniHelper::to_string(dictionary);
    type = IniType::Dictionary;
    return *this;
}

IniElement IniElement::operator=(IniElement element) {
    src = element.to_string();
    type = element.type;
    return *this;
}

IniElement IniElement::operator=(std::string str) {
    src = "\"" + str + "\"";
    type = IniType::String;
    return *this;
}

IniElement IniElement::operator=(int integer) {
    src = std::to_string(integer);
    type = IniType::Int;
    return *this;
}

IniElement IniElement::operator=(float floatp) {
    src = std::to_string(floatp);
    type = IniType::Float;
    return *this;
}


IniElement::operator IniList() {
    return IniHelper::to_list(src);
}

IniElement::operator IniVector() {
    return IniHelper::to_vector(src);
}

IniElement::operator IniDictionary() {
    return IniHelper::to_dictionary(src);
}

IniElement::operator std::string() {
    if(type == IniType::String)
        return src.substr(1,src.size()-2);
    return src;
}

std::ostream& operator<<(std::ostream& os, IniElement element) {
    std::string pr = element.to_string();
    if(element.getType() == IniType::String) {
        pr.erase(pr.begin());
        pr.pop_back();
    }
    
    os << pr;
    return os;
}

// IniLink
void IniLink::construct(IniFile& file) {
    std::string b = build;
    b.erase(b.begin());

    int p = -1;

    for(size_t i = 0; i < b.size(); ++i) {
        if(b[i] == ':') {
            p = i;
            break;
        }
    }

    if(p == b.size()-1 && p == 0) {
        return;
    }
    
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
    else {
        return;
    }
}

bool IniLink::valid(std::string str) {
    return (str.size() > 2 && str[0] == '$');
}

void IniLink::refresh(IniFile& file) {
    if(valid(build)) {
        last = &file;
        construct(file);
    }
}

IniElement IniLink::get() {
    return source;
}

IniElement IniLink::getr() {
    return getr(*last);
}

IniElement IniLink::getr(IniFile& file) {
    refresh(file);
    // last = &file; <- already in refresh()
    return get();
}

IniLink::operator IniElement() {
    return IniElement(IniType::Link,build);
}

IniElement IniLink::operator*() {
    return source;
}

// IniHelper namespace

std::vector<std::string> IniHelper::tls::split_by(std::string str,std::vector<char> erase, std::vector<char> keep, std::vector<char> extract, bool ignore_in_braces, bool delete_empty) {
    std::vector<std::string> ret;
    std::string tmp;
    int ignore = 0;
    for(size_t i = 0; i < str.size(); ++i) {

        if(isIn(str[i],{'[','{','('}) && ignore_in_braces) {
            ++ignore;
        }
        if(isIn(str[i],{']','}',')'}) && ignore_in_braces) {
            --ignore;
        }
        if(isIn(str[i],erase) && ignore == 0) {
            ret.push_back(tmp);
            tmp = "";
            continue;
        }
        if(isIn(str[i],keep) && ignore == 0) {
            ret.push_back(tmp + str[i]);
            tmp = "";
            continue;
        }
        if(isIn(str[i],extract) && ignore == 0) {
            ret.push_back(tmp);
            ret.push_back(std::string(1,str[i]));
            tmp = "";
            continue;
        }
        tmp += str[i];
    }
    if(tmp != "") {
        ret.push_back(tmp);
        tmp = "";
    }

    if(delete_empty) {
        for(size_t i = 0; i < ret.size(); ++i) {
            if(is_empty(ret[i])) {
                ret.erase(ret.begin()+i);
                --i;
            }
        }
    }
    
    return ret;
}

void IniHelper::set_type(IniElement& obj, IniType type) {
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
        default:
            obj = IniElement(type,"NULL");
    }
}

IniDictionary IniHelper::to_dictionary(std::string source) {
    if(source.front() != '{' || source.back() != '}') {
        return IniDictionary();
    }
    source.erase(source.begin());
    source.pop_back();
    if(source == "") {
        return IniDictionary();
    }
    IniDictionary dic;
    auto vec = tls::split_by(source,{','},{},{':'},true);

    std::string kp;
    bool got_db = false;
    for(auto i : vec) {
        if(i != ":") {
            if(kp != "" && !got_db) {
                return IniDictionary();
            }
            else if(got_db) {
                dic[kp] = IniHelper::to_element(i);
                kp = "";
                got_db = false;
            }
            else {
                kp = i;
            }
        }
        else {
            if(got_db) {
                return IniDictionary();
            }
            got_db = true;
        }
    }

    return dic;
}

IniList IniHelper::to_list(std::string source) {
    IniList list;
    if(source == "") { return list; }
    if(source.front() != '[' && source.back() != ']') {
        return IniList();
    }
    if(!br_check(source,'[',']')) {
        return IniList();
    }

    source.pop_back();
    source.erase(source.begin());

    auto vec = tls::split_by(source,{','},{},{},true);
    for(auto i : vec) {
        list.push_back(to_element(i));
    }
    return list;
}

IniVector IniHelper::to_vector(std::string source) {
    if(source.front() != '(' && source.back() != ')') {
        return IniVector();
    }
    IniVector vec;
    source.erase(source.begin());
    source.pop_back();
    auto sp = tls::split_by(source,{','});

    if(sp.size() != 3) {
        return IniVector();
    }

    try {
        vec.x = std::stoi(sp[0]);
        vec.y = std::stoi(sp[1]);
        vec.z = std::stoi(sp[2]);
    }
    catch(...) {
        return IniVector();
    }



    return vec;
}

IniElement IniHelper::to_element(std::string source) {
    if(source == "" || source == "NULL") {
        return IniElement(IniType::Null,"NULL");
    }
    if(source.front() == '"' && source.back() == '"') {
        return IniElement(IniType::String,source);
    }
    
    bool dot;
    bool intt = all_numbers(source,dot);
    if(intt && !dot) {
        return IniElement(IniType::Int,source);
    }
    if(intt && dot) {
        return IniElement(IniType::Float,source);
    }

    if(br_check(source,'{','}')) {
        return IniElement(IniType::Dictionary,source);
    }
    if(br_check(source,'[',']')) {
        return IniElement(IniType::List,source);
    }
    if(br_check(source,'(',')')) {
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

    if(!has) { // converts it to string non' the less
        return IniElement(IniType::String,"\""+source+"\"");
    }

    return IniElement();
}

IniLink IniHelper::make_link(std::string to, std::string section, IniFile& file) {
    IniLink lk("$" + to + ":" + section);
    //lk.refresh(file);
    lk.getr(file);
    return lk;
}

IniLink IniHelper::make_link(std::string src, IniFile& file) {
    IniLink lk(src);
    //lk.refresh(file);
    lk.getr(file);
    return lk;
}

std::string IniHelper::to_string(IniList list) {
    std::string ret = "[";
    for(auto i : list) {
        ret += i.to_string() + ",";
    }
    if(list.size() != 0) {
        ret.pop_back();
    }
    ret += ']';
    return ret;
}

std::string IniHelper::to_string(IniDictionary dictionary) {
    std::string ret = "{";
    for(auto i : dictionary) {
        ret += i.first + ":" + i.second.to_string() + ",";
    }
    if(dictionary.size() != 0)
        ret.pop_back();
    ret += '}';
    return ret;
}

std::string IniHelper::to_string(IniVector vector) {
    return vector.to_string();
}