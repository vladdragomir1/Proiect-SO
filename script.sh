#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <file_path>"
    exit 1
fi

KEYWORDS=("corrupt" "dangerous" "risk" "attack" "malware" "malicious")
FILE=$1

for kw in "${KEYWORDS[@]}"; do
    if grep -qi $kw "$FILE"; then
        echo "malicious"
        exit 0
    fi
done

echo "clean"
exit 0
