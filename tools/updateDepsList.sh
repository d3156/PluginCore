#!/usr/bin/env bash

workspace=$(realpath "$(dirname "$(realpath $0)")/../../..")
cd ${workspace}/PluginsSource
rm -f list
wget -q https://raw.githubusercontent.com/d3156/PluginCore_SourcesList/main/list

while IFS= read -r repo; do
    [ -z "$repo" ] && continue
    dir=$(basename "$repo" .git)
    echo "Check project $dir from $repo"
    if [ -d "$dir" ]; then
        cd "$dir" && git pull && cd ..
    else
        git clone "$repo"
    fi
done < list