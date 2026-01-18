#include "Core.hpp"

#include "SharedLib.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <regex>
#include <stdexcept>

namespace fs = std::filesystem;
namespace d3156
{
    namespace PluginCore
    {

        Core::Core(int argc, char *argv[])
        {
            Args::Builder bldr;
            loadPlugins();
            for (auto plugin : plugins_) plugin.second->registerArgs(bldr);
            bldr.parse(argc, argv);
            for (auto plugin : plugins_) plugin.second->registerModels(models_);
            for (auto i : models_) i.second->postInit();
            for (auto plugin : plugins_) plugin.second->postInit();
        }

        void Core::loadPlugins()
        {
            fs::path pluginDir;

            if (const char *env = std::getenv("PLUGINS_DIR")) {
                pluginDir = fs::path(env);
                std::cout << G_CORE << "Задан путь до плагинов PLUGINS_DIR = " << pluginDir.string();
            } else {
                pluginDir = fs::path(client_plugins_path);
                std::cout << G_CORE << "Используется стандартный путь до плагинов " << pluginDir.string();
            }

            if (!fs::exists(pluginDir) || !fs::is_directory(pluginDir)) {
                throw std::runtime_error("Каталог плагинов не найден: " + pluginDir.string());
            }

#ifdef DEBUG
            const std::regex re(R"(^lib([^\.]*)\.Debug\.so$)");
#else
            const std::regex re(R"(^lib([^\.]*)\.so$)");
#endif

            for (const auto &de : fs::directory_iterator(pluginDir)) { // итерация по файлам каталога [web:58]
                if (!de.is_regular_file()) continue;

                const std::string file = de.path().filename().string();

                std::smatch m;
                if (!std::regex_match(file, m, re)) continue;

                const std::string name    = m[1].str();
                const std::string absName = de.path().string();

                auto lib = std::make_unique<SharedLib>(absName);

                using CreateFn  = IPlugin *(*)();
                using DestroyFn = void (*)(IPlugin *);

                auto create  = lib->sym<CreateFn>("create_plugin");
                auto destroy = lib->sym<DestroyFn>("destroy_plugin");

                IPlugin *plugin = create();
                if (!plugin) throw std::runtime_error("create_plugin вернул nullptr: " + absName);

                std::cout << G_CORE << "Загружен успешно " << name;

                plugins_[name]    = plugin;
                destroyers_[name] = destroy;
                libs_[name]       = std::move(lib); // важно: держим .so загруженной пока жив plugin
            }
        }

        Core::~Core()
        {
            // Уничтожать через destroy_plugin, не через delete
            for (auto &[name, plugin] : plugins_) {
                if (!plugin) continue;
                auto it = destroyers_.find(name);
                if (it != destroyers_.end() && it->second) { it->second(plugin); }
            }

            plugins_.clear();
            destroyers_.clear();
            libs_.clear(); // после удаления объектов можно dlclose()
        }

    }
}