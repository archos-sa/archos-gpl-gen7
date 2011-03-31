#!/bin/sh

export LD_PRELOAD=./sambafsmon.so.1.0.1
exec $1