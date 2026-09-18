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

# SL7 EDep-Sim Specific: Build edep-sim first
RunWithError cd $PANDORA_PROJECT_DIR
RunWithError git clone https://github.com/ClarkMcGrew/edep-sim
RunWithError cd edep-sim
source setup.sh
export CMAKE_PREFIX_PATH=${EDEP_ROOT}/${EDEP_TARGET}
RunWithError mkdir -p install build
RunWithError cd build
GEANT4_DIR="/cvmfs/larsoft.opensciencegrid.org/products/geant4/v4_10_6_p01c/Linux64bit+3.10-2.17-e19-prof/lib64/Geant4-10.6.1"
RunWithError cmake -DCMAKE_INSTALL_PREFIX=${EDEP_ROOT}/install -DGeant4_DIR=${GEANT4_DIR} ..
RunWithError ${MAKE_COMMAND}

# Build standard Pandora packages
BuildPandoraPFA
BuildPandoraSDK
BuildPandoraMonitoring
BuildEigen3

# Set DL and Eigen variables
CVMFS_PREFIX_PATH="/cvmfs/larsoft.opensciencegrid.org/products/libtorch/v2_1_1b/Linux64bit+3.10-2.17-e26/share/cmake"
EIGEN_FLAG="-DEigen3_DIR=$PANDORA_PROJECT_DIR/Eigen3/share/eigen3/cmake/"
TORCH_FLAGS="-DPANDORA_LIBTORCH=ON -DCMAKE_PREFIX_PATH=${CVMFS_PREFIX_PATH}"

# Build LArContent
BuildLArContent $EIGEN_FLAG $TORCH_FLAGS

# Build LArRecoND with DL and EDep flags
EDEP_FLAGS="-DUSE_EDEPSIM=ON -DEDepSim_DIR=$PANDORA_PROJECT_DIR/edep-sim/build -DGeant4_DIR=${GEANT4_DIR}"
BuildLArRecoND $TORCH_FLAGS $EDEP_FLAGS -DLArDLContent_DIR=$PANDORA_PROJECT_DIR/LArContent/build/install/lib/cmake/LArDLContent

# Download the ML Data bits
GetMachineLearningData

cd $PANDORA_PROJECT_DIR
echo "Remember to source LArRecoND/scripts/tags.sh to setup your environment before running!"

