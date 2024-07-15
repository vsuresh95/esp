# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
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
# ASI
# MESI
ENABLE_SM=1 IS_ESP=1 COH_MODE=0 make sort_stratus-app-clean sort_stratus-app
mv soft-build/ariane/sysroot/applications/test/sort_stratus.exe test/asi-mesi/sort_stratus-test-asi-mesi.exe
# CDMA
ENABLE_SM=1 IS_ESP=1 COH_MODE=1 make sort_stratus-app-clean sort_stratus-app
mv soft-build/ariane/sysroot/applications/test/sort_stratus.exe test/asi-cdma/sort_stratus-test-asi-cdma.exe
