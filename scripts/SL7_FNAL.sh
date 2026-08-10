# Setup DUNE environment
source /cvmfs/dune.opensciencegrid.org/products/dune/setup_dune.sh

# Setup various packages
# For libxxhash.so
setup dune_oslibs v1_0_0
# Compiler
setup gcc v12_1_0
# Git
setup git v2_45_1
# ROOT
setup root v6_28_12 -q e26:p3915:prof
# GEANT4 for optional edep-sim
setup geant4 v4_11_2_p02 -q e26:prof
# CMAKE
setup cmake v3_27_4
# Clang formatting
setup clang v15_0_7
# LibTorch for optional Deep Learning vertexing
setup libtorch v2_1_1b -q e26
setup tbb v2021_9_0 -q e26

# For Fermigrid jobs
setup ifdhc
setup jobsub_client
