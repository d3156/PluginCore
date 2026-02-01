#!/usr/bin/env python3
import re
from dataclasses import dataclass
from pathlib import Path


def ask(prompt: str, default: str | None = None) -> str:
    s = input(f"{prompt}" + (f" [{default}]" if default else "") + ": ").strip()
    return s or (default or "")

def ask_bool(prompt: str, default: bool = False) -> bool:
    s = input(f"{prompt} (y/n) [{"y" if default else "n"}]: ").strip().lower()
    if not s: return default
    return s in ("y", "yes", "1", "true", "да", "д")

def cpp_ident(s: str) -> str:
    s = re.sub(r"[^a-zA-Z0-9_]", "_", s)
    if re.match(r"^[0-9]", s): s = "_" + s
    return s

def write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")

@dataclass
class Opt:
    plugin_name: str
    workspace_dir: Path
    with_model: bool
    model_name: str

def detect_workspace_dir(start: Path) -> Path | None:
    p = start.resolve()
    for _ in range(6):
        if (p / "Plugins").is_dir() and (p / "PluginsSource").is_dir() and (p / "tools").is_dir():
            return p
        if p.parent == p:
            break
        p = p.parent
    return None

def load_template(template_name: str, o: Opt) -> str:
    template_path = Path(__file__).parent / "templates_for_gen_plugins" / template_name
    if not template_path.exists(): raise FileNotFoundError(f"Template not found: {template_path}")
    content = template_path.read_text(encoding="utf-8")
    content = content.replace("{plugin_name}", o.plugin_name)
    content = content.replace("{model_name}", o.model_name)
    if o.with_model:
        content = re.sub(r'^\s*//\s*IF_WITH_MODEL:\s*', '', content, flags=re.MULTILINE)
        content = re.sub(r'^\s*//\s*END_IF:\s*', '', content, flags=re.MULTILINE)
    else:
        content = re.sub(r'^\s*//\s*IF_WITH_MODEL:.*?\n(.*?)^\s*//\s*END_IF:.*?\n', 
                        '', content, flags=re.MULTILINE | re.DOTALL)
    return content

def main():
    print("PluginCore plugin generator (workspace mode)\n")
    ws = detect_workspace_dir(Path.cwd())
    if ws is None:
        print("ERROR: workspace not found.")
        print("Run this script from the workspace directory (where Plugins/ PluginsSource/ tools/ exist).")
        raise SystemExit(1)
    plugin_name = cpp_ident(ask("Plugin project name (will produce lib<Name>.so)", "MyPlugin"))
    with_model = ask_bool("Generate a separate model project (STATIC lib) and link it into the plugin?", True)
    model_name = plugin_name.replace("Plugin","")
    if with_model:
        model_name = cpp_ident(ask("Model project name (folder under PluginsSource/)", f"{model_name}Model"))
    o = Opt(
        plugin_name=plugin_name,
        workspace_dir=ws,
        with_model=with_model,
        model_name=model_name,
    )
    plugin_root = ws / "PluginsSource" / plugin_name
    write(plugin_root / "CMakeLists.txt", load_template("plugin_cmake.txt", o))
    write(plugin_root / "src" / f"{plugin_name}.cpp", load_template("plugin_cpp.txt", o))
    write(plugin_root / "src" / f"{plugin_name}.hpp", load_template("plugin_hpp.txt", o))
    write(plugin_root / "build.sh", load_template("build_sh.txt", o))
    write(plugin_root / ".gitignore", load_template("gitignore.txt", o))
    (plugin_root / "build.sh").chmod(0o755)
    if with_model:
        model_root = ws / "PluginsSource" / model_name
        write(model_root / "CMakeLists.txt", load_template("model_cmake.txt", o))
        write(model_root / "src" / f"{model_name}.hpp", load_template("model_hpp.txt", o))
        write(model_root / "src" / f"{model_name}.cpp", load_template("model_cpp.txt", o))
        write(model_root / "include" / f"{model_name}", load_template("model_include.txt", o))
    print(f"\nOK: generated plugin project: {plugin_root}")
    if with_model:
        print(f"OK: generated model project:  {ws / 'PluginsSource' / model_name}")
    print("\nBuild:")
    print(f"  cd {plugin_root}")
    print("  ./build.sh")
    print(f"\nResulting .so will be placed into: {ws / 'Plugins'}")

if __name__ == "__main__":
    main()