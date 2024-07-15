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
# Spandex
ENABLE_SM=1 IS_ESP=0 COH_MODE=2 make sort_stratus-app-clean sort_stratus-app
cp soft-build/ariane/sysroot/applications/test/sort_stratus.exe test/sort-spx/sort-spx.exe

