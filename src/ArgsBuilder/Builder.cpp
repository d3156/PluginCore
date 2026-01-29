#include "Builder.hpp"
#include "Logger/Log.hpp"
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>

#include <chrono>
#include <ctime>
#include <sstream>
#include <utility>
#include <sys/utsname.h>
#include <unistd.h>

namespace d3156
{
    using Args::Builder;
    using namespace std;
#undef LOG_NAME
#define LOG_NAME "Args"

    inline void error(const string &err)
    {
        R_LOG(0, err);
        exit(-1);
    }

    const char *Builder::AbstractOption::type_to_string(TYPE t)
    {
        switch (t) {
            case OPTION: return "OPTION";
            case PARAM: return "PARAM";
            case FLAG: return "FLAG";
        };
        return "";
    }

    void Builder::help() const
    {
        int long_size = 7;
        for (const auto i : all_) long_size = max(static_cast<int>(i->long_name.length()), long_size);
        long_size += 4;
        cout << company_text_ << endl
             << left << setw(12) << "Short" << setw(long_size) << "Long" << setw(12) << "Param type"
             << "Description" << endl
             << left << setw(12) << "? or -h" << setw(long_size) << "--help" << setw(12) << "OPTION"
             << "Print this help message and exit" << endl
             << left << setw(12) << "-v" << setw(long_size) << "Version" << setw(12) << "OPTION"
             << "Print the app version and exit" << endl;

        for (const auto p : all_) {
            string shn = " ", lgn = " ", type = AbstractOption::type_to_string(p->type);
            if (p->short_name != '\0') shn = "-" + string(1, p->short_name);
            if (!p->long_name.empty()) lgn = "--" + p->long_name;
            cout << setw(12) << shn << setw(long_size) << lgn << setw(12) << type << p->description << endl;
        }
        exit(0);
    }

    void Builder::version() const
    {
        cout << company_text_ << endl << "Version : " << version_ << endl;
        exit(0);
    }

    void Builder::process(AbstractOption *param, const char *str)
    {
        if (!param->parse(str)) error("Cannot parse " + string(str));
    }

    Builder &Builder::parse(int argc, char *argv[])
    {
        app_path_ = string(argv[0]);
        for (int i = 1; i < argc; i++) {
            string line(argv[i]);

            if (line == "-h" || line == "--help" || line == "?" || line == "-?") help();

            if (line == "--version" || line == "-v") version();

            AbstractOption *param = nullptr;

            if (line.substr(0, 2) == "--") {
                auto str = line.substr(2);

                if (!params_long_.contains(str)) error("Unknown parameter " + line);
                param = params_long_[str];
            } else if (line.substr(0, 1) == "-") {
                auto str = line.substr(1);

                if (str.length() > 1) error("Invalid syntax in " + line + " short name must be 1 letter length");

                if (!params_short_.contains(str[0])) error("Unknown parameter " + line);
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

    Builder::~Builder()
    {
        for (const auto i : all_) delete i;
    }

    Builder &Builder::addFlag(bool &flag, const char short_name, const std::string &long_name,
                              const std::string &description)
    {
        return addParam(new Flag(flag, AbstractOption::FLAG, short_name, long_name, description));
    }

    Builder &Builder::addFlag(bool &flag, const std::string &long_name, const std::string &description)
    {
        return addParam(new Flag(flag, AbstractOption::FLAG, '\0', long_name, description));
    }

    Builder &Builder::setVersion(const std::string &version)
    {
        version_ += "\n" + version;
        return *this;
    }

    Builder &Builder::setCompanyText(const std::string &company)
    {
        company_text_ = company;
        return *this;
    }

    Builder &Builder::addParam(AbstractOption *param)
    {
        all_.push_back(param);
        if (param->short_name != '\0') params_short_[param->short_name] = param;
        if (!param->long_name.empty()) params_long_[param->long_name] = param;
        return *this;
    }

    Builder::Flag::Flag(bool &value, const TYPE t, const char s, const std::string &l, const std::string &d)
        : AbstractOption(t, s, l, d), value_(value)
    {
        value_ = false;
    }

    bool Builder::Flag::parse(const char *) { return parsed_ = value_ = true; }

    Builder::AbstractOption::AbstractOption(const TYPE t, const char s, std::string l, std::string d)
        : type(t), short_name(s), long_name(std::move(l)), description(std::move(d))
    {
    }

    bool Builder::AbstractOption::isParsed() const { return parsed_; }

    class SystemInfo
    {
    public:
        static std::string getCurrentTime()
        {
            const auto now       = std::chrono::system_clock::now();
            const auto in_time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
            return ss.str();
        }

        static std::string getOSInfo()
        {
            utsname unameData{};
            if (uname(&unameData) == 0)
                return std::string(unameData.sysname) + " " + unameData.nodename + " " + unameData.domainname + " " +
                       unameData.release + " " + unameData.version + " (" + unameData.machine + ") ";
            return "Unix-like OS";
        }

        static std::string getHostname()
        {
            char buffer[256];
            if (gethostname(buffer, sizeof(buffer)) == 0) return buffer;
            return "Unknown";
        }
    };

    void Args::printHeader(const int argc, char *argv[])
    {
        constexpr int width = 80;
        std::cout << std::string(width, '=') << std::endl;
        std::cout << "\033[1;32md3156::PluginCore - Plugin Loader System\033[0m" << std::endl;
        std::cout << std::string(width, '-') << std::endl;
        std::cout << "\033[34mVersion   \033[0m  : " << PLUGIN_CORE_VERSION << std::endl;
        std::cout << "\033[34mStart Time\033[0m  : " << SystemInfo::getCurrentTime() << std::endl;
        std::cout << "\033[34mHostname  \033[0m  : " << SystemInfo::getHostname() << std::endl;
        std::cout << "\033[34mOS        \033[0m  : " << SystemInfo::getOSInfo() << std::endl;
        if (argc > 0) {
            std::cout << "\033[34mExecutable\033[0m  : " << argv[0] << std::endl;
            if (argc > 1) {
                std::cout << "\033[34mArguments\033[0m   : ";
                for (int i = 1; i < argc; ++i) {
                    std::cout << argv[i];
                    if (i < argc - 1) std::cout << " ";
                }
                std::cout << std::endl;
            }
        }
        std::cout << std::string(width, '=') << std::endl;
    }
}
