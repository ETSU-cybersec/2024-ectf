#!/bin/bash

if [ ! -n "$IN_NIX_SHELL" ]; then
    echo "Please open nix-shell then run."
    exit 1
fi

# Get the name of the current directory
current_dir_name=$(basename "$PWD")

# Check if the current directory name is "2024-ectf"
if [ "$current_dir_name" != "2024-ectf" ]; then
    echo "Changing directory to 2024-ectf..."
    ectf_dir=$(find / -type d -name "2024-ectf" 2>/dev/null)
    cd $ectf_dir
fi


# Check if deployment/global_secrets.h file exists
if [ ! -f "deployment/global_secrets.h" ]; then
    echo "Building deployment"
    poetry run python ectf_tools/build_depl.py -d .
fi

echo "Building AP provisioned for 1 component"
poetry run python ectf_tools/build_ap.py -d . -on ap -od build -p 1234 -t 1234123412341234 -c 1 -ids 0x08 -b "Hello from AP"