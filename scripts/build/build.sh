#!/bin/bash

# Source the top level build script
BUILD_SCRIPT_LOCATION=$(dirname "$0")
BUILD_FUNCTION_SCRIPT_LOCATION="$BUILD_SCRIPT_LOCATION/build_functions.sh"
source "$BUILD_FUNCTION_SCRIPT_LOCATION"

GetPandoraProjectDir

if [ -f "$PANDORA_PROJECT_DIR/LArRecoND/scripts/tags.sh" ]; then
    source "$PANDORA_PROJECT_DIR/LArRecoND/scripts/tags.sh" "${PANDORA_PROJECT_DIR}"
    export MY_TEST_AREA="${PANDORA_PROJECT_DIR}"
else
    echo "tags.sh script not found. Please ensure PANDORA_PROJECT_DIR is set correctly."
    exit 1
fi

BuildPandoraPFA
BuildPandoraSDK
BuildPandoraMonitoring

# Build Eigen3 from source
BuildEigen3
EIGEN_FLAG="-DEigen3_DIR=$PANDORA_PROJECT_DIR/Eigen3/share/eigen3/cmake/"

# Mac/Linux Specific: Grab prebuilt LibTorch
OS_TYPE=$(uname -s)
ARCH_TYPE=$(uname -m)

RunWithError cd $PANDORA_PROJECT_DIR

if [ ! -d "$PANDORA_PROJECT_DIR/libtorch" ]; then
    echo "Downloading prebuilt LibTorch..."

    if [ "$OS_TYPE" = "Darwin" ]; then
        if [ "$ARCH_TYPE" = "arm64" ]; then
            TORCH_URL="https://download.pytorch.org/libtorch/cpu/libtorch-macos-arm64-${TORCH_VERSION}.zip"
        else
            TORCH_URL="https://download.pytorch.org/libtorch/cpu/libtorch-macos-x86_64-${TORCH_VERSION}.zip"
        fi
    elif [ "$OS_TYPE" = "Linux" ]; then
        TORCH_URL="https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-${TORCH_VERSION}%2Bcpu.zip"
    else
        echo "Unsupported OS for automatic LibTorch download: $OS_TYPE"
        exit 1
    fi

    # Download, unzip, and clean up the archive
    RunWithError curl -o libtorch.zip -L $TORCH_URL
    RunWithError unzip -q libtorch.zip
    RunWithError rm libtorch.zip
fi

TORCH_FLAG="-DTorch_DIR=$PANDORA_PROJECT_DIR/libtorch/share/cmake/Torch"

# Build LArContent and LArRecoND
BuildLArContent -DPANDORA_LIBTORCH=ON $EIGEN_FLAG $TORCH_FLAG
BuildLArRecoND -DPANDORA_LIBTORCH=ON -DLArDLContent_DIR=$PANDORA_PROJECT_DIR/LArContent/build/install/lib/cmake/LArDLContent $TORCH_FLAG

GetMachineLearningData

cd $PANDORA_PROJECT_DIR
echo "Remember to source LArRecoND/scripts/tags.sh to setup your environment before running!"

