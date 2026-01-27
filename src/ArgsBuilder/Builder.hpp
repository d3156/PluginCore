#pragma once
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace d3156 {
    template<typename T>
    bool from_string(const char *str, T &val) {
        const std::string data(str);
        if (data.empty()) return false;
        std::istringstream iss(data);
        if (data.find("0x") != std::string::npos)
            iss >> std::hex >> val;
        else
            iss >> std::dec >> val;
        return !iss.fail();
    }

    namespace Args {
        void printHeader(int argc, char *argv[]);

        class Builder {
            class AbstractOption {
            protected:
                bool parsed_ = false;

            public:
                enum TYPE { OPTION, PARAM, FLAG } type;

                char short_name = 0;
                std::string long_name;
                std::string description;

                static const char *type_to_string(TYPE t);

                AbstractOption(TYPE t, char s, std::string l, std::string d);

                bool isParsed() const;

                virtual bool parse(const char *) = 0;

                virtual ~AbstractOption() = default;
            };

            template<class Type>
            class Param : public AbstractOption {
            public:
                Type &value_;

                Param(Type &value, const TYPE t, const char s, const std::string &l, const std::string &d)
                    : AbstractOption(t, s, l, d), value_(value) {
                }

                bool parse(const char *str) override {
                    const bool ok = from_string(str, value_);
                    parsed_ = true;
                    return ok;
                }

                ~Param() override = default;
            };

            class Flag : public AbstractOption {
            public:
                bool &value_;

                Flag(bool &value, TYPE t, char s, const std::string &l, const std::string &d);

                bool parse(const char *) override;

                ~Flag() override = default;
            };

        public:
            ~Builder();

            Builder &parse(int argc, char *argv[]);

            template<class Type>
            Builder &addOption(Type &value, char short_name, std::string long_name = "", std::string description = "") {
                return addParam(new Param<Type>(value, AbstractOption::OPTION, short_name, long_name, description));
            }

            template<class Type>
            Builder &addParam(Type &value, char short_name, std::string long_name = "", std::string description = "") {
                return addParam(new Param<Type>(value, AbstractOption::PARAM, short_name, long_name, description));
            }

            template<class Type>
            Builder &addOption(Type &value, std::string long_name = "", std::string description = "") {
                return addParam(new Param<Type>(value, AbstractOption::OPTION, '\0', long_name, description));
            }

            template<class Type>
            Builder &addParam(Type &value, std::string long_name = "", std::string description = "") {
                return addParam(new Param<Type>(value, AbstractOption::PARAM, '\0', long_name, description));
            }

            Builder &addFlag(bool &flag, char short_name, const std::string &long_name = "",
                             const std::string &description = "");

            Builder &addFlag(bool &flag, const std::string &long_name = "", const std::string &description = "");

            Builder &setVersion(const std::string &version);

            Builder &setCompanyText(const std::string &company);

        private:
            static void process(AbstractOption *param, const char *str);

            void version() const;

            void help() const;

            Builder &addParam(AbstractOption *param);

            std::map<std::string, AbstractOption *> params_long_;
            std::map<char, AbstractOption *> params_short_;
            std::vector<AbstractOption *> all_;
            std::string version_;
            std::string app_path_;
            std::string company_text_;
        };
    } // namespace Args
}
