# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

# OS
if [ ! -d test/fcnn/ ]
then
    mkdir test/fcnn/
fi

# FCNN
# TODO
# OS
ENABLE_SM=0 IS_ESP=1 COH_MODE=0 make gemm_stratus-app-clean gemm_stratus-app
cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/fcnn/fcnn_stratus-test-os.exe

# Chaining
# MESI
ENABLE_SM=1 IS_ESP=1 COH_MODE=0 make gemm_stratus-app-clean gemm_stratus-app
cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/fcnn/fcnn_stratus-test-chaining-mesi.exe

# DMA
ENABLE_SM=1 IS_ESP=1 COH_MODE=1 make gemm_stratus-app-clean gemm_stratus-app
cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/fcnn/fcnn_stratus-test-chaining-dma.exe

# Pipelining
# MESI
ENABLE_SM=1 IS_ESP=1 COH_MODE=0 make gemm_stratus-app-clean gemm_stratus-app
cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/fcnn/fcnn_stratus-test-pipelining-mesi.exe

# DMA
ENABLE_SM=1 IS_ESP=1 COH_MODE=1 make gemm_stratus-app-clean gemm_stratus-app
cp soft-build/ariane/sysroot/applications/test/gemm_stratus.exe test/fcnn/fcnn_stratus-test-pipelining-dma.exe

