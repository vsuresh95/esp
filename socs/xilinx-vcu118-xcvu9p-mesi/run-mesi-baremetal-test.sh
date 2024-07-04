# Run each baremetal program
# Regular invocation
echo "Running baremetal programs under MESI and Coherent DMA with regular invocation"
# Sort
for length in 32 64 128 256 512 1024
do
    # MESI
    echo "Under MESI"
    TEST_PROGRAM=./reg-mesi/sort_stratus-reg-mesi-${length}.exe make fpga-program fpga-run
done

