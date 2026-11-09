#!/usr/bin/env bash

set -euo pipefail

readonly ARCHIVE_NAME="relaunch-on-change-linux-x86_64.tar.gz"
readonly BINARY_NAME="relaunch-on-change"
readonly DOWNLOAD_URL="https://github.com/AntoineBuirey/relaunch-on-change/releases/latest/download/${ARCHIVE_NAME}"
readonly PREFIX="${PREFIX:-/usr/local}"
readonly INSTALL_DIR="${PREFIX}/bin"

if ! command -v curl >/dev/null 2>&1; then
    echo "Erreur : curl est requis." >&2
    exit 1
fi

if ! command -v tar >/dev/null 2>&1; then
    echo "Erreur : tar est requis." >&2
    exit 1
fi

temp_dir="$(mktemp -d)"
trap 'rm -rf "$temp_dir"' EXIT

echo "Téléchargement de la dernière version..."
curl --fail --location --silent --show-error \
    "$DOWNLOAD_URL" \
    --output "$temp_dir/$ARCHIVE_NAME"

tar --extract --gzip --file "$temp_dir/$ARCHIVE_NAME" --directory "$temp_dir"

if [[ ! -x "$temp_dir/$BINARY_NAME" ]]; then
    echo "Erreur : l'archive ne contient pas l'exécutable attendu." >&2
    exit 1
fi

install_command=(install -D -m 755 "$temp_dir/$BINARY_NAME" "$INSTALL_DIR/$BINARY_NAME")
if [[ "$(id -u)" -eq 0 ]]; then
    "${install_command[@]}"
else
    if ! command -v sudo >/dev/null 2>&1; then
        echo "Erreur : sudo est requis pour installer dans $INSTALL_DIR." >&2
        echo "Vous pouvez utiliser PREFIX=\$HOME/.local pour une installation utilisateur." >&2
        exit 1
    fi
    sudo "${install_command[@]}"
fi

echo "relaunch-on-change installé dans $INSTALL_DIR/$BINARY_NAME"
