# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

# Bare metal
if [ ! -d test/gemm-bm/ ]
then
    mkdir test/gemm-bm/
fi

# OS
if [ ! -d test/gemm-os/ ]
then
    mkdir test/gemm-os/
fi

# MESI
if [ ! -d test/gemm-mesi/ ]
then
    mkdir test/gemm-mesi/
fi

# DMA
if [ ! -d test/gemm-dma/ ]
then
    mkdir test/gemm-dma/
fi

# GEMM
# TODO
for length in 32 64 128 256 512 1024
do
    # Bare metal
    GEMM_LEN=$length ENABLE_SM=0 IS_ESP=1 COH_MODE=0 make gemm_stratus-baremetal-clean gemm_stratus-baremetal
    cp soft-build/ariane/baremetal/gemm_stratus.exe test/gemm-bm/gemm_stratus-gemm-bm-${length}.exe

    # OS
    GEMM_LEN=$length ENABLE_SM=0 IS_ESP=1 COH_MODE=0 make gemm_stratus-app-clean gemm_stratus-app
    cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/gemm-os/gemm_stratus-test-gemm-os-${length}.exe

    # MESI
    GEMM_LEN=$length ENABLE_SM=1 IS_ESP=1 COH_MODE=0 make gemm_stratus-app-clean gemm_stratus-app
    cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/gemm-mesi/gemm_stratus-test-gemm-mesi-${length}.exe

    # DMA
    GEMM_LEN=$length ENABLE_SM=1 IS_ESP=1 COH_MODE=1 make gemm_stratus-app-clean gemm_stratus-app
    cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/gemm-dma/gemm_stratus-test-gemm-dma-${length}.exe

done

