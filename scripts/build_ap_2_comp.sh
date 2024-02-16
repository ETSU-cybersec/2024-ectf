#!/bin/bash

# Change to project root if not in 2024-ectf
if [[ $current_dir != *"2024-ectf"* ]]; then
    echo "Changing directory to 2024-ectf..."
    ectf_dir=$(find / -type d -name "2024-ectf" 2>/dev/null)
    cd $ectf_dir
fi

# Check if deployment/global_secrets.h file exists
if [ ! -f "deployment/global_secrets.h" ]; then
    poetry run python ectf_tools/build_depl.py -d .
fi

echo "Building AP provisioned for 2 component"
poetry run python ectf_tools/build_ap.py -d . -on ap -od build -p 1234 -t 1234123412341234 -c 2 -ids 0x08,0x09 -b "Hello from AP"