echo "Running Linux tests under MESI"
# Regular invocation
echo "Running Linux apps under MESI with regular invocation"
# Sort
for length in 32 64 128 256 512 1024
do
    # MESI
    ./reg-mesi/sort_stratus-test-reg-mesi.exe ${length}
done

