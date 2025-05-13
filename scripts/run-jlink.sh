#!/bin/bash

if [[ "$OSTYPE" == "darwin"* ]]; then
    # Mac OS
    JLINK_CMD="JLinkExe"
else
    # Assume Windows
    JLINK_CMD="JLink.exe"
fi

$JLINK_CMD "$@"
