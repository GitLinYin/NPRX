#!/usr/bin/env bash

gcc *.c -I../../include -fPIC -rdynamic -shared -o liblmt.so
mv liblmt.so $HOME/lib
