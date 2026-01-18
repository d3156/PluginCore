[English](Readme.md) | [Русский](Readme.ru.md)

# PluginCore

PluginCore is a C++ library for building a fully plugin-based application: the host creates `PluginCore::Core`, and functionality is provided by dynamic libraries from the plugins directory.

## How it works

When `PluginCore::Core(argc, argv)` is created, the core:

- Loads plugins from `./Plugins` (or from `PLUGINS_DIR` if the environment variable is set).
- Calls `registerArgs()` for each plugin, then parses command-line arguments, then calls `registerModels()`.
- After all models are registered, calls `postInit()` for all models, and only then calls `postInit()` for all plugins.

## Plugins directory and filenames

By default, plugins are searched in `./Plugins` (constant `client_plugins_path`).
The path can be overridden via the `PLUGINS_DIR` environment variable.

Shared library names must match:

- `lib<PluginName>.so` (release)
- `lib<PluginName>.Debug.so` (when built with `DEBUG`)

## Plugin interface

A plugin implements the `PluginCore::IPlugin` interface.

Required:

- `void registerModels(ModelsStorage &storage)` — register models and obtain dependencies via the models registry.

Optional:

- `void registerArgs(Args::Builder &bldr)` — register command-line arguments (called **before** `registerModels`).
- `void postInit()` — extra initialization after `postInit()` of all models.

## ABI (entry points)

Each plugin must export the following C-ABI functions with exact names:

- `extern "C" PluginCore::IPlugin *create_plugin();`
- `extern "C" void destroy_plugin(PluginCore::IPlugin *);`

`Core` creates a plugin via `create_plugin()` and destroys it via `destroy_plugin()` (not via `delete`).

## Models and data exchange

Communication between plugins is implemented via models (`PluginCore::IModel`) and their registry `ModelsStorage`.
A model must have a default constructor and perform all initialization in `init()`.

Model destruction order is controlled by `deleteOrder()`: the smaller the value, the earlier the model is destroyed (negative values are supported).

To register/get a model you can use the macro:

```cpp
RegisterModel(model_name, new_model, T)
```

It returns an existing model if it is already registered; otherwise it registers the provided one.
Minimal host (MyApp)

A host application typically just creates `PluginCore::Core`:
```cpp
#include <PluginCore/Core.hpp>

#include <csignal>
#include <cerrno>
#include <cstring>
#include <unistd.h>

static volatile sig_atomic_t g_stop = 0;

static void on_term(int /*signum*/) {
    g_stop = 1;
}

int main(int argc, char* argv[]) {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_term;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    d3156::PluginCore::Core core(argc, argv);

    while (!g_stop) pause();
    return 0;
}
```
All “real logic” lives in plugins: they can start their own threads and/or async tasks inside `registerModels()/postInit()`.

You can implement the main thread differently—this is just an example.
If you want fully async logic, you can avoid creating a separate thread and run (for example) `boost::io_context` in a model’s `postInit()`.
In that case the application will not exit after loading plugins, but shutdown signal handling will need to be implemented inside that model.

The idea is that the main thread should belong to the Core. When the OS requests a graceful shutdown via a signal, the Core should propagate a shutdown command to plugins.
Plugins can then use any async model in their own threads (different libraries, their own event loops, etc.) as long as the shutdown signal is handled correctly.
# PluginLoader host example

Example implementation: `./PluginLoader`

# Building PluginCore

PluginCore is built as a static library PluginCore (CMake, C++20).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cpack --config build/CPackConfig.cmake -G DEB
```

# Writing a plugin (step-by-step)
Use the script `./tools/gen_plugin.py` or

1. Create a shared library libMyPlugin.so (or libMyPlugin.Debug.so for DEBUG).

2. Implement a class derived from PluginCore::IPlugin and at minimum registerModels(ModelsStorage&).

3. Export create_plugin() and destroy_plugin() with C ABI.

4. Put the .so into ./Plugins or set PLUGINS_DIR.

Minimal skeleton:

```cpp
// MyPlugin.cpp
#include <PluginCore/IPlugin.hpp>
#include <PluginCore/IModel.hpp>

class MyPlugin final : public PluginCore::IPlugin {
public:
    void registerModels(PluginCore::ModelsStorage& models) override {
        // Register your models or obtain existing ones:
        // auto* m = models.getModel<SomeModel>("SomeModel");
        // models.registerModel(new MyModel());
    }
};

extern "C" PluginCore::IPlugin* create_plugin() {
    return new MyPlugin();
}

extern "C" void destroy_plugin(PluginCore::IPlugin* p) {
    delete p;
}

```
