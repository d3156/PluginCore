# Рабочий пример простого загрузчика плагинов на базе d3156::PluginCore

# Зависимости для сборки:

```
sudo dpkg -i d3156-plugincore_amd64.deb
sudo dpkg -i d3156-plugincore-dev_amd64.deb
```

# Сборка

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Собранная версия статическая, зависимостей не имеет.
