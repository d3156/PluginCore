#include "Core.hpp"
#include <dlfcn.h>
#include "Structs.hpp"
#include <filesystem>
#include <iostream>
#include <regex>

namespace fs = std::filesystem;
namespace d3156
{

    namespace PluginCore
    {
        struct IPluginLoaderLib {
            using Create    = IPlugin *(*)();
            using Destroy   = void (*)(IPlugin *);
            IPlugin *plugin = nullptr;
            Destroy destroy = nullptr;

            static std::unique_ptr<IPluginLoaderLib> load(std::string path);

            ~IPluginLoaderLib();

        private:
            IPluginLoaderLib(void *h, Destroy destroy_, IPlugin *p) : destroy(destroy_), h_(h), plugin(p) {}
            void *h_ = nullptr;
        };

        Core::Core(int argc, char *argv[])
        {
            Args::Builder bldr;
            loadPlugins();
            for (auto &lib : libs_) lib.second->plugin->registerArgs(bldr);
            bldr.parse(argc, argv);
            for (auto &lib : libs_) lib.second->plugin->registerModels(models_);
            for (auto i : models_) i.second->postInit();
            for (auto &lib : libs_) lib.second->plugin->postInit();
        }

        static std::vector<fs::path> getPaths()
        {
            if (std::getenv("PLUGINS_DIR") == nullptr) {
                std::cout << G_CORE << "Using standart pluging path " << client_plugins_path << "\n";
                if (fs::exists(client_plugins_path) && fs::is_directory(client_plugins_path))
                    return {fs::path(client_plugins_path)};
                return {};
            }
            std::string s = std::getenv("PLUGINS_DIR");
            std::vector<fs::path> out;
            size_t start = 0;
            while (start <= s.size()) {
                size_t end = s.find(':', start);
                if (end == std::string::npos) end = s.size();
                std::string token = s.substr(start, end - start);
                if (!token.empty()) {
                    if (fs::exists(token)) {
                        out.emplace_back(token);
                        std::cout << G_CORE << "Directory of plugins " << token << " added\n";
                    } else
                        std::cout << Y_CORE << "Directory from PLUGINS_DIR not found: " << token << "\n";
                }
                start = end + 1;
            }
            return out;
        }

        void Core::loadPlugins()
        {
            std::vector<fs::path> pluginsDir = getPaths();
            if (pluginsDir.empty()) {
                std::cout << R_CORE << "Empty existing path list for loading plugin!\n";
                exit(-1);
            }
#ifdef DEBUG
            const std::regex re(R"(^lib([^\.]*)\.Debug\.so$)");
#else
            const std::regex re(R"(^lib([^\.]*)\.so$)");
#endif
            for (const auto &pluginDir : pluginsDir) {
                std::cout << G_CORE << "Loading plugins from dir " << pluginDir << "\n";
                for (const auto &de :
                     fs::recursive_directory_iterator(pluginDir, fs::directory_options::skip_permission_denied)) {
                    if (!de.is_regular_file()) continue;

                    const std::string file = de.path().filename().string();

                    std::smatch m;
                    if (!std::regex_match(file, m, re)) continue;

                    const std::string name    = m[1].str();
                    const std::string absName = de.path().string();

                    auto lib = IPluginLoaderLib::load(absName);

                    if (lib == nullptr) continue;

                    std::cout << G_CORE << "Plugin " << name << " loaded\n";
                    if (libs_.contains(name)) {
                        std::cout << Y_CORE << "Plugin with name " << name << " already loaded!\n";
                        std::cout << Y_CORE << "Plugin with path " << absName << " will be skiped!\n";
                        continue;
                    }
                    libs_[name] = std::move(lib);
                }
            }
        }

        Core::~Core()
        {
            /// Сначала удаляем плагины, чтобы они на обратились к несущетсвующей модели
            for (auto& i :  libs_) {
                if (i.second->plugin && i.second->destroy) i.second->destroy(i.second->plugin);
                i.second->plugin = nullptr;
            }
            /// Затем удаляются модели
            models_.reset();
            libs_.clear(); /// И только потом выгружаем символы.
        }

        template <class Fn> Fn sym(void *h_, const char *name)
        {
            dlerror();
            void *p = dlsym(h_, name);
            if (const char *e = dlerror()) {
                std::cout << R_CORE << "Cannot load symbol of plugin loader: dlsym failed: " << name << ": " << e
                          << "\n";
                return nullptr;
            }
            return reinterpret_cast<Fn>(p);
        }

        IPluginLoaderLib::~IPluginLoaderLib()
        {
            if (h_) dlclose(h_);
        }

        std::unique_ptr<IPluginLoaderLib> IPluginLoaderLib::load(std::string path)
        {
            auto h_ = dlopen(path.c_str(), RTLD_NOW);
            if (!h_) {
                std::cout << R_CORE << "Cannot load plugin: dlopen failed: " << path << ": " << dlerror() << "\n";
                return nullptr;
            }
            Create create   = sym<IPlugin *(*)()>(h_, "create_plugin");
            Destroy destroy = sym<void (*)(IPlugin *)>(h_, "destroy_plugin");
            if (create == nullptr || destroy == nullptr) {
                dlclose(h_);
                return nullptr;
            }
            IPlugin *plugin = create();
            if (!plugin) {
                std::cout << R_CORE << "Plugin " << path << " not loaded\n";
                dlclose(h_);
                return nullptr;
            }
            return std::unique_ptr<IPluginLoaderLib>(new IPluginLoaderLib(h_, destroy, plugin));
        }

    }
}