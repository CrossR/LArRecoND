#!/bin/bash

# Shared error handler
RunWithError() {
    "$@"
    local status=$?
    if [ $status -ne 0 ]; then
        echo "Error running command: $@"
        exit $status
    fi
}

# Shared PANDORA_PROJECT_DIR inference logic
GetPandoraProjectDir() {
    if [ -z "$PANDORA_PROJECT_DIR" ]; then
        if [ -d "$PWD/LArRecoND" ]; then
            export PANDORA_PROJECT_DIR=$PWD
        elif [ -d "$PWD/../LArRecoND" ]; then
            export PANDORA_PROJECT_DIR=$PWD/..
        elif [ -d "$PWD/../../LArRecoND" ]; then
            export PANDORA_PROJECT_DIR=$PWD/../..
        else
            echo "PANDORA_PROJECT_DIR is not set and cannot be inferred from the current working directory."
            echo "Please set the PANDORA_PROJECT_DIR environment variable to the Pandora project directory."
            exit 1
        fi
    fi
}

# Define any common build flags here.
COMMON_CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=RelWithDebInfo"
COMMON_CMAKE_FLAGS="${COMMON_CMAKE_FLAGS} -DCMAKE_INSTALL_LIBDIR=lib"
COMMON_CMAKE_FLAGS="${COMMON_CMAKE_FLAGS} -DCMAKE_INSTALL_PREFIX=install"
COMMON_CMAKE_FLAGS="${COMMON_CMAKE_FLAGS} -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE"
MAKE_COMMAND="cmake --build . --target install -j4"

CloneIfMissing() {
    local url=$1 dir=$2 version=$3

    if [ -d "$PANDORA_PROJECT_DIR/$dir" ]; then
        echo "$dir directory already exists. Skipping clone."
        return
    fi

    RunWithError git clone "$url" "$PANDORA_PROJECT_DIR/$dir"
    RunWithError git -C "$PANDORA_PROJECT_DIR/$dir" checkout "$version"
}

BuildIfMissing() {

    local name=$1; shift
    local dir="$PANDORA_PROJECT_DIR/$name"

    if [ -d "$dir/build/install" ]; then
        echo "$name already built. Skipping build."
        return
    fi

    RunWithError mkdir -p "$dir/build"
    RunWithError cd "$dir/build"
    RunWithError cmake $COMMON_CMAKE_FLAGS "$@" ..
    RunWithError $MAKE_COMMAND
}

BuildPandoraPFA() {
    CloneIfMissing https://github.com/PandoraPFA/PandoraPFA.git PandoraPFA "$PANDORA_PFA_VERSION"
}

BuildEigen3() {
    # Download Eigen tarball, rather than git clone, as the repo history is massive.
    if [ ! -d "$PANDORA_PROJECT_DIR/eigen" ]; then
        echo "Downloading Eigen ${EIGEN_VERSION}..."
        local tarball="eigen-${EIGEN_VERSION}.tar.gz"
        local url="https://gitlab.com/libeigen/eigen/-/archive/${EIGEN_VERSION}/${tarball}"

        RunWithError cd "$PANDORA_PROJECT_DIR"
        RunWithError curl -L -o "$tarball" "$url"
        RunWithError tar -xzf "$tarball"
        RunWithError rm "$tarball"
        RunWithError mv "eigen-${EIGEN_VERSION}" eigen
    else
        echo "eigen directory already exists. Skipping download."
    fi

    BuildIfMissing eigen \
        -DBUILD_TESTING=OFF \
        -DEIGEN_BUILD_TESTING=OFF \
        -DEIGEN_BUILD_DOC=OFF \
        -DEIGEN_BUILD_BLAS=OFF \
        -DEIGEN_BUILD_LAPACK=OFF \
        -DEIGEN_BUILD_DEMOS=OFF
}

BuildPandoraSDK() {
    CloneIfMissing https://github.com/PandoraPFA/PandoraSDK.git PandoraSDK "$PANDORA_SDK_VERSION"
    BuildIfMissing PandoraSDK \
        -DCMAKE_MODULE_PATH=$PANDORA_PROJECT_DIR/PandoraPFA/cmakemodules
}

BuildPandoraMonitoring() {
    CloneIfMissing https://github.com/PandoraPFA/PandoraMonitoring.git PandoraMonitoring "$PANDORA_MONITORING_VERSION"
    BuildIfMissing PandoraMonitoring \
        -DCMAKE_MODULE_PATH="$PANDORA_PROJECT_DIR/PandoraPFA/cmakemodules;$ROOTSYS/etc/cmake" \
        -DPandoraSDK_DIR=$PANDORA_PROJECT_DIR/PandoraSDK/build/install/lib/cmake/PandoraSDK
}

BuildLArContent() {
    CloneIfMissing https://github.com/PandoraPFA/LArContent.git LArContent "$PANDORA_LAR_CONTENT_VERSION"
    BuildIfMissing LArContent \
        -DPANDORA_MONITORING=ON \
        -DPandoraSDK_DIR=$PANDORA_PROJECT_DIR/PandoraSDK/build/install/lib/cmake/PandoraSDK \
        -DPandoraMonitoring_DIR=$PANDORA_PROJECT_DIR/PandoraMonitoring/build/install/lib/cmake/PandoraMonitoring \
        "$@"
}

BuildLArRecoND() {
    # INFO: Assumed to be cloned already...you have this script!
    BuildIfMissing LArRecoND \
        -DPANDORA_MONITORING=ON \
        -DPandoraSDK_DIR=$PANDORA_PROJECT_DIR/PandoraSDK/build/install/lib/cmake/PandoraSDK \
        -DPandoraMonitoring_DIR=$PANDORA_PROJECT_DIR/PandoraMonitoring/build/install/lib/cmake/PandoraMonitoring \
        -DLArContent_DIR=$PANDORA_PROJECT_DIR/LArContent/build/install/lib/cmake/LArContent \
        "$@"
}

GetMachineLearningData() {
    if [ -d "$PANDORA_PROJECT_DIR/LArMachineLearningData" ]; then
        echo "LArMachineLearningData directory already exists. Skipping!"
        return
    fi

    CloneIfMissing https://github.com/PandoraPFA/LArMachineLearningData.git LArMachineLearningData "$PANDORA_LAR_MLDATA_VERSION"

    # INFO: Get the training files (only on a fresh clone).
    RunWithError cd "$PANDORA_PROJECT_DIR/LArMachineLearningData"
    RunWithError ./download.sh dunend
    RunWithError ./download.sh dune lbl
    RunWithError ./download.sh uboone
}
