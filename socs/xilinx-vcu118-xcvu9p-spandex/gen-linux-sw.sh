# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi
# Regular invocation
if [ ! -d test/reg-spandex/ ]
then
    mkdir test/reg-spandex/
fi

# ASI
if [ ! -d test/asi-spandex/ ]
then
    mkdir test/asi-spandex/
fi


# Compiling Linux apps
# Sort
echo "For Sort Linux App"
for length in 32 64 128 256 512 1024
do
    # Regular invocation
    export SORT_LEN=$length
    export ENABLE_SM=0    
    # Spandex
    export IS_ESP=0
    export COH_MODE=2
    make sort_stratus-app-clean sort_stratus-app
    mv soft-build/ariane/sysroot/applications/test/sort_stratus.exe test/reg-spandex/sort_stratus-test-reg-spandex-${length}.exe


    # ASI
    export ENABLE_SM=1
    # Spandex
    export IS_ESP=0
    export COH_MODE=2
    make sort_stratus-app-clean sort_stratus-app
    mv soft-build/ariane/sysroot/applications/test/sort_stratus.exe test/asi-spandex/sort_stratus-test-asi-spandex-${length}.exe
done
