#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
g++ -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation -g -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude src/core/model.cpp src/core/ui.cpp test/native/test_main.cpp -o build/test_native
./build/test_native
