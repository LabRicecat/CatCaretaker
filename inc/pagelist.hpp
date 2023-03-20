#ifndef PAGELIST_HPP
#define PAGELIST_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

#include "../mods/kittenlexer.hpp"

struct Url {
    int args = 0;
    std::vector<std::string> placeholders;
    std::vector<std::string> url;
    
    size_t size() const { return url.size(); }
    bool matches(size_t _size) const { return args == _size; }
};

struct Rule {
    std::string name;
    std::vector<char> symbols;
    Url link;
    std::map<std::string,int> positions;
    std::map<int,std::string> rvpositions;
    std::map<std::string,std::string> defaults;
    std::vector<std::string> scripts;
    // 0 -> before
    // 1 -> after
    // 2 -> instead
    int script_handle = 1;
    std::vector<std::pair<std::string,int>> embedded;

    void merge(Rule rule) {
        symbols = rule.symbols;
        link = rule.link;
        positions = rule.positions;
        defaults = rule.defaults;
        scripts = rule.scripts;
        script_handle = rule.script_handle;
        embedded = rule.embedded;
    }
};
using RuleList = std::unordered_map<std::string,Rule>;

struct UrlPackage {
    Rule rule;
    std::string link;
};

static inline bool is_message_split_sign(char c) {
    switch(c) {
        case '!':
        case '/':
        case '@':
        case '?':
        case '&':
        case '%':
        case '$':
        case '#':
        case '*':
        case '~':
        case '<':
        case '>':
        case ':':
            return true;
    }
    return false;
}
RuleList process_rulelist(std::string source);

Url parse_link(std::string source);

std::vector<UrlPackage> find_url(RuleList list, std::string string);

bool is_url(std::string check);

#define ERR(message) return message
#define NEEDARG(argc) if(args.size() != argc) ERR("command needs " + std::to_string(argc) + " argument(s)")
#define NOSTRING(arg) if(args[arg].str) ERR("command's parameter " + std::to_string(arg+1) + " is not allowed to be a string")
#define ISSTRING(arg) if(!args[arg].str) ERR("command's parameter " + std::to_string(arg+1) + " must be a string")
#define HASRULE() if(rule == nullptr) ERR("command needs to belong to a rule")
#define HASLINK() if(rule->link.url.size() == 0) ERR("command requires a link to be set")
static inline std::unordered_map<std::string,std::function<std::string(std::vector<KittenToken>,Rule*&,RuleList&)>> commands = {
    {"RULE",[](std::vector<KittenToken> args, Rule*& rule, RuleList& rules) -> std::string {
        NEEDARG(1);
        NOSTRING(0);
        rule = &rules[args[0].src];
        rule->name = args[0].src;
        return "";
    }},
    {"FROM",[](std::vector<KittenToken> args, Rule*& rule, RuleList& rules) -> std::string {
        HASRULE();
        if(args.size() == 0) ERR("command needs at least one argument");
        
        for(size_t i = 0; i < args.size(); ++i) {
            NOSTRING(i);
            if(rules.find(args[i].src) == rules.end()) ERR("no such rule: " + args[i].src);
            
            rule->merge(rules[args[i].src]);
        }
        return "";
    }},
    {"LINK",[](std::vector<KittenToken> args, Rule*& rule, RuleList& rules) -> std::string {
        HASRULE();
        NEEDARG(1);
        ISSTRING(0);
        rule->link = parse_link(args[0].src);
        return "";
    }},
    {"DEFAULT",[](std::vector<KittenToken> args, Rule*& rule, RuleList& rules) -> std::string {
        HASRULE();
        HASLINK();
        if(args.size() == 0) ERR("command needs at least one argument");
        if(args.size() % 2 == 1) ERR("command needs even ammount of parameters");

        for(size_t i = 0; i < args.size(); i += 2) {
            NOSTRING(i);
            ISSTRING(i+1);
            if(rule->positions.find(args[i].src) == rule->positions.end()) ERR("no such placeholder to default: " + args[i].src);
            for(auto pos : rule->positions) 
                if(pos.second > rule->positions[args[i].src] && rule->defaults.find(pos.first) == rule->defaults.end()) 
                    ERR("not allowed to default a placeholder before a undefaulted one");
            rule->defaults[args[i].src] = args[i+1].src;
        }
        
        return "";
    }},
    {"SIGNS",[](std::vector<KittenToken> args, Rule*& rule, RuleList& rules) -> std::string {
        HASRULE();
        HASLINK();
        if(args.size() == 0) ERR("command needs at least one argument");
        for(size_t i = 0; i < args.size(); ++i) {
            NOSTRING(0);
            if(args[i].src.size() != 1 || !is_message_split_sign(args[i].src[0])) ERR("unexpected character");
            rule->symbols.push_back(args[i].src[0]);
        }
        while(rule->symbols.size() < (rule->link.placeholders.size() - 1)) {
            rule->symbols.push_back(rule->symbols.back());
        }
        if(rule->symbols.size() > (rule->link.placeholders.size() - 1)) ERR("too many symbols for this link");
        return "";
    }},
    {"WITH",[](std::vector<KittenToken> args, Rule*& rule, RuleList& rules) -> std::string {
        HASRULE();
        HASLINK();
        if(args.size() == 0) ERR("command needs at least one argument");
        if(args.size() % 2 == 1) ERR("command needs even ammount of parameters");

        for(size_t i = 0; i < args.size(); i += 2) {
            NOSTRING(i);
            NOSTRING(i+1);
            if(std::find(rule->link.placeholders.begin(), rule->link.placeholders.end(),args[i].src) == rule->link.placeholders.end()) ERR("no such placeholder: " + args[i].src);
            try {
                int pos = std::stoi(args[i+1].src);
                if(pos <= 0) ERR("invalid placeholder position: " + args[i+1].src);
                --pos;
                if(rule->rvpositions.find(pos) != rule->rvpositions.end()) ERR("already taken position: " + args[i+1].src);
                rule->positions[args[i].src] = pos;
                rule->rvpositions[pos] = args[i].src;
            }
            catch(...) {
                ERR("command needs number value as position (got: " + args[i+1].src + ")");
            }
        }
        return "";
    }},
    {"ATTACH",[](std::vector<KittenToken> args, Rule*& rule, RuleList& rules) -> std::string {
        HASRULE();
        if(args.size() == 0) ERR("command needs at least one argument");
        
        for(size_t i = 0; i < args.size(); ++i) {
            ISSTRING(i);
            rule->scripts.push_back(args[i].src);
        }
        return "";
    }},
    {"SCRIPTS",[](std::vector<KittenToken> args, Rule*& rule, RuleList& rules) -> std::string {
        HASRULE();
        NEEDARG(1);
        NOSTRING(0);

        if(args[0].src == "BEFORE") {
            rule->script_handle = 0;
        }
        else if(args[0].src == "AFTER") {
            rule->script_handle = 1;
        }
        else if(args[0].src == "INSTEAD") {
            rule->script_handle = 2;
        }
        else ERR("unknown option: " + args[0].src);
        
        return "";
    }},
    {"EMBED",[](std::vector<KittenToken> args, Rule*& rule, RuleList& rules) -> std::string {
        HASRULE();
        NEEDARG(2);
        NOSTRING(0);
        NOSTRING(1);
        std::pair<std::string,int> emb;

        if(args[0].src == "BEFORE") {
            emb.second = 0;
        }
        else if(args[0].src == "AFTER") {
            emb.second = 1;
        }
        else if(args[0].src == "INSTEAD") {
            emb.second = 2;
        }
        else ERR("unknown option: " + args[0].src);

        if(args[1].src.front() != '{') ERR("expected {capsule}");
        args[1].src.erase(args[1].src.begin());
        args[1].src.pop_back();
        emb.first = args[1].src;
        rule->embedded.push_back(emb);
        return "";
    }}
};
#undef NEEDARG
#undef NOSTRING
#undef ISSTRING
#undef ERR
#undef HASRULE
#undef HASLINK

#endif