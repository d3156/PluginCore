#include "IModel.hpp"
#include <iostream>
#include <set>
namespace d3156
{
    namespace PluginCore
    {
        ModelsStorage::~ModelsStorage()
        {
            if (size()) reset();
        }

        void ModelsStorage::reset()
        {
            std::set<int> orders;
            for (auto i : *this) orders.insert(i.second->deleteOrder());
            for (int ord : orders)
                for (auto i = begin(); i != end();)
                    if (i->second->deleteOrder() == ord) {
                        std::cout << G_CORE << "[DestroyOrder " << ord << "] Destroy " << i->second->name().c_str()
                                  << "\n";
                        delete i->second;
                        this->erase(i++);
                    } else
                        ++i;
        }
    }
}