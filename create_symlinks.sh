#!/bin/bash
cd "$1"
for dir in flight cycles flip buttonfly; do
    if [ -d "$dir" ]; then
        for f in "$dir"/*; do
            ln -sf "$f" . 2>/dev/null
        done
    fi
done
