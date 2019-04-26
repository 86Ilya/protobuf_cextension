#!/bin/sh
set -xe

ulimit -c unlimited

protoc-c --c_out=. deviceapps.proto
protoc  --python_out=. deviceapps.proto

python setup.py test
