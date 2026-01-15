#include "Builder.hpp"
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>

using Args::Builder;
using namespace std;

void error(string err) {
    cout << "[Args] " << err << endl;
    exit(-1);
}

const char *Builder::AbstractOption::type_to_string(TYPE t) {
    switch (t) {
    case OPTION: return "OPTION";
    case PARAM: return "PARAM";
    case FLAG: return "FLAG";
    };
    return "";
}

void Builder::help() {
    size_t long_size = 7;
    for (auto i : all_) long_size = max((size_t)i->long_name.length(), long_size);
    long_size += 4;
    cout << compamy_text_ << endl
         << left << setw(12) << "Short" << setw(long_size) << "Long" << setw(12) << "Param type"
         << "Description" << endl
         << left << setw(12) << "? or -h" << setw(long_size) << "--help" << setw(12) << "OPTION"
         << "Print this help message and exit" << endl
         << left << setw(12) << "-v" << setw(long_size) << "Version" << setw(12) << "OPTION"
         << "Print the app version and exit" << endl;

    for (auto p : all_) {
        string shn = " ", lgn = " ", type = AbstractOption::type_to_string(p->type);
        if (p->short_name != '\0') shn = "-" + string(1, p->short_name);
        if (p->long_name.size()) lgn = "--" + p->long_name;
        cout << setw(12) << shn << setw(long_size) << lgn << setw(12) << type << p->description << endl;
    }
    exit(0);
}

void Builder::version() {
    cout << compamy_text_ << endl << "Version : " << version_ << endl;
    exit(0);
}

void Builder::process(AbstractOption *param, const char *str) {
    if (!param->parse(str)) error("Cannot parse " + string(str));
}

Builder &Builder::parse(int argc, char *argv[]) {
    app_path_ = string(argv[0]);
    for (int i = 1; i < argc; i++) {
        string line(argv[i]);

        if (line == "-h" || line == "--help" || line == "?" || line == "-?") help();

        if (line == "--version" || line == "-v") version();

        AbstractOption *param = nullptr;

        if (line.substr(0, 2) == "--") {
            auto str = line.substr(2);

            if (params_long_.find(str) == params_long_.end()) error("Unknown parametr " + line);
            param = params_long_[str];
        } else if (line.substr(0, 1) == "-") {
            auto str = line.substr(1);

            if (str.length() > 1) error("Invalid syntax in " + line + " short name must be 1 leter length");

            if (params_short_.find(str[0]) == params_short_.end()) error("Unknown parametr " + line);
            param = params_short_[str[0]];
        } else
            error("Invalid syntax in " + line + " -- or - expected");
        if (param->type == AbstractOption::FLAG)
            process(param, "");
        else
            process(param, argv[++i]);
    }
    for (auto i : all_)
        if (i->type == AbstractOption::PARAM && !i->isParsed())
            error("Parameter must be specified -" + string(1, i->short_name) + " or --" + i->long_name);
    return *this;
}
Builder::~Builder() {
    for (auto i : all_) delete i;
}

Args::Builder &Args::Builder::addFlag(bool &flag, char short_name, std::string long_name,
                                                          std::string description) {
    return addParam(new Flag(flag, AbstractOption::FLAG, short_name, long_name, description));
}

Args::Builder &Args::Builder::addFlag(bool &flag, std::string long_name, std::string description) {
    return addParam(new Flag(flag, AbstractOption::FLAG, '\0', long_name, description));
}

Args::Builder &Args::Builder::setVersion(std::string version) {
    version_ = version;
    return *this;
}

Args::Builder &Args::Builder::setCompanyText(std::string compamy) {
    compamy_text_ = compamy;
    return *this;
}

Args::Builder &Args::Builder::addParam(AbstractOption *param) {
    all_.push_back(param);
    if (param->short_name != '\0') params_short_[param->short_name] = param;
    if (param->long_name != "") params_long_[param->long_name] = param;
    return *this;
}