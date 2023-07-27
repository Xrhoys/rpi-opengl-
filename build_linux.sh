#!/bin/bash
echo "Building assets..."

/bin/bash ./build_asset.sh

echo "Compiling for Linux..."

/bin/bash ./build.sh

echo "Finished"