#!/bin/bash
# Добавляет CMake алиасы в ~/.bashrc если нет
BASHRC="$HOME/.bashrc"

if ! grep -q "^alias cmbr=" "$BASHRC" 2>/dev/null ||
   ! grep -q "^alias cmbd=" "$BASHRC" 2>/dev/null ||
   ! grep -q "^alias cmbrc=" "$BASHRC" 2>/dev/null ||
   ! grep -q "^alias cmbdc=" "$BASHRC" 2>/dev/null; then
    
    echo "Add CMake Build Aliases in $BASHRC"
    cat >> "$BASHRC" << 'EOF'
# ═══════════════════════════════════════════════════════════════════════════════
# CMake Build Aliases (added automaticly)
# ═══════════════════════════════════════════════════════════════════════════════
alias cmbr='cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build'
alias cmbd='cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build-debug'
alias cmbrc='rm -rf build && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build'
alias cmbdc='rm -rf build-debug && cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build-debug'
EOF

    echo "Success! use cmbr or cmbd or cmbrc or cmbdc"
else
    echo "Nothing to add in ~/.bashrc"
fi
