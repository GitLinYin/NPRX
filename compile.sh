#!/usr/bin/env bash

echo "start complie......."

gcc ./src/*.c ./main/*.c -I./include -I./ -lpthread -o nprx
