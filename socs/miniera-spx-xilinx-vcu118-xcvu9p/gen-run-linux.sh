# Untar pre-generated sysroot
tar -xvf ../sysroot.tar
cp -rf sysroot/ ../../soft/ariane
rm -rf sysroot

# Build Linux to run the experiments
make linux

# Copy the test programs to soft-build/ariane/sysroot/applications/test
tar -xvf test/miniera.tar
cp -rf miniera test
rm -rf miniera
cp -r test/ soft-build/ariane/sysroot/applications/

make linux fpga-program fpga-run-linux
