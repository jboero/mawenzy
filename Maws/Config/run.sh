#!/bin/bash
# John Boero
# Example of how to run an instance.

# Set MAWS_PLAT to the platform index you'd like to use.  Use clinfo to determine indexes.
export MAWS_PLAT=0
# Set MAWS_DEV to the desired device(s) index within MAWS_PLAT.
export MAWS_DEV=0

export MAWS_BASE=/mnt/gpu/
export MAWS_SRCDIR=~/backing/

export MAWS_SOCK=/tmp/mawd.sock

# Set full path to common src.  If run without -d, our cwd will be /
export MAWS_COMMON="./common_src.cl"

# Create dirs if they don't exist (/mnt/gpu/ in this example)
mkdir -p /mnt/gpu/

#DEBUG
./mawd -d -f -s -o big_writes,max_write=131072,max_read=131072 /mnt/gpu

#RELEASE
#./mawd -o big_writes,max_write=131072,max_read=131072 /mnt/gpu
