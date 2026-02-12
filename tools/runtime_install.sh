#!/usr/bin/env bash
need_cmd() { command -v "$1" >/dev/null 2>&1 || { echo "Missing required command: $1" >&2; exit 1; }; }
need_cmd curl
need_cmd jq
need_cmd sudo
need_cmd dpkg

sudo apt-get update
sudo apt-get install -y \
    wget curl jq libboost-system-dev libboost-filesystem-dev \
    libboost-program-options-dev libboost-thread-dev libboost-chrono-dev \
    libboost-date-time-dev libboost-atomic-dev libboost-json-dev \
    openssl ca-certificates libcap2-bin

DEFAULT_NAME="PluginWorkspace"
read -r -p "Workspace name [${DEFAULT_NAME}]: " WS_NAME
WS_NAME="${WS_NAME:-$DEFAULT_NAME}"
WS="${PWD}/${WS_NAME}"
echo "\033[32m[0/4]\033[0m Using workspace: ${WS}"
mkdir -p "${WS}/Plugins" "${WS}/core" "${WS}/configs"
cd "${WS}"
echo "\033[32m[1/4]\033[0m Maked path: ${WS}"

API="https://api.github.com/repos/d3156/PluginCore/releases/latest"
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

echo "\033[32m[2/4]\033[0m Download PluginLoader"
download_asset '^PluginLoader_[0-9]+\.[0-9]+\.[0-9]+$'
PLUGINLOADER_NAME="$(get_name_by_regex '^PluginLoader_[0-9]+\.[0-9]+\.[0-9]+$')"
chmod +x "${WS}/${PLUGINLOADER_NAME}"
ln -sf "${WS}/${PLUGINLOADER_NAME}" "${WS}/PluginLoader"

echo "\033[32m[3/4]\033[0m Download d3156-plugincore.deb and d3156-plugincore-dev.deb"
download_asset '^d3156-plugincore_[0-9]+\.[0-9]+\.[0-9]+_amd64\.deb$'

echo "\033[32m[4/4]\033[0m Install deb packages"
sudo dpkg -i "${WS}"/d3156-plugincore*
rm -f "${WS}"/d3156-plugincore*

cat > start.sh << 'EOF'
#!/usr/bin/env bash
export FORMAT="|{date:%H:%M:%S}|{source}|{message}"
export OUT="FILE"
export OUT_DIR="./logs/"
export PER_SOURCE_FILES="true"
export R_LEVEL="1"
export Y_LEVEL="1"
export G_LEVEL="1"
export W_LEVEL="1"
exec ./PluginLoader "$@"
EOF
chmod +x start.sh

echo "\033[32m[OK]\033[0m Workspace ready at ${WS} (no sources, only loader/plugins setup)"
tree -L 2 "${WS}"

