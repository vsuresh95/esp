# Make the directories for storing different baremetal programs under different coherence protocols
# Regular invocation
if [ ! -d reg-spandex/ ]
then
    mkdir reg-spandex/
fi

# ASI
if [ ! -d asi-spandex/ ]
then
    mkdir asi-spandex/
fi


# Compiling baremetal programs
# Sort
echo "For Sort Baremetal"
for length in 32 64 128 256 512 1024
do
    # Regular invocation
    # Spandex
    export IS_ESP=0
    export COH_MODE=2
    make sort_stratus-baremetal-clean sort_stratus-baremetal
    mv soft-build/ariane/baremetal/sort_stratus.exe reg-spandex/sort_stratus-reg-spandex-${length}.exe


    # ASI
    export ENABLE_SM=1
    # Spandex
    export IS_ESP=0
    export COH_MODE=2
    make sort_stratus-baremetal-clean sort_stratus-baremetal
    mv soft-build/ariane/baremetal/sort_stratus.exe asi-spandex/sort_stratus-asi-spandex-${length}.exe
done
