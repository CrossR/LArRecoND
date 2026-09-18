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

# SL7 Specific: Build Eigen3 from source
BuildEigen3
EIGEN_FLAG="-DEigen3_DIR=$PANDORA_PROJECT_DIR/Eigen3/share/eigen3/cmake/"

# Build LArContent without LibTorch, passing the Eigen flag
BuildLArContent $EIGEN_FLAG
BuildLArRecoND

# Finally, download the ML Data bits
GetMachineLearningData

cd $PANDORA_PROJECT_DIR
echo "Remember to source LArRecoND/scripts/tags.sh to setup your environment before running!"

