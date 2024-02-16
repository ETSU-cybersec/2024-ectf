#!/bin/bash

# Change to project root if not in 2024-ectf
if [[ $current_dir != *"2024-ectf"* ]]; then
    echo "Changing directory to 2024-ectf..."
    ectf_dir=$(find / -type d -name "2024-ectf" 2>/dev/null)
    cd $ectf_dir
fi

# Check if deployment/global_secrets.h file exists
if [ ! -f "deployment/global_secrets.h" ]; then
    echo "Building deployment"
    poetry run python ectf_tools/build_depl.py -d .
fi

# Run the Python commands
echo "Building 2 components"
poetry run python ectf_tools/build_comp.py -d . -on comp1 -od build -id 0x08 -b "Hello from Component 1" -al "Johnson City" -ad "02/13/2024" -ac "ETSU Cyberbucs"
poetry run python ectf_tools/build_comp.py -d . -on comp2 -od build -id 0x09 -b "Hello from Component 2" -al "Johnson City" -ad "02/13/2024" -ac "ETSU Cyberbucs"
