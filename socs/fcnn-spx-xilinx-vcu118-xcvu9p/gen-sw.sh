# repopulate pre-generate socgen folder
tar -xvf socgen.tar

# generate wrappers
make socketgen-distclean socketgen

source ./gen-fcnn-sw.sh
source ./gen-gemm-sw.sh
source ./gen-sort-sw.sh