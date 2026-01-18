#pragma once
#include <map>
#include <sstream>
#include <string>
#include <vector>
namespace d3156
{
    template <typename T> bool from_string(const char *str, T &val)
    {
        std::string data(str);
        if (data.empty()) return false;
        std::istringstream iss(data);
        if (data.find("0x") != std::string::npos)
            iss >> std::hex >> val;
        else
            iss >> std::dec >> val;
        return !iss.fail();
    }

    namespace Args
    {
        class Builder
        {
            class AbstractOption
            {
            protected:
                bool parsed_ = false;

            public:
                enum TYPE { OPTION, PARAM, FLAG } type;
                char short_name         = 0;
                std::string long_name   = "";
                std::string description = "";
                static const char *type_to_string(TYPE t);
                AbstractOption(TYPE t, char s, std::string l, std::string d)
                    : type(t), short_name(s), long_name(l), description(d)
                {
                }
                bool isParsed() { return parsed_; }
                virtual bool parse(const char *) = 0;
                virtual ~AbstractOption() {};
            };
            template <class Type> class Param : public AbstractOption
            {
            public:
                Type &value_;
                Param(Type &value, TYPE t, char s, std::string l, std::string d)
                    : AbstractOption(t, s, l, d), value_(value)
                {
                }
                virtual bool parse(const char *str) override
                {
                    bool ok = from_string(str, value_);
                    parsed_ = true;
                    return ok;
                }
                virtual ~Param() {}
            };
            class Flag : public AbstractOption
            {
            public:
                bool &value_;
                Flag(bool &value, TYPE t, char s, std::string l, std::string d)
                    : AbstractOption(t, s, l, d), value_(value)
                {
                    value_ = false;
                }
                virtual bool parse(const char *) override { return parsed_ = value_ = true; }
                virtual ~Flag() {}
            };

        public:
            ~Builder();
            Builder &parse(int argc, char *argv[]);
            template <class Type>
            Builder &addOption(Type &value, char short_name, std::string long_name = "", std::string description = "")
            {
                return addParam(new Param<Type>(value, AbstractOption::OPTION, short_name, long_name, description));
            }
            template <class Type>
            Builder &addParam(Type &value, char short_name, std::string long_name = "", std::string description = "")
            {
                return addParam(new Param<Type>(value, AbstractOption::PARAM, short_name, long_name, description));
            }
            template <class Type>
            Builder &addOption(Type &value, std::string long_name = "", std::string description = "")
            {
                return addParam(new Param<Type>(value, AbstractOption::OPTION, '\0', long_name, description));
            }
            template <class Type>
            Builder &addParam(Type &value, std::string long_name = "", std::string description = "")
            {
                return addParam(new Param<Type>(value, AbstractOption::PARAM, '\0', long_name, description));
            }
            Builder &addFlag(bool &flag, char short_name, std::string long_name = "", std::string description = "");
            Builder &addFlag(bool &flag, std::string long_name = "", std::string description = "");
            Builder &setVersion(std::string version);
            Builder &setCompanyText(std::string compamy);

        private:
            void process(AbstractOption *param, const char *str);
            void version();
            void help();
            Builder &addParam(AbstractOption *param);
            std::map<std::string, AbstractOption *> params_long_;
            std::map<char, AbstractOption *> params_short_;
            std::vector<AbstractOption *> all_;
            std::string version_  = "";
            std::string app_path_ = "";
            std::string compamy_text_;
        };
    } // namespace Args
}