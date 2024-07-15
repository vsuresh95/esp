# cfake the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

# SW
if [ ! -d test/synth-cfa-sw/ ]
then
    mkdir test/synth-cfa-sw/
fi

# OS
if [ ! -d test/synth-cfa-os/ ]
then
    mkdir test/synth-cfa-os/
fi

# MESI
if [ ! -d test/synth-cfa-mesi/ ]
then
    mkdir test/synth-cfa-mesi/
fi

# DMA
if [ ! -d test/synth-cfa-dma/ ]
then
    mkdir test/synth-cfa-dma/
fi

# Synthetic CFA

