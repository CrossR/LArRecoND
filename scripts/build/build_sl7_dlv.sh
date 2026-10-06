#!/bin/bash

# Source the top level build script
BUILD_SCRIPT_LOCATION=$(dirname "$0")
BUILD_FUNCTION_SCRIPT_LOCATION="$BUILD_SCRIPT_LOCATION/build_functions.sh"
source "$BUILD_FUNCTION_SCRIPT_LOCATION"

# Ensure that the Pandora project directory is set correctly
GetPandoraProjectDir

# Source the tags.sh script to set the Pandora package versions
if [ -f "$PANDORA_PROJECT_DIR/LArRecoND/scripts/tags.sh" ]; then
    source "$PANDORA_PROJECT_DIR/LArRecoND/scripts/tags.sh" "${PANDORA_PROJECT_DIR}"
    export MY_TEST_AREA="${PANDORA_PROJECT_DIR}"
else
    echo "tags.sh script not found. Please ensure PANDORA_PROJECT_DIR is set correctly."
    exit 1
fi

# Build standard Pandora packages
BuildPandoraPFA
BuildPandoraSDK
BuildPandoraMonitoring

# Build Eigen3 from source
BuildEigen3

# SL7 DL Specific Variables
CVMFS_PREFIX_PATH="/cvmfs/larsoft.opensciencegrid.org/products/libtorch/v2_1_1b/Linux64bit+3.10-2.17-e26/share/cmake"
EIGEN_FLAG="-DEigen3_DIR=$PANDORA_PROJECT_DIR/Eigen3/share/eigen3/cmake/"
TORCH_FLAGS="-DPANDORA_LIBTORCH=ON -DCMAKE_PREFIX_PATH=${CVMFS_PREFIX_PATH}"

# Build LArContent and LArRecoND with DL flags
BuildLArContent $EIGEN_FLAG $TORCH_FLAGS
BuildLArRecoND $TORCH_FLAGS -DLArDLContent_DIR=$PANDORA_PROJECT_DIR/LArContent/build/install/lib/cmake/LArDLContent

# Download the ML Data bits
GetMachineLearningData

cd $PANDORA_PROJECT_DIR
echo "Remember to source LArRecoND/scripts/tags.sh to setup your environment before running!"

