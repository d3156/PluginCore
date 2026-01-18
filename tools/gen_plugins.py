#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import annotations
import re
from dataclasses import dataclass
from pathlib import Path

def ask(prompt: str, default: str | None = None) -> str:
    s = input(f"{prompt}" + (f" [{default}]" if default else "") + ": ").strip()
    return s or (default or "")

def ask_bool(prompt: str, default: bool = False) -> bool:
    d = "y" if default else "n"
    s = input(f"{prompt} (y/n) [{d}]: ").strip().lower()
    if not s:
        return default
    return s in ("y", "yes", "1", "true")

def cpp_ident(s: str) -> str:
    s = re.sub(r"[^a-zA-Z0-9_]", "_", s)
    if re.match(r"^[0-9]", s):
        s = "_" + s
    return s

@dataclass
class Opt:
    name: str
    cls: str
    out_dir: Path
    with_model: bool
    model_cls: str

def write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")

def cmake(o: Opt) -> str:
    return f"""cmake_minimum_required(VERSION 3.16)
project({o.name} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library({o.name} SHARED
    src/{o.name}.cpp
)

target_include_directories({o.name}
    PRIVATE
        ${{CMAKE_CURRENT_SOURCE_DIR}}/include
)

# Linux: produce lib{o.name}.so by default
set_target_properties({o.name} PROPERTIES OUTPUT_NAME "{o.name}")
"""

def plugin_cpp(o: Opt) -> str:
    model_include = f'#include "{o.model_cls}.hpp"\n' if o.with_model else ""
    model_stub = ""
    if o.with_model:
        model_stub = f"""
class {o.model_cls} final : public d3156::PluginCore::IModel {{
public:
    d3156::PluginCore::model_name name() override {{ return "{o.model_cls}"; }}
    void init() override {{
        // TODO: init storage/resources here (ctor must be empty by convention)
    }}
}};
"""
    return f"""#include <PluginCore/IPlugin.hpp>
#include <PluginCore/IModel.hpp>
{model_include}

class {o.cls} final : public d3156::PluginCore::IPlugin {{
public:
    void registerArgs(d3156::Args::Builder& bldr) override {{
        // TODO: bldr.addOption(...) / bldr.addParam(...) / bldr.addFlag(...)
    }}

    void registerModels(d3156::PluginCore::ModelsStorage& models) override {{
        // Provide model(s)
{"        models.registerModel(new " + o.model_cls + "());" if o.with_model else "        // TODO: models.registerModel(new YourModel());"}
        // Or consume model(s)
        // auto* other = models.getModel<OtherModel>(\"OtherModel\");
    }}

    void postInit() override {{
        // TODO: start threads / async jobs here if needed
    }}
}};

{model_stub}

// ABI required by d3156::PluginCore::Core (dlsym uses exact names)
extern "C" d3156::PluginCore::IPlugin* create_plugin() {{
    return new {o.cls}();
}}

extern "C" void destroy_plugin(d3156::PluginCore::IPlugin* p) {{
    delete p;
}}
"""

def model_hpp(o: Opt) -> str:
    return f"""#pragma once
#include <PluginCore/IModel.hpp>

class {o.model_cls} final : public d3156::PluginCore::IModel {{
public:
    d3156::PluginCore::model_name name() override {{ return "{o.model_cls}"; }}
    int deleteOrder() override {{ return 0; }}

    void init() override {{
        // TODO: allocate/init resources here
    }}

    void postInit() override {{
        // TODO: optional
    }}
}};
"""

def main():
    print("PluginCore plugin generator\n")

    name = cpp_ident(ask("Имя плагина (будет lib<Name>.so)", "MyPlugin"))
    cls  = cpp_ident(ask("Имя C++ класса плагина", f"{name}Plugin"))

    out_dir = Path(ask("Куда генерировать (директория)", "./plugins")).expanduser()

    with_model = ask_bool("Создать модель-заглушку (IModel)?", False)
    model_cls = cpp_ident(ask("Имя модели (C++ класс)", f"{name}Model")) if with_model else ""

    o = Opt(name=name, cls=cls, out_dir=out_dir, with_model=with_model, model_cls=model_cls)

    root = out_dir / name
    write(root / "CMakeLists.txt", cmake(o))
    write(root / "src" / f"{name}.cpp", plugin_cpp(o))
    if with_model:
        write(root / "include" / f"{model_cls}.hpp", model_hpp(o))

    print(f"\nOK: {root}")

if __name__ == "__main__":
    main()
