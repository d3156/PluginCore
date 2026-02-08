#include "IModel.hpp"
#include <iostream>

namespace d3156::PluginCore
{
    ModelsStorage::~ModelsStorage()
    {
        if (!empty()) reset();
    }

    void ModelsStorage::reset()
    {
        for (const int ord : orders)
            for (auto i = begin(); i != end();)
                if (i->second->deleteOrder() == ord) {
                    G_LOG(0, "[DestroyOrder " << ord << "] Destroy " << i->first);
                    delete i->second;
                    this->erase(i++);
                } else
                    ++i;
    }

    void ModelsStorage::finishRegistering()
    {
        G_LOG(0, "Plugin-Model Compatibility Tree:");
        G_LOG(0, std::string(60, '-'));
        size_t model_idx = 0;
        std::unordered_map<std::string, size_t> counts;
        for (const auto &[model, plugins] : plugins_req_model) {
            bool is_last_model    = (++model_idx == plugins_req_model.size());
            auto model_name_clean = model.substr(0, model.find_first_of("_"));
            if (counts.contains(model_name_clean))
                R_LOG(0, (is_last_model ? "└── " : "├── ") << model);
            else
                G_LOG(0, (is_last_model ? "└── " : "├── ") << model);
            counts[model_name_clean]++;
            size_t plugin_idx = 0;
            for (const auto &plugin : plugins) {
                bool is_last_plugin = (++plugin_idx == plugins.size());
                std::string prefix  = is_last_model ? "    " : "│   ";
                prefix += (is_last_plugin ? "└── " : "├── ");

                G_LOG(0, prefix << plugin);
            }
        }
        G_LOG(0, std::string(60, '-'));
        for (auto count : counts)
            if (count.second > 1)
                R_LOG(0, "Attention, multiple versions of model '"
                             << count.first << "' detected (" << count.second << " versions). "
                             << "Plugins using different versions cannot communicate with each other");
        plugins_req_model.clear();
        current_plugin.clear();
    }
}