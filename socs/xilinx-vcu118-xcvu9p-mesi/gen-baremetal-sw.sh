# Make the directories for storing different baremetal programs under different coherence protocols
# Regular invocation
if [ ! -d reg-mesi/ ]
then
    mkdir reg-mesi/

# Compiling baremetal programs
# Sort
echo "For Sort Baremetal"
echo "ITERATIONS=${ITERATIONS}"
for length in 32 64 128 256 512 1024
do
    # Regular invocation
    # MESI
    SORT_LEN=$length ENABLE_SM=0 IS_ESP=1 COH_MODE=0 make sort_stratus-baremetal-clean sort_stratus-baremetal
    mv soft-build/ariane/baremetal/sort_stratus.exe reg-mesi/sort_stratus-reg-mesi-${length}.exe
done

