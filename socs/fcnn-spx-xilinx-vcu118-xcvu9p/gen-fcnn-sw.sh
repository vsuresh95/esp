# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

if [ ! -d test/fcnn/ ]
then
    mkdir test/fcnn/
fi

# Chaining
# Spandex
ENABLE_SM=1 NUM_DEVICES=3 FCN=1 SPX=1  COH_MODE=2 COMP_MODE=1 make gemm_stratus-app-clean gemm_stratus-app
cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/fcnn/fcnn-chaining-spx.exe

# Pipelining
# Spandex
ENABLE_SM=1 NUM_DEVICES=3 FCN=1 SPX=1  COH_MODE=2 COMP_MODE=2 make gemm_stratus-app-clean gemm_stratus-app
cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/fcnn/fcnn-pipelining-spx.exe

