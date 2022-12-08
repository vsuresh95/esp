# Linux app for FFT-FIR-IFFT chain using shared memory invocations

## Default build steps
```
make ffi_chain_sm_invoke
make examples
make linux
```

## Custom options for make
- `IS_ESP`: ESP or Spandex coherence
- `COH_MODE`: Choice of coherence protocol within ESP or Spandex. Refer `ffi_chain_utils.h` for enumeration.
- `ITERATIONS`: Number of iterations of the chain
- `LOG_LEN`: Size of the FFT/FIR
- `IS_PIPELINE`: Pipeline the accelerator chain


## Clean
```
make examples-clean
```
