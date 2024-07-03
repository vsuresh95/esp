# Run each baremetal program
# Regular invocation
echo "Running baremetal programs under Spandex-FCS with regular invocation"
# Sort
for length in 32 64 128 256 512 1024
do
    # Spandex
    echo "Under Spandex-FCS"
    TEST_PROGRAM=./reg-spandex/sort_stratus-reg-spandex-${length}.exe make fpga-program fpga-run
done

# ASI
echo "Running baremetal programs under Spandex-FCS with ASI"
# Sort
for length in 32 64 128 256 512 1024
do
    # Spandex
    echo "Under Spandex-FCS"
    TEST_PROGRAM=./asi-spandex/sort_stratus-asi-spandex-${length}.exe make fpga-program fpga-run
done
