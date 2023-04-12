#include "../inc/pagelist.hpp"
#include <regex>

RuleList process_rulelist(std::string source) {
    KittenLexer line_lexer = KittenLexer()
        .add_capsule('{','}')
        .add_ignore(' ')
        .add_linebreak(';')
        .add_con_extract(is_message_split_sign)
        .add_ignore('\t')
        .add_ignore('\n')
        .ignore_backslash_opts()
        .erase_empty()
        .add_stringq('"')
        .add_stringq('\'')
    ;
    auto lexed = line_lexer.lex(source);
    std::vector<lexed_kittens> lines;
    int l = 0;
    for(auto i : lexed)
        if(i.line != l) { l = i.line; lines.push_back({i}); }
        else lines.back().push_back(i);
    
    Rule* current_rule = nullptr;
    RuleList rules;
    for(auto i : lines) {
        while(!i.empty() && i.front().str) i.erase(i.begin());
        if(i.empty()) continue;

        if(commands.find(i.front().src) == commands.end()) { std::cout << "no " << i.front().src << "\n"; return {}; }
        auto command = commands[i.front().src]; i.erase(i.begin());
        std::string err = command(i,current_rule,rules);
        if(err != "") { std::cout << err << "\n"; return {}; }
    }
    //auto cprules = rules;
    //for(const auto& i : rules)
    //    if(i.second.link.url.empty() || i.second.symbols.empty() || i.second.link.placeholders.empty()) 
    //        cprules.erase(i.first);
    return rules; // TODO: add error checks
}

bool is_url(std::string check) {
    return (check.size() > 8 && check.substr(0,8) == "https://")
        || (check.size() > 7 && check.substr(0,7) == "http://")
        || (check.size() > 6 && check.substr(0,6) == "ftp://")
        || (check.size() > 7 && check.substr(0,7) == "rsync://")
        || (check.size() > 6 && check.substr(0,6) =="sftp://")
        || (check.size() > 7 && check.substr(0,7) =="ipfs://")
        || (check.size() > 8 && check.substr(0,8) =="samba://")
        || (check.size() > 9 && check.substr(0,9) == "gemini://")
        || (check.size() > 6 && check.substr(0,6) == "irc://");
}

Url parse_link(std::string source) {
    KittenLexer url_lexer = KittenLexer()
        .add_capsule('{','}')
        .erase_empty()
        .ignore_backslash_opts();

    auto url = url_lexer.lex(source);
    Url ret;
    for(auto u : url) {
        ret.url.push_back(u.src);
        if(u.src.front() == '{') {
            u.src.erase(u.src.begin());
            u.src.pop_back();
            ret.placeholders.push_back(u.src);
        }
    }
    return ret;
}

std::vector<UrlPackage> find_url(RuleList list, std::string source) {
    if(is_url(source)) return {{{"$empty$"},source}};
    std::vector<UrlPackage> ret;
    KittenLexer lexer = KittenLexer()
        .add_con_extract(is_message_split_sign)
        .erase_empty()
        .ignore_backslash_opts();
    const auto args = lexer.lex(source);

    for(auto i : list) {
        Rule rule = i.second;
        std::string s;
        std::map<std::string,std::string> mp;

        for(auto j : rule.link.placeholders) mp[j] = "";
        for(auto j : rule.defaults) mp[j.first] = j.second;

        bool failed = false;
        for(size_t j = 0; j < args.size(); ++j) {
            if(j % 2 == 1) {
                if(rule.symbols.size() == 0 || rule.symbols.front() != args[j].src[0]) { failed = true; break; }
                rule.symbols.erase(rule.symbols.begin());
            }
            else mp[rule.rvpositions[j/2]] = args[j].src;
        }
        if(failed) continue;
        for(auto j : mp) 
            if(j.second == "") {
                failed = true;
                break;
            }
        
        if(failed) continue;
        
        for(auto u : rule.link.url) {
            if(u.front() == '{') {
                u.erase(u.begin());
                u.pop_back();
                s += mp[u];
            }
            else s += u;
        }
        ret.push_back({rule,s,mp});
    }
    return ret;
}