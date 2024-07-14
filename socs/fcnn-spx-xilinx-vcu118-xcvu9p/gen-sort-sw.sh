# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

# Spandex
if [ ! -d test/sort-spx/ ]
then
    mkdir test/sort-spx/
fi

# Sort
for length in 32 64 128 256 512 1024
do
    # Spandex
    SORT_LEN=$length ENABLE_SM=1 IS_ESP=1 COH_MODE=1 make sort_stratus-app-clean sort_stratus-app
    cp soft-build/ariane/sysroot/applications/test/sort_stratus.exe test/sort-spx/sort-spx-${length}.exe
done

