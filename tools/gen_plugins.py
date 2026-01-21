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


def write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


@dataclass
class Opt:
    plugin_name: str
    plugin_cls: str
    workspace_dir: Path
    with_model: bool
    model_name: str
    model_cls: str


def detect_workspace_dir(start: Path) -> Path | None:
    p = start.resolve()
    for _ in range(6):
        if (p / "Plugins").is_dir() and (p / "PluginsSource").is_dir() and (p / "tools").is_dir():
            return p
        if p.parent == p:
            break
        p = p.parent
    return None


def plugin_cmake(o: Opt) -> str:
    plugin = o.plugin_name
    plugins_out = "${CMAKE_CURRENT_SOURCE_DIR}/../../Plugins"

    model_block = ""
    if o.with_model:
        # Plugin project: PluginsSource/<PluginName>
        # Model project:  PluginsSource/<ModelName>   (sibling directory)
        model_src = "${CMAKE_CURRENT_SOURCE_DIR}/../" + o.model_name
        model_bin = "${CMAKE_CURRENT_BINARY_DIR}/_deps/" + o.model_name

        # Must specify binary_dir when adding out-of-tree / sibling source dir. [web:469]
        model_block = f"""

if(NOT EXISTS "{model_src}/CMakeLists.txt")
    message(STATUS "{model_src} not found, downloading...")
    
    include(FetchContent)
    FetchContent_Declare(
        {model_src}
        GIT_REPOSITORY https://github.com/d3156/{model_src}.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable({model_src})
else()
    add_subdirectory("${{CMAKE_CURRENT_SOURCE_DIR}}/../{model_src}" 
                     "${{CMAKE_BINARY_DIR}}/_deps/{model_src}")
endif()

target_link_libraries({plugin} PRIVATE {o.model_name})
"""

    return f"""cmake_minimum_required(VERSION 3.16)
project({plugin}
  VERSION 1.0.0
  LANGUAGES CXX
)
add_compile_definitions({plugin}_VERSION="${{PROJECT_VERSION}}" )
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC_FILES
  CONFIGURE_DEPENDS
  ${{CMAKE_CURRENT_SOURCE_DIR}}/src/*.cpp
  ${{CMAKE_CURRENT_SOURCE_DIR}}/src/*.hpp
  ${{CMAKE_CURRENT_SOURCE_DIR}}/src/*.h
)

add_library({plugin} SHARED ${{SRC_FILES}})

target_include_directories({plugin}
  PRIVATE
    ${{CMAKE_CURRENT_SOURCE_DIR}}/include
)

# Put resulting .so into workspace/Plugins
set_target_properties({plugin} PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "{plugins_out}"
  RUNTIME_OUTPUT_DIRECTORY "{plugins_out}"
  OUTPUT_NAME "{plugin}"
)
find_package(PluginCore REQUIRED)
target_link_libraries({plugin} PRIVATE d3156::PluginCore)

{model_block}
"""


def model_cmake(o: Opt) -> str:
    # Model target name == model project name (simple & predictable)
    # Plugin will link to this target via target_link_libraries(... PRIVATE <ModelName>) [web:168]
    return f"""cmake_minimum_required(VERSION 3.16)
project({o.model_name} 
  VERSION 1.0.0
  LANGUAGES CXX
)

add_compile_definitions({o.model_name}_VERSION="${{PROJECT_VERSION}}" )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC_FILES
  CONFIGURE_DEPENDS
  ${{CMAKE_CURRENT_SOURCE_DIR}}/src/*.cpp
  ${{CMAKE_CURRENT_SOURCE_DIR}}/include/*.hpp
  ${{CMAKE_CURRENT_SOURCE_DIR}}/include/*.h
)

add_library({o.model_name} STATIC  ${{SRC_FILES}})

target_include_directories({o.model_name}
  PUBLIC
    ${{CMAKE_CURRENT_SOURCE_DIR}}/include
)
"""


def plugin_cpp(o: Opt) -> str:
    model_include = f'#include <{o.model_cls}.hpp>\n' if o.with_model else ""
    register_model = f"        models.registerModel(new {o.model_cls}());" if o.with_model else "        // TODO: models.registerModel(new YourModel());"

    return f"""#include <PluginCore/IPlugin.hpp>
#include <PluginCore/IModel.hpp>

{model_include}
class {o.plugin_cls} final : public d3156::PluginCore::IPlugin {{
public:
    void registerArgs(d3156::Args::Builder& bldr) override {{
        // TODO: bldr.addOption(...) / bldr.addParam(...) / bldr.addFlag(...)
        (void)bldr;
    }}

    void registerModels(d3156::PluginCore::ModelsStorage& models) override {{
{register_model}
        (void)models;
    }}

    void postInit() override {{
        // TODO: start threads / async jobs here if needed
    }}
}};

// ABI required by d3156::PluginCore::Core (dlsym uses exact names)
extern "C" d3156::PluginCore::IPlugin* create_plugin() {{
    return new {o.plugin_cls}();
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


def model_cpp(o: Opt) -> str:
    return f"""#include "{o.model_cls}.hpp"

// Implementation can stay in the header for now.
// Keep this file for future expansion.
"""


def build_sh(o: Opt) -> str:
    return """#!/usr/bin/env bash

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT}/build"

cmake -S "${ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --config Release

echo
echo "[OK] Build finished."
echo "Plugin .so should be in: ${ROOT}/../../Plugins"
"""


def main():
    print("PluginCore plugin generator (workspace mode)\n")

    ws = detect_workspace_dir(Path.cwd())
    if ws is None:
        print("ERROR: workspace not found.")
        print("Run this script from the workspace directory (where Plugins/ PluginsSource/ tools/ exist).")
        raise SystemExit(1)

    plugin_name = cpp_ident(ask("Plugin project name (will produce lib<Name>.so)", "MyPlugin"))
    plugin_cls = cpp_ident(ask("C++ plugin class name", f"{plugin_name}Plugin"))

    with_model = ask_bool("Generate a separate model project (STATIC lib) and link it into the plugin?", True)

    model_name = ""
    model_cls = ""
    if with_model:
        model_name = cpp_ident(ask("Model project name (folder under PluginsSource/)", f"{plugin_name}Model"))
        model_cls = cpp_ident(ask("Model C++ class name", model_name))

    o = Opt(
        plugin_name=plugin_name,
        plugin_cls=plugin_cls,
        workspace_dir=ws,
        with_model=with_model,
        model_name=model_name,
        model_cls=model_cls,
    )

    plugin_root = ws / "PluginsSource" / plugin_name
    write(plugin_root / "CMakeLists.txt", plugin_cmake(o))
    write(plugin_root / "src" / f"{plugin_name}.cpp", plugin_cpp(o))
    write(plugin_root / "build.sh", build_sh(o))
    write(plugin_root / ".gitignore", "build\n.cache\nrelease\n_CPack_Packages\n*.deb\n*/build")
    (plugin_root / "build.sh").chmod(0o755)

    if with_model:
        model_root = ws / "PluginsSource" / model_name
        write(model_root / "CMakeLists.txt", model_cmake(o))
        write(model_root / "include" / f"{model_cls}.hpp", model_hpp(o))
        write(model_root / "src" / f"{model_cls}.cpp", model_cpp(o))

    print(f"\nOK: generated plugin project: {plugin_root}")
    if with_model:
        print(f"OK: generated model project:  {ws / 'PluginsSource' / model_name}")
    print("\nBuild:")
    print(f"  cd {plugin_root}")
    print("  ./build.sh")
    print(f"\nResulting .so will be placed into: {ws / 'Plugins'}")


if __name__ == "__main__":
    main()
