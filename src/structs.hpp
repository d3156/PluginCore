#pragma once

#include <string>
#include <unordered_map>

#define Y_CORE "\033[33m[CORE]\033[0m"
#define R_CORE "\033[31m[CORE]\033[0m"
#define G_CORE "\033[32m[CORE]\033[0m"
#define W_CORE "[CORE]"

namespace PluginCore
{
    typedef std::string model_name;
    typedef std::string plugin_name;

    class IModel;
    class IConnector;
    class IPlugin;

    using PluginsStorage = std::unordered_map<plugin_name, IPlugin *>;

    const std::string client_plugins_path = "./Plugins";
}