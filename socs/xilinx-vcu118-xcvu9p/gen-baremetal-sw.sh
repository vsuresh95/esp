# For sort
if [ ! -d reg-mesi/ ]
then
    mkdir reg-mesi/
fi
if [ ! -d reg-cdma/ ]
then
    mkdir reg-cdma/
fi
if [ ! -d reg-spandex/ ]
then
    mkdir reg-spandex/
fi

if [ ! -d asi-mesi/ ]
then
    mkdir asi-mesi/
fi
if [ ! -d asi-cdma/ ]
then
    mkdir asi-cdma/
fi
if [ ! -d asi-spandex/ ]
then
    mkdir asi-spandex/
fi

# For sort
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
    # CDMA
    export COH_MODE=1
    make sort_stratus-baremetal-clean sort_stratus-baremetal
    mv soft-build/ariane/baremetal/sort_stratus.exe reg-cdma/sort_stratus-reg-cdma-${length}.exe
    
    # Spandex
    export IS_ESP=0
    export COH_MODE=2
    make sort_stratus-baremetal-clean sort_stratus-baremetal
    mv soft-build/ariane/baremetal/sort_stratus.exe reg-spandex/sort_stratus-reg-spandex-${length}.exe


    # ASI
    export ENABLE_SM=1
    # MESI
    export IS_ESP=1
    export SORT_LEN=$length
    export COH_MODE=0
    make sort_stratus-baremetal-clean sort_stratus-baremetal
    mv soft-build/ariane/baremetal/sort_stratus.exe asi-mesi/sort_stratus-asi-mesi-${length}.exe
    # CDMA
    export COH_MODE=1
    make sort_stratus-baremetal-clean sort_stratus-baremetal
    mv soft-build/ariane/baremetal/sort_stratus.exe asi-cdma/sort_stratus-asi-cdma-${length}.exe
    
    # Spandex
    export IS_ESP=0
    export COH_MODE=2
    make sort_stratus-baremetal-clean sort_stratus-baremetal
    mv soft-build/ariane/baremetal/sort_stratus.exe asi-spandex/sort_stratus-asi-spandex-${length}.exe
done
