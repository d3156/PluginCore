#pragma once
#include "IPlugin.hpp"
#include <memory>
#include <unordered_map>

namespace d3156
{
    namespace PluginCore
    {

        class IPluginLoaderLib;

        class Core
        {
        public:
            Core(int argc, char *argv[]);
            ~Core();

        private:
            void loadPlugins();

            ModelsStorage models_;
            std::unordered_map<std::string, std::unique_ptr<IPluginLoaderLib>> libs_;
        };

    } // namespace PluginCore
}