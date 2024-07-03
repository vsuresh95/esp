# Make the directories for storing different baremetal programs under different coherence protocols
# Regular invocation
if [ ! -d reg-mesi/ ]
then
    mkdir reg-mesi/
fi
if [ ! -d reg-cdma/ ]
then
    mkdir reg-cdma/
fi

# ASI
if [ ! -d asi-mesi/ ]
then
    mkdir asi-mesi/
fi
if [ ! -d asi-cdma/ ]
then
    mkdir asi-cdma/
fi


# Compiling baremetal programs
# Sort
echo "For Sort Baremetal"
for length in 32 64 128 256 512 1024
do
    # Regular invocation
    export SORT_LEN=$length
    export ENABLE_SM=0
    # MESI
    export IS_ESP=1
    export COH_MODE=0
    make sort_stratus-baremetal-clean sort_stratus-baremetal
    mv soft-build/ariane/baremetal/sort_stratus.exe reg-mesi/sort_stratus-reg-mesi-${length}.exe
done
