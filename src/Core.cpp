#include "Core.hpp"
#include <dlfcn.h>
#include <filesystem>
#include <regex>
#include <unistd.h>
#include <sys/utsname.h>

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
            Args::printHeader(argc, argv);
            Args::Builder bldr;
            bldr.setVersion("d3156::PluginCore " + std::string(PLUGIN_CORE_VERSION));
            loadPlugins();
            for (auto &lib : libs_) lib.second->plugin->registerArgs(bldr);
            for (auto &lib : libs_) lib.second->plugin->registerModels(models_);
            for (auto i : models_) i.second->registerArgs(bldr);
            bldr.parse(argc, argv);
            for (auto i : models_) i.second->postInit();
            for (auto &lib : libs_) lib.second->plugin->postInit();
            if (libs_.empty()) exit(0);
        }

        const std::string client_plugins_path = "./Plugins";

        static std::vector<fs::path> getPaths()
        {
            if (std::getenv("PLUGINS_DIR") == nullptr) {
                G_LOG(0, "Using standart pluging path " << client_plugins_path);
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
                        G_LOG(0, "Directory of plugins " << token << " added");
                    } else
                        Y_LOG(0, "Directory from PLUGINS_DIR not found: " << token);
                }
                start = end + 1;
            }
            return out;
        }

        void Core::loadPlugins()
        {
            std::vector<fs::path> pluginsDir = getPaths();
            if (pluginsDir.empty()) R_LOG(0, "Empty existing path list for loading plugin!");
#ifdef DEBUG
            const std::regex re(R"(^lib([^\.]*)\.Debug\.so$)");
#else
            const std::regex re(R"(^lib([^\.]*)\.so$)");
#endif
            for (const auto &pluginDir : pluginsDir) {
                G_LOG(0, "Loading plugins from dir " << pluginDir);
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
                    G_LOG(0, "Plugin " << name << " loaded");
                    if (libs_.contains(name)) {
                        Y_LOG(0, "Plugin with name " << name << " already loaded!");
                        Y_LOG(0, "Plugin with path " << absName << " will be skiped!");
                        continue;
                    }
                    libs_[name] = std::move(lib);
                }
            }
        }

        Core::~Core()
        {
            G_LOG(0, "Destory CORE");
            for (auto &i : libs_) { /// Сначала удаляем плагины, чтобы они на обратились к несущетсвующей модели
                G_LOG(0, "Destroy plugin " << i.first);
                if (i.second->plugin && i.second->destroy) i.second->destroy(i.second->plugin);
                i.second->plugin = nullptr;
            }
            models_.reset(); /// Затем удаляются модели
            libs_.clear();   /// И только потом выгружаем символы.
            G_LOG(0, "CORE destroyed");
        }

        template <class Fn> Fn sym(void *h_, const char *name)
        {
            dlerror();
            void *p = dlsym(h_, name);
            if (const char *e = dlerror()) {
                R_LOG(0, "Cannot load symbol of plugin loader: dlsym failed: " << name << ": " << e);
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
                R_LOG(0, "Cannot load plugin: dlopen failed: " << path << ": " << dlerror());
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
                R_LOG(0, "Plugin " << path << " not loaded");
                dlclose(h_);
                return nullptr;
            }
            return std::unique_ptr<IPluginLoaderLib>(new IPluginLoaderLib(h_, destroy, plugin));
        }

    }
}