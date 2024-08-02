# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

# Spandex
if [ ! -d test/gemm/ ]
then
    mkdir test/gemm/
fi

# Chaining
# SPANDEX
ENABLE_SM=1 SPX=1  COH_MODE=2 COMP_MODE=1 make gemm_stratus-app-clean gemm_stratus-app
cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/gemm/gemm-chaining-spx.exe

