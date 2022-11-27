#include "ArgParser.h"

bool Arg::hasAlias(std::string name) {
    for(auto& i : aliase) {
        if(i == name) {
            return true;
        }
    }
    return false;
}

bool ParsedArgs::operator[](const char* arg) {
    for(auto& i : args_tag) {
        if(i.name == arg || i.hasAlias(arg)) {
            return i.is;
        }
    }
    return false;
}

bool ParsedArgs::operator[](std::string arg) {
    for(auto& i : args_tag) {
        if(i.name == arg || i.hasAlias(arg)) {
            return i.is;
        }
    }
    return false;
}

std::string ParsedArgs::operator()(std::string arg) {
    LOG(args_set.size())
    for(auto& i : args_set) {
        LOG(i.name << " == " << arg)
        if(i.name == arg || i.hasAlias(arg)) {
            LOG("retuned i.val (" << i.val << ")")
            return i.val;
        }
    }
    LOG("args_get:" << args_get.size())
    for(auto& i : args_get) {
        LOG(i.name << " == " << arg)
        if(i.name == arg || i.hasAlias(arg)) {
            LOG("retuned i.val (" << i.val << ")")
            return i.val;
        }
    }
    return "";
}

ParsedArgs::operator bool() {
    return error_code == ArgParserErrors::OK;
}

bool ParsedArgs::operator==(ArgParserErrors error) {
    return error_code == error;
}

bool ParsedArgs::operator!=(ArgParserErrors error) {
    return error_code != error;
}

std::string ParsedArgs::error() const {
    return error_msg;
}

bool ParsedArgs::has(std::string arg_name) {
    for(auto i : this->args_get) {
        if(i.name == arg_name && i.val != "") {
            return true;
        }
    }

    for(auto i : this->args_set) {
        if(i.name == arg_name && i.val != "") {
            return true;
        }
    }

    for(auto i : this->args_tag) {
        if(i.name == arg_name && i.is) {
            return true;
        }
    }
    
    return false;
}

std::vector<std::string> ParsedArgs::get_bin() const {
    return bin;
}

bool ParsedArgs::has_bin() const {
    return bin_filled;
}

size_t ArgParser::find(std::string name, bool& failed) {
    for(size_t i = 0; i < args.size(); ++i) {
        LOG(":" << args[i].name << " == " << name)
        if(args[i].name == name || args[i].hasAlias(name)) {
            LOG("found!")
            failed = false;
            return i;
        }
    }
    failed = true;
    return 0;
}

size_t ArgParser::find_next_getarg(bool& failed) {
    LOG("unusedGetArgs:" << unusedGetArgs)
    if(unusedGetArgs == 0) {
        failed = true;
        return 0;
    }
    for(size_t i = 0; i < args.size(); ++i) {
        if(args[i].type == ARG_GET && args[i].val == "") {
            --unusedGetArgs;
            failed = false;
            return i;
        }
    }
    failed = true;
    return 0;
}

ArgParser& ArgParser::addArg(std::string name, int type, std::vector<std::string> aliase, int fixed_pos, Arg::Priority prio) {
    args.push_back(Arg{prio,type,name,aliase,false,fixed_pos});
    return *this;
}
ArgParser& ArgParser::enableString(char sym) {
    strsym = sym;
    return *this;
}

ArgParser& ArgParser::setbin() {
    has_bin = true;
    return *this;
}

ParsedArgs ArgParser::parse(std::vector<std::string> args) {
    if(args.size() == 0) {
        return ParsedArgs({},ArgParserErrors::NO_ARGS,"");
    }
    unusedGetArgs = 0;
    bool failed = false;
    for(size_t i = 0; i < this->args.size(); ++i) {
        if(this->args[i].type == ARG_GET) {
            ++unusedGetArgs;
        }
    }

    for(size_t i = 0; i < args.size(); ++i) {
        LOG(args[i]);
    }
    failed = false;
    for(size_t i = 0; i < args.size(); ++i) {
        size_t index = find(args[i],failed);
        LOG("find(" << args[i] << ") = " << index)
        if(failed) {
            //LOG("failed = true")
            //return ParsedArgs({},true);
            index = find_next_getarg(failed);
            if(failed) {
                if(has_bin) {
                    LOG("filling bin..")
                    for(size_t j = i; j < args.size(); ++j) {
                        bin.push_back(args[j]);
                    }
                    break;
                }

                LOG("failed = true")
                return ParsedArgs({},ArgParserErrors::UNKNOWN_ARG,args[i]);
            }
            else if(this->args[index].type == ARG_GET) {
                this->args[index].val = args[i];
            }
        }

        if(this->args[index].fixed_pos != -1 && this->args[index].fixed_pos != i) {
            return ParsedArgs({},ArgParserErrors::POSITION_MISSMATCH,args[i] + " not at right position!"); 
        }

        if(this->args[index].type == ARG_SET) {
            if(i+1 >= args.size()) {
                LOG("i+1 >= tokens.size()")
                return ParsedArgs({},ArgParserErrors::INVALID_SET,this->args[index].name);
            }

            this->args[index].val = args[i+1];
            LOG("this->args[" << index << "] = " << args[i+1])
            ++i;
        }
        else if (this->args[index].type == ARG_TAG) {
            this->args[index].is = true;
        }
    }

    auto tmpa = this->args;
    unusedGetArgs = 0;
    for(size_t i = 0; i < this->args.size(); ++i) { //clear
        this->args[i].val = "";
        this->args[i].is = false;
        if(this->args[i].type == ARG_GET) {
            ++unusedGetArgs;
        }
    }

    // check for unmatching dependencies
    for(size_t i = 0; i < tmpa.size(); ++i) { //clear
        if(((tmpa[i].val == "" && tmpa[i].type != ARG_TAG ) || (!tmpa[i].is && tmpa[i].type == ARG_TAG)) && tmpa[i].priority == Arg::Priority::FORCE) {
            return ParsedArgs(tmpa,ArgParserErrors::UNMATCHED_DEP_FORCE,tmpa[i].name);
        }
        else if(((tmpa[i].val != "" && tmpa[i].type != ARG_TAG ) || (tmpa[i].is && tmpa[i].type == ARG_TAG)) && tmpa[i].priority == Arg::Priority::IGNORE) {
            return ParsedArgs(tmpa,ArgParserErrors::UNMATCHED_DEP_IGNORE,tmpa[i].name);
        }
    }

    
    return ParsedArgs(tmpa,ArgParserErrors::OK,"",bin);
}

ParsedArgs ArgParser::parse(char** args, int argc) {
    if(argc == 1) {
        return ParsedArgs({},ArgParserErrors::NO_ARGS,"");
    }
	std::vector<std::string> par;
	
    for(size_t i = 1; i < argc; ++i) {
        par.push_back(args[i]);
    }

	return parse(par);
}
