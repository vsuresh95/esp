# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi
# Regular invocation
if [ ! -d test/reg-mesi/ ]
then
    mkdir test/reg-mesi/
fi

# ASI
if [ ! -d test/asi-mesi/ ]
then
    mkdir test/asi-mesi/
fi
if [ ! -d test/asi-cdma/ ]
then
    mkdir test/asi-cdma/
fi


# Compiling Linux apps
# Sort
echo "For Sort Linux App"
for length in 32 64 128 256 512 1024
do
    # Regular invocation
    export SORT_LEN=$length
    export ENABLE_SM=0
    # MESI
    export IS_ESP=1
    export COH_MODE=0
    make sort_stratus-app-clean sort_stratus-app
    mv soft-build/ariane/sysroot/applications/test/sort_stratus.exe test/reg-mesi/sort_stratus-test-reg-mesi-${length}.exe


    # ASI
    export ENABLE_SM=1
    # MESI
    export IS_ESP=1
    export SORT_LEN=$length
    export COH_MODE=0
    make sort_stratus-app-clean sort_stratus-app
    mv soft-build/ariane/sysroot/applications/test/sort_stratus.exe test/asi-mesi/sort_stratus-test-asi-mesi-${length}.exe
    # CDMA
    export COH_MODE=1
    make sort_stratus-app-clean sort_stratus-app
    mv soft-build/ariane/sysroot/applications/test/sort_stratus.exe test/asi-cdma/sort_stratus-test-asi-cdma-${length}.exe
done
