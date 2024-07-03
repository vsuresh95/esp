# Run each baremetal program
echo "Running Linux tests under Spandex"
# Regular invocation
echo "Running Linux apps under Spandex-FCS with regular invocation"
# Sort
for length in 32 64 128 256 512 1024
do
    # Spandex
    echo "Under Spandex-FCS"
    ./reg-spandex/sort_stratus-reg-spandex-${length}.exe
done

# ASI
echo "Running Linux apps under Spandex-FCS with ASI"
# Sort
for length in 32 64 128 256 512 1024
do
    # Spandex
    echo "Under Spandex-FCS"
    ./asi-spandex/sort_stratus-asi-spandex-${length}.exe
done
