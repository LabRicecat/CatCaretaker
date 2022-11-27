#ifndef INI_HANDLER_HPP
#define INI_HANDLER_HPP

#include <string>
#include <vector>
#include <map>
#include <optional>

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

    void operator=(IniVector v) {
        x = v.x;
        y = v.y;
        z = v.z;
    }

    inline std::string to_string() const { return "(" + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z) + ")"; }
};
struct IniLink          IS_INI_TYPE;

namespace IniHelper {
    // Sets OBJ to and empty version of TYPE
    void set_type(IniElement& obj, IniType type);

    // @exception will return IniDictionary()
    IniDictionary to_dictionary(std::string source);
    // @exception will return IniList()
    IniList to_list(std::string source);
    // @exception will return IniVector()
    IniVector to_vector(std::string source);
    // @exception will return IniElement() (aka Null)
    IniElement to_element(std::string src);

    IniLink make_link(std::string to, std::string section, IniFile& file);
    // Section is "Main"
    IniLink make_link(std::string src, IniFile& file);


    namespace tls {
        std::vector<std::string> split_by(std::string str,std::vector<char> erase, std::vector<char> keep = {}, std::vector<char> extract = {}, bool ignore_in_braces = false, bool delete_empty = true);

        template<typename T>
        inline bool isIn(T element, std::vector<T> vec) {
            for(auto i : vec) {
                if(element == i) {
                    return true;
                }
            }
            return false;
        }
    }

    std::string to_string(IniList list);
    std::string to_string(IniDictionary dictionary);
    std::string to_string(IniVector vector);
}

class IniElement {
    IniType type;
    std::string src;
public:
    IniType getType() const;

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
    std::string to_string() const;
    // returns IniVector() on error
    IniVector to_vector() const;
    // returns IniList() on error
    IniList to_list() const;
    // returns IniDictionary() on error
    IniDictionary to_dictionary() const;
    // returns a broken link on error
    IniLink to_link(IniFile& file) const;

    static IniElement from_vector(IniVector vec);
    static IniElement from_list(IniList list);
    static IniElement from_dictionary(IniDictionary dictionary);

    // checks only the type
    bool is_list() const;
    // checks only the type
    bool is_dictionary() const;
    // checks only the type
    bool is_vector() const;
    // doesnt just return type == IniType::Link
    // also checks with IniLink::is_valid() !
    bool is_link() const;

    IniElement operator=(IniList list);
    IniElement operator=(IniVector vector);
    IniElement operator=(IniDictionary dictionary);
    IniElement operator=(IniElement element);
    // This, other then IniElement(std::string), constructs a string!
    IniElement operator=(std::string str);
    IniElement operator=(int integer);
    IniElement operator=(float floatp);

    operator IniList();
    operator IniVector();
    operator IniDictionary();
    operator std::string();
};

std::ostream &operator<<(std::ostream& os, IniElement element);   

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

    IniElement& operator[](std::string key);

    bool has(std::string key) const;

    std::string to_string() const;
};

class IniFile {
    IniError err = IniError::OK;
    std::string err_desc = "";
public:
    std::vector<IniSection> sections;

    bool has(std::string key) const;
    bool has(std::string key, std::string section) const;
    bool has_section(std::string section) const;

    // Writes all sections into the file.
    // Might fail and set error() & error_msg()
    IniError to_file(std::string file);

    // Use IniFile::from_file() when you know that the file already exists.
    // Use IniFile(std::string) when you want the fire beeing constructed if it doesn't exist
    static IniFile from_file(std::string file);

    void operator=(IniFile file);
    operator bool();
    IniError error() const;
    std::string error_msg() const;

    // Returns true if the IniFile failed before the clear.
    bool clearerr();

    // @exception will return sections.front().members.front().element and sets err/err_desc
    IniElement& get(std::string key, std::string section = "Main");
    // @exception will return sections.front() and sets err/err_desc
    IniSection& section(std::string name);

    void set(std::string key, IniElement value, std::string section = "Main");
    void set(std::string key, IniList value, std::string section = "Main");
    void set(std::string key, IniDictionary value, std::string section = "Main");
    void set(std::string key, IniVector value, std::string section = "Main");

    void set(std::string key, std::string value, std::string section = "Main");
    void set(std::string key, int value, std::string section = "Main");
    void set(std::string key, float value, std::string section = "Main");

    void construct(std::string key, std::string source, std::string section = "Main");

    IniFile(std::string file) {
        operator=(from_file(file));
    }

    IniFile() {}
    // checks if `elem` is a IniLink, and if yes, returns it's value.
    IniElement operator[](IniElement elem);
    // Also sets `last` from `link`
    IniElement operator[](IniLink link);
};

class IniLink
{ // a read only pointer to another element
    IniFile* last;
    std::string build;
    IniElement source;
    void construct(IniFile& file);
public:
    // refreshes
    void refresh(IniFile& file);
    IniElement get();
    // calls also `reload` with the last used IniFile
    IniElement getr();
    // calls also `reload()`
    IniElement getr(IniFile& file);
    IniLink(std::string src) : build(src) { }

    static bool valid(std::string str);

    IniElement operator*();
    operator IniElement();
};

#endif