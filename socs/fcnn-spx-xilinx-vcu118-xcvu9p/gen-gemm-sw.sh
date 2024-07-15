# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

# Spandex
if [ ! -d test/gemm-spx/ ]
then
    mkdir test/gemm-spx/
fi

# GEMM
# TODO
for length in 32 64 128 256 512 1024
do
    # Spandex
    GEMM_LEN=$length ENABLE_SM=1 IS_ESP=0 COH_MODE=2 make gemm_stratus-app-clean gemm_stratus-app
    cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/gemm-spx/gemm-spx-${length}.exe

done

