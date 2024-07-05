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


# Compiling Linux apps
# Sort
echo "For Sort Linux App"
for length in 32 64 128 256 512 1024
do
    # Regular invocation
    # MESI
    SORT_LEN=$length ENABLE_SM=0 IS_ESP=1 COH_MODE=0 make sort_stratus-app-clean sort_stratus-app
    mv soft-build/ariane/sysroot/applications/test/sort_stratus.exe test/reg-mesi/sort_stratus-test-reg-mesi-${length}.exe
done
