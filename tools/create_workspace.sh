#!/usr/bin/env bash
need_cmd() { command -v "$1" >/dev/null 2>&1 || { echo "Missing required command: $1" >&2; exit 1; }; }
need_cmd curl
need_cmd jq
need_cmd sudo
need_cmd dpkg
need_cmd git
DEFAULT_NAME="PluginWorkspace"
read -r -p "Workspace name [${DEFAULT_NAME}]: " WS_NAME
WS_NAME="${WS_NAME:-$DEFAULT_NAME}"
WS="${PWD}/${WS_NAME}"
echo "\033[32m[0/8]\033[0m Using workspace: ${WS}"

mkdir -p "${WS}/Plugins" "${WS}/PluginsSource" "${WS}/core"  "${WS}/configs"
cd ${WS}

API="https://api.github.com/repos/d3156/PluginCore/releases/latest"

printf "\033[32m[1/8]\033[0m Cloning PluginCore repo."
git clone https://github.com/d3156/PluginCore.git "${WS}/core/PluginCore"
git clone https://github.com/d3156/PluginCore_SourcesList.git "${WS}/core/PluginCore_SourcesList"
ln -s ./core/PluginCore/tools tools

get_url_by_regex() {
  local re="$1"
  curl -fsSL "${API}" \
    | jq -r --arg re "${re}" '.assets[] | select(.name? | test($re)) | .browser_download_url' \
    | head -n1
}

get_name_by_regex() {
  local re="$1"
  curl -fsSL "${API}" \
    | jq -r --arg re "${re}" '.assets[] | select(.name? | test($re)) | .name' \
    | head -n1
}

download_asset() {
  local re="$1"
  local name url
  name="$(get_name_by_regex "${re}")"
  url="$(get_url_by_regex "${re}")"
  if [ -z "${url}" ] || [ "${url}" = "null" ] || [ -z "${name}" ] || [ "${name}" = "null" ]; then
    echo "Asset not found (regex: ${re})" >&2
    exit 1
  fi
  echo "[*] Downloading ${name}"
  curl -fL -o "${WS}/${name}" "${url}"
}

# --- PluginLoader_X.Y.Z (бинарник) ---
echo "\033[32m[2/8]\033[0m Download PluginLoader"
download_asset '^PluginLoader_[0-9]+\.[0-9]+\.[0-9]+$'
PLUGINLOADER_NAME="$(get_name_by_regex '^PluginLoader_[0-9]+\.[0-9]+\.[0-9]+$')"
chmod +x "${WS}/${PLUGINLOADER_NAME}"
echo "\033[32m[3/8]\033[0m Download sources"
sh ./tools/updateDepsList.sh

# --- deb пакеты ---
echo "\033[32m[4/8]\033[0m Download d3156-plugincore.deb"
download_asset '^d3156-plugincore_[0-9]+\.[0-9]+\.[0-9]+_amd64\.deb$'
echo "\033[32m[5/8]\033[0m Download d3156-plugincore-dev.deb"
download_asset '^d3156-plugincore-dev_[0-9]+\.[0-9]+\.[0-9]+_amd64\.deb$'

RUNTIME_DEB="${WS}/$(get_name_by_regex '^d3156-plugincore_[0-9]+\.[0-9]+\.[0-9]+_amd64\.deb$')"
DEV_DEB="${WS}/$(get_name_by_regex '^d3156-plugincore-dev_[0-9]+\.[0-9]+\.[0-9]+_amd64\.deb$')"

echo "\033[32m[6/8]\033[0m Installing runtime package: ${RUNTIME_DEB}"
sudo dpkg -i "${RUNTIME_DEB}"
echo "\033[32m[7/8]\033[0m Installing dev package: ${DEV_DEB}"
sudo dpkg -i "${DEV_DEB}"
rm -f "${WS}"/d3156-plugincore*.deb
# --- deb пакеты ---
echo "\033[32m[8/8]\033[0m  Workspace succes created"
echo "\033[32m[OK]\033[0m  Workspace ready at ${WS}"
tree -L 2 ${WS}