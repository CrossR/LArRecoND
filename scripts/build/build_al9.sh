#!/bin/bash

# Source the top level build script
BUILD_SCRIPT_LOCATION=$(dirname "$0")
BUILD_FUNCTION_SCRIPT_LOCATION="$BUILD_SCRIPT_LOCATION/build_functions.sh"
source "$BUILD_FUNCTION_SCRIPT_LOCATION"

# First, ensure that the Pandora project directory is set correctly
GetPandoraProjectDir

# Source the tags.sh script to set the Pandora package versions
if [ -f "$PANDORA_PROJECT_DIR/LArRecoND/scripts/tags.sh" ]; then
    source "$PANDORA_PROJECT_DIR/LArRecoND/scripts/tags.sh" "${PANDORA_PROJECT_DIR}"
    export MY_TEST_AREA="${PANDORA_PROJECT_DIR}"
else
    echo "tags.sh script not found in $PANDORA_PROJECT_DIR/LArRecoND/scripts/"
    echo "This is needed to set the various Pandora package versions. Please ensure that the Pandora project directory is set correctly."
    exit 1
fi

# Build the Pandora packages
BuildPandoraPFA
BuildPandoraSDK
BuildPandoraMonitoring

# AL9 Specific: Find Torch via spack and set the Torch_DIR flag for cmake
TORCH_PREFIX=$(spack location -i py-torch@1.13.1/dicc4vw)
TORCH_FLAG="-DTorch_DIR=${TORCH_PREFIX}/lib/python3.9/site-packages/torch/share/cmake/Torch"

# Enable LibTorch for LArContent and LArRecoND builds
BuildLArContent -DPANDORA_LIBTORCH=ON $TORCH_FLAG
BuildLArRecoND -DPANDORA_LIBTORCH=ON -DLArDLContent_DIR=$PANDORA_PROJECT_DIR/LArContent/build/install/lib/cmake/LArDLContent $TORCH_FLAG

# Finally, download the ML Data bits.
GetMachineLearningData

cd $PANDORA_PROJECT_DIR
echo "Remember to source LArRecoND/scripts/tags.sh to setup your environment before running!"

