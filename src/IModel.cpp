#include "IModel.hpp"
#include <iostream>
#include <set>

namespace d3156::PluginCore {
    ModelsStorage::~ModelsStorage() {
        if (!empty()) reset();
    }

    void ModelsStorage::reset() {
        std::set<int> orders;
        for (const auto &i: *this) orders.insert(i.second->deleteOrder());
        for (const int ord: orders)
            for (auto i = begin(); i != end();)
                if (i->second->deleteOrder() == ord) {
                    G_LOG(0, "[DestroyOrder " << ord << "] Destroy " << i->second->name().c_str());
                    delete i->second;
                    this->erase(i++);
                } else
                    ++i;
    }
}
