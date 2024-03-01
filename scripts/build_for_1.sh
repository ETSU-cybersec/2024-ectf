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

./scripts/build_1_comp.sh &
./scripts/build_ap_1_comp.sh &
wait
echo "Built 1 Component and AP"