#ifndef KITTEN_LEXER_HPP
#define KITTEN_LEXER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <stack>
#include <map>

struct KittenToken {
    std::string src;
    bool str = false;
    unsigned long line = 0;
};

using lexed_kittens = std::vector<KittenToken>;

class KittenLexer {
    using storage = std::map<char,bool>;
    storage stringqs;
    std::vector<std::tuple<char,char>> capsules;
    storage ignores;
    storage as_newline;
    storage extracts;
    storage lineskips;
    std::map<char,char> backslash_opts;
    bool erase_emptys = false;
    bool ignore_backslash_ops = false;
    bool failbit = false;

    using conditional_func = bool(*)(char c);
    using conditional_storage = std::vector<conditional_func>;
    conditional_storage con_ignores;
    conditional_storage con_newlines;
    conditional_storage con_extracts;
    conditional_storage con_lineskips;
    conditional_storage con_stringqs;

    inline bool match_con_any(conditional_storage f,char c) {
        for(auto i : f) if(i(c)) return true;
        return false;
    }

public:
    inline bool is_stringq(char c) const { 
        return stringqs.count(c) > 0;
    }
    inline bool is_capsule(char open, char close) const {
        for(auto tu : capsules) {
            auto [o,c] = tu;
            if(o == open || o == close || c == open || c == close) return true;
        }
        return false;
    }
    inline bool is_capsule_open(char ch) const {
        for(auto tu : capsules) {
            auto [o,c] = tu;
            if(o == ch) return true;
        }
        return false;
    }
    inline bool is_capsule_close(char ch) const {
        for(auto tu : capsules) {
            auto [o,c] = tu;
            if(c == ch) return true;
        }
        return false;
    }
    inline bool match_closure(char open, char close) const {
        for(auto tu : capsules) {
            auto [o,c] = tu;
            if(o == open && c == close) return true;
        }
        return false;
    }
    inline bool is_ignore(char c) const {
        return ignores.count(c) > 0;
    }
    inline bool is_newline(char c) const {
        return as_newline.count(c) > 0;
    }
    inline bool is_extract(char c) const {
        return extracts.count(c) > 0;
    }
    inline bool is_lineskip(char c) const {
        return lineskips.count(c) > 0;
    }
    

    inline KittenLexer& add_capsule(char open, char close) {
        if(!is_capsule(open,close))
            capsules.push_back(std::make_tuple(open,close));
        return *this;
    }
    inline KittenLexer& add_stringq(char c) {
        if(!is_stringq(c))
            stringqs[c] = true;
        return *this;
    }
    inline KittenLexer& add_ignore(char c) {
        if(!is_ignore(c))
            ignores[c] = true;
        return *this;
    }
    inline KittenLexer& add_extract(char c) {
        if(!is_extract(c))
            extracts[c] = true;
        return *this;
    }
    inline KittenLexer& add_lineskip(char c) {
        if(!is_lineskip(c))
            lineskips[c] = true;
        return *this;
    }
    inline KittenLexer& erase_empty() {
        this->erase_emptys = true;
        return *this;
    }
    inline KittenLexer& ignore_backslash_opts() {
        this->ignore_backslash_ops = true;
        return *this;
    }
    inline KittenLexer& add_linebreak(char c) {
        if(!is_newline(c))
            as_newline[c] = true;
        return *this;
    }
    inline KittenLexer& add_backslashopt(char c, char to) {
        backslash_opts[c] = to;
        return *this;
    }

    inline KittenLexer& add_con_stringq(conditional_func func) {
        con_stringqs.push_back(func);
        return *this;
    }
    inline KittenLexer& add_con_extract(conditional_func func) {
        con_extracts.push_back(func);
        return *this;
    }
    inline KittenLexer& add_con_lineskip(conditional_func func) {
        con_lineskips.push_back(func);
        return *this;
    }
    inline KittenLexer& add_con_ignore(conditional_func func) {
        con_ignores.push_back(func);
        return *this;
    }
    inline KittenLexer& add_con_newline(conditional_func func) {
        con_newlines.push_back(func);
        return *this;
    }

    inline lexed_kittens lex(std::string src) {
        lexed_kittens ret;
        std::stack<char> opens;
        std::stack<char> stringqs;
        bool suntil_newline = false;
        unsigned long line = 1;
        KittenToken token;
        for(size_t i = 0; i < src.size(); ++i) {
            if(is_newline(src[i]) || match_con_any(con_newlines,src[i])) {
                ++line;
            }
            if(stringqs.empty() && opens.empty() && (is_ignore(src[i]) || is_newline(src[i]) || match_con_any(con_newlines,src[i]) || match_con_any(con_ignores,src[i]))) {
                if(is_newline(src[i]) || match_con_any(con_newlines,src[i])) suntil_newline = false;
                if((is_ignore(src[i]) || match_con_any(con_ignores,src[i])) && suntil_newline) continue; 
                if(token.src != "" || !erase_emptys) {
                    ret.push_back(token);
                    token = {};
                }
            }
            else if(stringqs.empty() && is_capsule_open(src[i]) && !suntil_newline) {
                if(opens.empty() && (token.src != "" || !erase_emptys)) {
                    if(token.src == "") token.line = line;
                    ret.push_back(token);
                    token = {};
                    token.line = line;
                }
                opens.push(src[i]);
                if(token.src == "") token.line = line;
                token.src += src[i];
            }
            else if(stringqs.empty() && is_capsule_close(src[i]) && !suntil_newline) {
                if(opens.empty() || !match_closure(opens.top(),src[i])) {
                    failbit = true;
                    return ret;
                }
                token.src += src[i];
                opens.pop();
                if(opens.empty()) {
                    if(token.src == "") token.line = line;
                    ret.push_back(token);
                    token = {};
                }
                if(token.src == "") token.line = line;
            }
            else if(opens.empty() && (is_stringq(src[i]) || match_con_any(con_stringqs,src[i])) && !suntil_newline) {
                if(stringqs.empty()) {
                    if(token.src != "" || !erase_emptys) {
                        if(token.src == "") token.line = line;
                        ret.push_back(token);
                        token = {};
                    }
                    stringqs.push(src[i]);
                }
                else if(stringqs.top() == src[i]) {
                    stringqs.pop();
                    token.str = true;
                    ret.push_back(token);
                    token = {};
                }
                else {
                    token.src += src[i];
                }
            }
            else if(opens.empty() && stringqs.empty() && (is_extract(src[i]) || match_con_any(con_extracts,src[i])) && !suntil_newline) {
                if(token.src != "" || !erase_emptys)
                    ret.push_back(KittenToken{token.src,false,line});
                ret.push_back(KittenToken{std::string(1,src[i]),false,line});
                token = {};
            }
            else if(stringqs.empty() && (is_lineskip(src[i]) || match_con_any(con_lineskips,src[i]))) {
                suntil_newline = true;
            }
            else if((is_newline(src[i]) || match_con_any(con_newlines,src[i])) && stringqs.empty() && suntil_newline) {
                suntil_newline = false;
            }
            else if(opens.empty() && /*stringqs.empty() &&*/ src[i] == '\\') {
                if(ignore_backslash_ops) {
                    token.src += src[i];
                    continue;
                }
                if(i+1 != src.size()) {
                    char n = src[i+1];
                    if(backslash_opts.count(n) != 0) token.src += backslash_opts[n];
                    else token.src += n;
                    ++i;
                }
                else {
                    failbit = true;
                    return ret;
                }
            }
            else if(!suntil_newline) {
                if(token.src == "") token.line = line;
                token.src += src[i];
            }
        }
        if(!opens.empty() || !stringqs.empty()) {
            failbit = true;
            return ret;
        }
        if(token.src != "") {
            ret.push_back(token);
        }
        return ret;
    }

    inline operator bool() {
        return !failbit;
    }
};

#endif
