#pragma once
#include <dlfcn.h>
#include <stdexcept>
#include <string>

namespace PluginCore {

class SharedLib {
public:
    explicit SharedLib(const std::string& path) {
        h_ = dlopen(path.c_str(), RTLD_NOW);
        if (!h_) throw std::runtime_error(std::string("dlopen failed: ") + dlerror());
    }

    ~SharedLib() {
        if (h_) dlclose(h_);
    }

    SharedLib(const SharedLib&) = delete;
    SharedLib& operator=(const SharedLib&) = delete;

    template <class Fn>
    Fn sym(const char* name) {
        dlerror(); // очистить прошлую ошибку (паттерн из man/док)
        void* p = dlsym(h_, name);
        if (const char* e = dlerror()) {
            throw std::runtime_error(std::string("dlsym failed: ") + name + ": " + e);
        }
        return reinterpret_cast<Fn>(p);
    }

private:
    void* h_{nullptr};
};

} // namespace PluginCore
