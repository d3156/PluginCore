#!/usr/bin/env bash
DEFAULT_NAME="PluginWorkspace"

read -r -p "Workspace name [${DEFAULT_NAME}]: " WS_NAME
WS_NAME="${WS_NAME:-$DEFAULT_NAME}"

WS="${PWD}/${WS_NAME}"
echo "[*] Using workspace: ${WS}"

mkdir -p "${WS}/Plugins" "${WS}/PluginsSource" "${WS}/tools"

OWNER="d3156"
REPO="PluginCore"
API="https://api.github.com/repos/${OWNER}/${REPO}/releases/latest"

need_cmd() { command -v "$1" >/dev/null 2>&1 || { echo "Missing required command: $1" >&2; exit 1; }; }
need_cmd curl
need_cmd jq
need_cmd sudo
need_cmd dpkg
need_cmd git

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

# --- tools/ из репозитория (sparse checkout) ---
if [ -z "$(ls -A "${WS}/tools" 2>/dev/null || true)" ]; then
  echo "[*] Fetching tools/ from repo..."
  tmpdir="$(mktemp -d)"
  git clone --depth 1 --filter=blob:none --sparse "https://github.com/${OWNER}/${REPO}.git" "${tmpdir}"
  git -C "${tmpdir}" sparse-checkout set tools
  cp -a "${tmpdir}/tools/." "${WS}/tools/"
  rm -rf "${tmpdir}"
fi

# --- PluginLoader_X.Y.Z (бинарник) ---
download_asset '^PluginLoader_[0-9]+\.[0-9]+\.[0-9]+$'
PLUGINLOADER_NAME="$(get_name_by_regex '^PluginLoader_[0-9]+\.[0-9]+\.[0-9]+$')"
chmod +x "${WS}/${PLUGINLOADER_NAME}"

# --- deb пакеты ---
download_asset '^d3156-plugincore_[0-9]+\.[0-9]+\.[0-9]+_amd64\.deb$'
download_asset '^d3156-plugincore-dev_[0-9]+\.[0-9]+\.[0-9]+_amd64\.deb$'

RUNTIME_DEB="${WS}/$(get_name_by_regex '^d3156-plugincore_[0-9]+\.[0-9]+\.[0-9]+_amd64\.deb$')"
DEV_DEB="${WS}/$(get_name_by_regex '^d3156-plugincore-dev_[0-9]+\.[0-9]+\.[0-9]+_amd64\.deb$')"

echo "[*] Installing runtime package: ${RUNTIME_DEB}"
sudo dpkg -i "${RUNTIME_DEB}"

echo "[*] Installing dev package: ${DEV_DEB}"
sudo dpkg -i "${DEV_DEB}"

echo
echo "[OK] Workspace ready at ${WS}"
echo "    PluginLoader:  ${WS}/${PLUGINLOADER_NAME}"
echo "    Plugins:       ${WS}/Plugins"
echo "    PluginsSource: ${WS}/PluginsSource"
echo "    tools:         ${WS}/tools"

rm ${WS}/d3156-plugincore*

printf "Download plugins, models and libs? y|n "
read -r answer

case "$answer" in
    [Yy]*)
        sh ./tools/updateDepsList.sh
        ;;
    *)
        echo "Skipped"
        ;;
esac
