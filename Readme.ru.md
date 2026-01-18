[English](Readme.md) | [Русский](Readme.ru.md)


# PluginCore

PluginCore — C++ библиотека для создания приложения, полностью основанного на плагинах: хост создаёт `PluginCore::Core`, а функциональность поставляется динамическими библиотеками из каталога плагинов.

## Как это работает

При создании `PluginCore::Core(argc, argv)` ядро:
- Загружает плагины из каталога `./Plugins` (или из `PLUGINS_DIR`, если переменная окружения задана).
- Вызывает у каждого плагина `registerArgs()`, затем парсит аргументы командной строки, затем вызывает `registerModels()`.
- После регистрации моделей вызывает `postInit()` у всех моделей, и только потом `postInit()` у всех плагинов.

## Каталог плагинов и имена файлов

По умолчанию плагины ищутся в `./Plugins` (константа `client_plugins_path`).  
Путь можно переопределить переменной окружения `PLUGINS_DIR`.  
Имена `.so` должны соответствовать шаблону:
- `lib<PluginName>.so` (release)
- `lib<PluginName>.Debug.so` (если сборка с `DEBUG`)

## Интерфейс плагина

Плагин реализует интерфейс `PluginCore::IPlugin`.

Обязательное:
- `void registerModels(ModelsStorage &storage)` — регистрация моделей и получение зависимостей через хранилище моделей.

Опциональное:
- `void registerArgs(Args::Builder &bldr)` — регистрация аргументов командной строки (вызывается **до** `registerModels`).
- `void postInit()` — дополнительная инициализация после `postInit()` всех моделей.

## ABI (точки входа)

Каждый плагин обязан экспортировать C-ABI функции с точными именами:
- `extern "C" PluginCore::IPlugin *create_plugin();`
- `extern "C" void destroy_plugin(PluginCore::IPlugin *);`

`Core` создаёт плагин через `create_plugin()` и уничтожает через `destroy_plugin()` (не через `delete`).

## Модели и обмен данными

Связь между плагинами реализована через модели (`PluginCore::IModel`) и их реестр `ModelsStorage`.  
Модель обязана иметь пустой конструктор, а всю инициализацию выполнять в `init()`;  
Порядок удаления моделей контролируется `deleteOrder()`: чем меньше число, тем раньше удаляется модель (поддерживаются отрицательные значения).

Для регистрации/получения модели можно использовать макрос:

```cpp
RegisterModel(model_name, new_model, T)
```

Он вернёт уже существующую модель, если она зарегистрирована, иначе зарегистрирует переданную.

# Минимальный хост (MyApp)

Хост-приложение обычно просто создаёт `PluginCore::Core`:
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

Вся логика “живёт” в плагинах: они могут запускать свои потоки и/или асинхронные задачи внутри `registerModels()/postInit()`.
Можно использовать иной способ реализации основного потока. Это пример, который мне по больше душе. 
Если хочется ипсользовать асинхронную логику, можно не создавать отдельный поток и запускать, например boost::io_context в post_init модели.
Тогда приложение после загрузки плагинов тоже не завершиться, но обработку сигналов остановки придётся делать внутри этой модели. 
Я считаю, что основной поток должен оставаться за Ядром и при запросе корректного завершения от системы через сигнал, передавать команду плагинам, 
а те в свою очередь уже могут в своих потоках использовать асинхронные модели разных библиотек, могут просто крутить свой event loop, главное корректно передать сигнал о завершении работы.

# Пример хоста в PluginLoader

Пример реализации `./PluginLoader`

# Сборка PluginCore

PluginCore собирается как статическая библиотека `PluginCore` (CMake, C++20).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cpack --config build/CPackConfig.cmake -G DEB
```

## Как написать плагин (пошагово)

Испольуй скрипт ./tools/gen_plugin.py 

или

1) Создай shared-библиотеку `libMyPlugin.so` (или `libMyPlugin.Debug.so` при `DEBUG`).[1]
2) Реализуй класс-наследник `PluginCore::IPlugin` и минимум `registerModels(ModelsStorage&)`.[1]
3) Экспортируй `create_plugin()` и `destroy_plugin()` с C ABI.[1]
4) Положи `.so` в `./Plugins` или укажи `PLUGINS_DIR`.[1]

Минимальный каркас:

```cpp
// MyPlugin.cpp
#include <PluginCore/IPlugin.hpp>
#include <PluginCore/IModel.hpp>

class MyPlugin final : public PluginCore::IPlugin {
public:
    void registerModels(PluginCore::ModelsStorage& models) override {
        // Регистрируй свои модели или получай чужие:
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
