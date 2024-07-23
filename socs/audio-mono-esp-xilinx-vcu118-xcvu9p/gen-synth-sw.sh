# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

# Synthetic MA
work=(150 1500)
test_size=("Small" "Large")
cohmodes=("MESI" "DMA" "SPX")
test_types=("Linux" "Chaining" "Pipelining")
hw_types=("ma" "cfa")

for cohcode in {1,2};
do
COH_CODE=$cohcode NUM_STAGES=15 make tiled_app_stratus-app-clean tiled_app_stratus-app;
mv soft-build/ariane/sysroot/applications/test/tiled_app_stratus.exe test/tiled_app_stratus_ma_${cohmodes[cohcode-1]}.exe
done;
