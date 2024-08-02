# repopulate pre-generate socgen folder
tar -xvf socgen.tar

# generate wrappers
make socketgen-distclean socketgen

source ./gen-fft-sw.sh