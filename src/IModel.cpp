#include "IModel.hpp"
#include <iostream>
#include <set>
namespace PluginCore
{
    ModelsStorage::~ModelsStorage()
    {
        std::set<int> orders;
        for (auto i : *this) orders.insert(i.second->deleteOrder());
        for (int ord : orders)
            for (auto i = begin(); i != end();)
                if (i->second->deleteOrder() == ord) {
                    std::cout << W_CORE << "[Приоритет удаления " << ord << "]" << "Удаление"
                              << i->second->name().c_str();
                    delete i->second;
                    this->erase(i++);
                } else
                    ++i;
    }

}