#!/bin/bash

IFS=$'\n'
for LINE in $(cat badge_info.csv); do
    TEAM=$(echo $LINE | cut -d, -f1)
    ID=$(echo $LINE | cut -d, -f2)
    NAME=$(echo $LINE | cut -d, -f3)
    R=$(echo $LINE | cut -d, -f5)
    G=$(echo $LINE | cut -d, -f6)
    B=$(echo $LINE | cut -d, -f7)

    echo "====================================================================="
    echo "TEAM: $TEAM ID: $ID NAME: $NAME R: $R G: $G B: $B"
    echo "====================================================================="
    read -rsp $'Next: <n> Continue: <any>' -n1 KEY
    echo ""
    if [ "$KEY" != "n" ]; then
        node ./tools/generate_conf.js $TEAM $ID $R $G $B "$NAME"
        platformio run -t uploadfs
        echo "====================================================================="
        read -rsp $'Put in bootloader mode...' -n1
        echo ""
        echo "====================================================================="
        platformio run -t upload
    fi
done