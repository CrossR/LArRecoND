# Setup the FNAL spack environment
source /cvmfs/larsoft.opensciencegrid.org/spack-packages/setup-env.sh

# cmake and gcc versions
spack load cmake@3.27.7
spack load gcc@12.2.0

# Eigen
spack load eigen@3.4.0

# PyTorch
spack load py-torch@1.13.1/dicc4vw

# ROOT
spack load root@6.28.12

# For copying job files: ifdh cp origFile copyFile
spack load ifdhc@2.6.20

