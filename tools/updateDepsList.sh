#!/usr/bin/env bash
cd PluginsSource
rm list
wget -q https://raw.githubusercontent.com/d3156/PluginCore_SourcesList/main/list

while IFS= read -r repo; do
    dir=$(basename "$repo" .git)
    if [ -d "$dir" ]; then
        cd "$dir" && git pull && cd ..
    else
        git clone "$repo"
    fi
done < list