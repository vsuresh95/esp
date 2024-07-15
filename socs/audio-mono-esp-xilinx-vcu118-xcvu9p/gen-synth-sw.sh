# Make the directories for storing different baremetal programs under different coherence protocols
if [ ! -d test/ ]
then
    mkdir test/
fi

# SW
if [ ! -d test/synth-ma-sw/ ]
then
    mkdir test/synth-ma-sw/
fi

# OS
if [ ! -d test/synth-ma-os/ ]
then
    mkdir test/synth-ma-os/
fi

# MESI
if [ ! -d test/synth-ma-mesi/ ]
then
    mkdir test/synth-ma-mesi/
fi

# DMA
if [ ! -d test/synth-ma-dma/ ]
then
    mkdir test/synth-ma-dma/
fi

# Synthetic MA

