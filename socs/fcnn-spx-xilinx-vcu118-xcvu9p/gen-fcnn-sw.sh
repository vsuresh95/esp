# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

if [ ! -d test/fcnn/ ]
then
    mkdir test/fcnn/
fi

# FCNN
# TODO
# Chaining
# Spandex
ENABLE_SM=1 IS_ESP=1 COH_MODE=0 make gemm_stratus-app-clean gemm_stratus-app
cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/fcnn/fcnn-chaining-spandex.exe

# Pipelining
# Spandex
ENABLE_SM=1 IS_ESP=1 COH_MODE=0 make gemm_stratus-app-clean gemm_stratus-app
cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/fcnn/fcnn-pipelining-spandex.exe.exe

