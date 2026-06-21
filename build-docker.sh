#!/bin/bash
set -e

echo "Building Bitmark in Ubuntu 24.04 container..."

# Build the image
docker build --no-cache -f Dockerfile.build -t bitmark-builder .

# Extract binaries
echo "Extracting binaries..."
docker create --name bitmark-extract bitmark-builder
docker cp bitmark-extract:/output/bitmarkd ./bitmarkd
docker cp bitmark-extract:/output/bitmark-cli ./bitmark-cli
docker rm bitmark-extract

echo ""
echo "Build complete! Binaries:"
ls -la bitmarkd bitmark-cli
echo ""
echo "Test with: ./bitmarkd --version"
