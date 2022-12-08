# Bare metal app for FFT-FIR-IFFT chain using regular invocations

## Default build steps
```
make ffi_chain_reg_invoke-baremetal
```

## Custom options for make
- `IS_ESP`: ESP or Spandex coherence
- `COH_MODE`: Choice of coherence protocol within ESP or Spandex. Refer `ffi_chain_utils.h` for enumeration.
- `ITERATIONS`: Number of iterations of the chain
- `LOG_LEN`: Size of the FFT/FIR
- `SW_FIR`: Perform FIR on the CPU, instead of hardware accelerator


## Clean
```
make ffi_chain_reg_invoke-baremetal-clean
```
