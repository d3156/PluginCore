#pragma once
#include "IPlugin.hpp"
#include <memory>

namespace d3156
{
    namespace PluginCore
    {

        class SharedLib; // forward

        class Core
        {
        public:
            Core(int argc, char *argv[]);
            ~Core();

        private:
            void loadPlugins();

            ModelsStorage models_;
            PluginsStorage plugins_;

            // новое:
            std::unordered_map<std::string, std::unique_ptr<SharedLib>> libs_;
            std::unordered_map<std::string, void (*)(IPlugin *)> destroyers_;
        };

    } // namespace PluginCore
}