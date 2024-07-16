'''
This is the file to generate the results in .csv formats for comparison with the results in the paper
'''
import csv
import os
import sys


coherence = ['MESI', 'DMA', 'Spandex']  # 3 coherence in total
coherenceLength = len(coherence)

'''
Single-accelerator benchmarks
'''
# For FFT
fftSize = ['128', '256', '512', '1024', '2048', '4096', '8192', '16384', '32768']   # 9 sizes in total
fftSizeLength = len(fftSize)
# Baseline
fftBase = [[100] for _ in range(fftSizeLength)]
# Linux (OS)
fftLinux = [[1] for _ in range(fftSizeLength)]
# Baremetal
fftBM = [[1] for _ in range(fftSizeLength)]
# ASI
fftASI_CPUW = [[[1] for _ in range(coherenceLength)] for _ in range(fftSizeLength)]    # Inner: MESI, DMA, Spandex
fftASI_CPUR = [[[1] for _ in range(coherenceLength)] for _ in range(fftSizeLength)]
fftASI_Acc = [[[1] for _ in range(coherenceLength)] for _ in range(fftSizeLength)]
fftASI_Total = [[[1] for _ in range(coherenceLength)] for _ in range(fftSizeLength)]

# For Sort
sortSize = ['32', '64', '128', '256', '512', '1024']    # 6 sizes in total
sortSizeLength = len(sortSize)
# Baseline
sortBase = [[100] for _ in range(sortSizeLength)]
# Linux (OS)
sortLinux = [[1] for _ in range(sortSizeLength)]
# Baremetal
sortBM = [[1] for _ in range(sortSizeLength)]
# ASI
sortASI_CPUW = [[[1] for _ in range(coherenceLength)] for _ in range(sortSizeLength)]    # Inner: MESI, DMA, Spandex
sortASI_CPUR = [[[1] for _ in range(coherenceLength)] for _ in range(sortSizeLength)]
sortASI_Acc = [[[1] for _ in range(coherenceLength)] for _ in range(sortSizeLength)]
sortASI_Total = [[[1] for _ in range(coherenceLength)] for _ in range(sortSizeLength)]

# For GEMM
gemmSize = ['1x64x64', '8x8x8', '16x16x16', '16x32x16', '32x32x32', '32x64x32', '64x64x64'] # 7 sizes in total
gemmSizeLength = len(gemmSize)
# Baseline
gemmBase = [[100] for _ in range(gemmSizeLength)]
# Linux (OS)
gemmLinux = [[1] for _ in range(gemmSizeLength)]
# Baremetal
gemmBM = [[1] for _ in range(gemmSizeLength)]
# ASI
gemmASI_CPUW = [[[1] for _ in range(coherenceLength)] for _ in range(gemmSizeLength)]    # Inner: MESI, DMA, Spandex
gemmASI_CPUR = [[[1] for _ in range(coherenceLength)] for _ in range(gemmSizeLength)]
gemmASI_Acc = [[[1] for _ in range(coherenceLength)] for _ in range(gemmSizeLength)]
gemmASI_Total = [[[1] for _ in range(coherenceLength)] for _ in range(gemmSizeLength)]

'''
Multi-accelerator benchmarks
'''
# For Audio
audioComponent = ['Psycho', 'Rotate', 'Zoomer', 'Binaur']
audioComponentLength = len(audioComponent)
# Baseline
audioBase = [100]
# Linux (OS)
audioLinux = [[1] for _ in range(audioComponentLength)] # Disaggregated audio
monoAudioLinux = [[1] for _ in range(audioComponentLength)] # Monolithic audio
# ASI
audioASI_chain = [[[1] for _ in range(coherenceLength)] for _ in range(audioComponentLength)] # Inner: MESI, DMA, Spandex
monoAudioASI_chain = [[[1] for _ in range(coherenceLength)] for _ in range(audioComponentLength)] # Inner: MESI, DMA, Spandex
audioASI_pipe = [[[1] for _ in range(coherenceLength)] for _ in range(audioComponentLength)]
monoAudioASI_pipe = [[[1] for _ in range(coherenceLength)] for _ in range(audioComponentLength)]

# For Mini-ERA
eraComponent = ['Radar', 'Viterbi']
eraComponentLength = len(eraComponent)
# Baseline
eraBase = [100]
# Linux (OS)
eraLinux = [[1] for _ in range(eraComponentLength)]
# ASI
eraASI = [[[1] for _ in range(coherenceLength)] for _ in range(eraComponentLength)] # Inner: MESI, DMA, Spandex

# For FCNN
# Baseline
fcnnBase = [100]
# Linux (OS)
fcnnLinux = [1]
# ASI
fcnnASI_chain = [[1] for _ in range(coherenceLength)] # MESI, DMA, Spandex
fcnnASI_pipe = [[1] for _ in range(coherenceLength)]

# For synthetic
synSize = ['Small', 'Large']
synSizeLength = len(synSize)
synAcc = ['1', '3', '5', '7', '10', '12', '13', '14', '15']
synAccLength = len(synAcc)
# Baseline
synBase = [[1] for _ in range(synSizeLength)]
synLinux = [[[1] for _ in range(synAccLength)] for _ in range(synSizeLength)]
# ASI
synASI_chain = [[[[1] for _ in range(coherenceLength)] for _ in range(synAccLength)] for _ in range(synSizeLength)]
synASI_pipe = [[[[1] for _ in range(coherenceLength)] for _ in range(synAccLength)] for _ in range(synSizeLength)]
monoSynASI_pipe = [[[[1] for _ in range(coherenceLength)] for _ in range(synAccLength)] for _ in range(synSizeLength)]



'''
Data parsing
'''
def dataParsing(logFile):
    fileRead = open(logFile, 'r')
    lines = fileRead.readlines()
    # Parsing the file
    for line in lines:
        if line:
            chars = line.split()
            if chars:
                if chars[0] != 'Result:':
                    continue    # This line is not about the results
                if 'random:' in chars:
                    continue

                # Single-accelerator benchmarks first
                if chars[1] == 'FFT':
                    if chars[2] == 'SW':
                        fftBase[fftSize.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'Linux':
                        fftLinux[fftSize.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'Baremetal':
                        fftBM[fftSize.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'ASI':
                        if chars[5] == 'CPU_Write':
                            fftASI_CPUW[fftSize.index(chars[3])][coherence.index(chars[4])] = [int(chars[-1])]
                        elif chars[5] == 'CPU_Read':
                            fftASI_CPUR[fftSize.index(chars[3])][coherence.index(chars[4])] = [int(chars[-1])]
                        elif chars[5] == 'Acc':
                            fftASI_Acc[fftSize.index(chars[3])][coherence.index(chars[4])] = [int(chars[-1])]
                        elif chars[5] == 'Total':
                            fftASI_Total[fftSize.index(chars[3])][coherence.index(chars[4])] = [int(chars[-1])]
                        else:
                            print("Unknown ASI case for fft: " + chars[5])
                    else:
                        print("Unknown case for fft: " + chars[2])
                elif chars[1] == 'Sort':
                    if chars[2] == 'SW':
                        sortBase[sortSize.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'Linux':
                        sortLinux[sortSize.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'Baremetal':
                        sortBM[sortSize.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'ASI':
                        if chars[5] == 'CPU_Write':
                            sortASI_CPUW[sortSize.index(chars[3])][coherence.index(chars[4])] = [int(chars[-1])]
                        elif chars[5] == 'CPU_Read':
                            sortASI_CPUR[sortSize.index(chars[3])][coherence.index(chars[4])] = [int(chars[-1])]
                        elif chars[5] == 'Acc':
                            sortASI_Acc[sortSize.index(chars[3])][coherence.index(chars[4])] = [int(chars[-1])]
                        elif chars[5] == 'Total':
                            sortASI_Total[sortSize.index(chars[3])][coherence.index(chars[4])] = [int(chars[-1])]
                        else:
                            print("Unknown ASI case for sort: " + chars[5])
                    else:
                        print("Unknown case for sort: " + chars[2])
                elif chars[1] == 'GEMM':
                    if chars[2] == 'SW':
                        gemmBase[gemmSize.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'Linux':
                        gemmLinux[gemmSize.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'Baremetal':
                        gemmBM[gemmSize.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'ASI':
                        if chars[5] == 'CPU_Write':
                            gemmASI_CPUW[gemmSize.index(chars[3])][coherence.index(chars[4])] = [int(chars[-1])]
                        elif chars[5] == 'CPU_Read':
                            gemmASI_CPUR[gemmSize.index(chars[3])][coherence.index(chars[4])] = [int(chars[-1])]
                        elif chars[5] == 'Acc':
                            gemmASI_Acc[gemmSize.index(chars[3])][coherence.index(chars[4])] = [int(chars[-1])]
                        elif chars[5] == 'Total':
                            gemmASI_Total[gemmSize.index(chars[3])][coherence.index(chars[4])] = [int(chars[-1])]
                        else:
                            print("Unknown ASI case for gemm: " + chars[5])
                    else:
                        print("Unknown case for gemm: " + chars[2])
                # Now multi-accelerator benchmark
                elif chars[1] == 'Audio':   # Disaggregated audio acceleration
                    if chars[2] == 'SW':
                        audioBase[0] = int(chars[-1])
                    elif chars[2] == 'Linux':
                        audioLinux[audioComponent.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'Chaining':
                        audioASI_chain[audioComponent.index(chars[4])][coherence.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'Pipelining':
                        audioASI_pipe[audioComponent.index(chars[4])][coherence.index(chars[3])] = [int(chars[-1])]
                    else:
                        print("Unknown case for audio: " + chars[2])
                elif chars[1] == 'Mono_Audio':   # Monolithic audio acceleration
                    if chars[2] == 'Linux':
                        monoAudioLinux[audioComponent.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'Chaining':
                        monoAudioASI_chain[audioComponent.index(chars[4])][coherence.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'Pipelining':
                        monoAudioASI_pipe[audioComponent.index(chars[4])][coherence.index(chars[3])] = [int(chars[-1])]
                    else:
                        print("Unknown case for mono_audio: " + chars[2])
                elif chars[1] == 'ERA':
                    if chars[2] == 'SW':
                        eraBase[0] = int(chars[-1])
                    elif chars[2] == 'Linux':
                        eraLinux[eraComponent.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'Chaining':
                        eraASI[eraComponent.index(chars[4])][coherence.index(chars[3])] = [int(chars[-1])]
                    else:
                        print("Unknown case for era: " + chars[2])
                elif chars[1] == 'FCNN':
                    if chars[2] == 'SW':
                        fcnnBase[0] = int(chars[-1])
                    elif chars[2] == 'Linux':
                        fcnnLinux[0] = int(chars[-1])
                    elif chars[2] == 'Chaining':
                        fcnnASI_chain[coherence.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'Pipelining':
                        fcnnASI_pipe[coherence.index(chars[3])] = [int(chars[-1])]
                    else:
                        print("Unknown case for era: " + chars[2])
                # Synthetic benchmarks
                elif chars[1] == 'Synthetic':
                    if chars[2] == 'SW':
                        synBase[synSize.index(chars[3])] = [int(chars[-1])]
                    elif chars[2] == 'Linux':
                        synLinux[synSize.index(chars[3])][synAcc.index(chars[4])] = [int(chars[-1])]
                    elif chars[2] == 'Chaining':
                        synASI_chain[synSize.index(chars[3])][synAcc.index(chars[4])][coherence.index(chars[5])] = [int(chars[-1])]
                    elif chars[2] == 'Pipelining':
                        synASI_pipe[synSize.index(chars[3])][synAcc.index(chars[4])][coherence.index(chars[5])] = [int(chars[-1])]
                    else:
                        print("Unknown case for synthetic: " + chars[2])
                elif chars[1] == 'Mono_Synthetic':
                    if chars[2] == 'Pipelining':
                        monoSynASI_pipe[synSize.index(chars[3])][synAcc.index(chars[4])][coherence.index(chars[5])] = [int(chars[-1])]
                    else:
                        print("Unknown case for mono_synthetic: " + chars[2])
                else:
                    print("Unknown acceleration case: " + chars[1])




'''
Writing the data for 4a into the .csv output file
'''
def generate4a():
    with open('results.csv', mode='w') as f:
        # Headings
        newLine = ','
        # FFT
        newLine += 'FFT,'
        cnt = 1
        while cnt < fftSizeLength:
            newLine += ','
            cnt += 1
        # Sort
        newLine += 'Sort,'
        cnt = 1
        while cnt < sortSizeLength:
            newLine += ','
            cnt += 1
        # GEMM
        newLine += 'GEMM,'
        cnt = 1
        while cnt < gemmSizeLength:
            newLine += ','
            cnt += 1
        # Endline
        newLine += '\n'
        f.write(newLine)

        # Sizes
        newLine = ','
        # FFT
        for s in fftSize:
            newLine += s
            newLine += ','
        # Sort
        for s in sortSize:
            newLine += s
            newLine += ','
        # GEMM
        for s in gemmSize:
            newLine += s
            newLine += ','
        # Endline
        newLine += '\n'
        f.write(newLine)

        # Linux (OS)
        newLine = 'Linux (OS),'
        # FFT
        for i in range(fftSizeLength):
            newLine += str(100 * fftLinux[i][0] / fftBase[i][0])
            newLine += ','
        # Sort
        for i in range(sortSizeLength):
            newLine += str(100 * sortLinux[i][0] / sortBase[i][0])
            newLine += ','
        # GEMM
        for i in range(gemmSizeLength):
            newLine += str(100 * gemmLinux[i][0] / gemmBase[i][0])
            newLine += ','
        # Endline
        newLine += '\n'
        f.write(newLine)

        # Baremetal
        newLine = 'Baremetal,'
        # FFT
        for i in range(fftSizeLength):
            newLine += str(100 * fftBM[i][0] / fftBase[i][0])
            newLine += ','
        # Sort
        for i in range(sortSizeLength):
            newLine += str(100 * sortBM[i][0] / sortBase[i][0])
            newLine += ','
        # GEMM
        for i in range(gemmSizeLength):
            newLine += str(100 * gemmBM[i][0] / gemmBase[i][0])
            newLine += ','
        # Endline
        newLine += '\n'
        f.write(newLine)

        # ASI+MESI
        newLine = 'ASI+MESI,'
        # FFT
        for i in range(fftSizeLength):
            newLine += str(100 * fftASI_Total[i][0][0] / fftBase[i][0])
            newLine += ','
        # Sort
        for i in range(sortSizeLength):
            newLine += str(100 * sortASI_Total[i][0][0] / sortBase[i][0])
            newLine += ','
        # GEMM
        for i in range(gemmSizeLength):
            newLine += str(100 * gemmASI_Total[i][0][0] / gemmBase[i][0])
            newLine += ','
        # Endline
        newLine += '\n'
        f.write(newLine)

        f.write('\n')
        f.close()

'''
Writing the data for 4b into the .csv output file
'''
def generate4b():
    with open('results.csv', mode='a') as f:
        # Headings
        newLine = ','
        # FFT
        newLine += 'FFT, , , '
        for _ in range(1, fftSizeLength):
            newLine += ', , , '
        # Sort
        newLine += 'Sort, , , '
        for _ in range(1, sortSizeLength):
            newLine += ', , , '
        # GEMM
        newLine += 'GEMM, , , '
        for _ in range(1, gemmSizeLength):
            newLine += ', , , '
        # Endline
        newLine += '\n'
        f.write(newLine)

        # Sizes
        newLine = ','
        # FFT
        for s in fftSize:
            newLine += s
            newLine += ', , , '
        # Sort
        for s in sortSize:
            newLine += s
            newLine += ', , , '
        # GEMM
        for s in gemmSize:
            newLine += s
            newLine += ', , , '
        # Endline
        newLine += '\n'
        f.write(newLine)

        # Coherence
        # FFT + Sort + GEMM
        newLine = ','
        for i in range(fftSizeLength + sortSizeLength + gemmSizeLength):
            newLine += ', '.join(coherence)
            newLine += ', '
        # Endline
        newLine += '\n'
        f.write(newLine)

        # CPU Write
        newLine = 'CPU Write,'
        # FFT
        for i in range(fftSizeLength):
            # MESI
            newLine += str(100 * fftASI_CPUW[i][0][0] / fftBase[i][0])
            newLine += ','
            # DMA
            newLine += str(100 * fftASI_CPUW[i][1][0] / fftBase[i][0])
            newLine += ','
            # Spandex
            newLine += str(100 * fftASI_CPUW[i][2][0] / fftBase[i][0])
            newLine += ','
        # Sort
        for i in range(sortSizeLength):
            # MESI
            newLine += str(100 * sortASI_CPUW[i][0][0] / sortBase[i][0])
            newLine += ','
            # DMA
            newLine += str(100 * sortASI_CPUW[i][1][0] / sortBase[i][0])
            newLine += ','
            # Spandex
            newLine += str(100 * sortASI_CPUW[i][2][0] / sortBase[i][0])
            newLine += ','
        # GEMM
        for i in range(gemmSizeLength):
            # MESI
            newLine += str(100 * gemmASI_CPUW[i][0][0] / gemmBase[i][0])
            newLine += ','
            # DMA
            newLine += str(100 * gemmASI_CPUW[i][1][0] / gemmBase[i][0])
            newLine += ','
            # Spandex
            newLine += str(100 * gemmASI_CPUW[i][2][0] / gemmBase[i][0])
            newLine += ','
        # Endline
        newLine += '\n'
        f.write(newLine)

        # CPU Read
        newLine = 'CPU Read,'
        # FFT
        for i in range(fftSizeLength):
            # MESI
            newLine += str(100 * fftASI_CPUR[i][0][0] / fftBase[i][0])
            newLine += ','
            # DMA
            newLine += str(100 * fftASI_CPUR[i][1][0] / fftBase[i][0])
            newLine += ','
            # Spandex
            newLine += str(100 * fftASI_CPUR[i][2][0] / fftBase[i][0])
            newLine += ','
        # Sort
        for i in range(sortSizeLength):
            # MESI
            newLine += str(100 * sortASI_CPUR[i][0][0] / sortBase[i][0])
            newLine += ','
            # DMA
            newLine += str(100 * sortASI_CPUR[i][1][0] / sortBase[i][0])
            newLine += ','
            # Spandex
            newLine += str(100 * sortASI_CPUR[i][2][0] / sortBase[i][0])
            newLine += ','
        # GEMM
        for i in range(gemmSizeLength):
            # MESI
            newLine += str(100 * gemmASI_CPUR[i][0][0] / gemmBase[i][0])
            newLine += ','
            # DMA
            newLine += str(100 * gemmASI_CPUR[i][1][0] / gemmBase[i][0])
            newLine += ','
            # Spandex
            newLine += str(100 * gemmASI_CPUR[i][2][0] / gemmBase[i][0])
            newLine += ','
        # Endline
        newLine += '\n'
        f.write(newLine)

        # Accelerator execution
        newLine = 'Accelerator,'
        # FFT
        for i in range(fftSizeLength):
            # MESI
            newLine += str(100 * fftASI_Acc[i][0][0] / fftBase[i][0])
            newLine += ','
            # DMA
            newLine += str(100 * fftASI_Acc[i][1][0] / fftBase[i][0])
            newLine += ','
            # Spandex
            newLine += str(100 * fftASI_Acc[i][2][0] / fftBase[i][0])
            newLine += ','
        # Sort
        for i in range(sortSizeLength):
            # MESI
            newLine += str(100 * sortASI_Acc[i][0][0] / sortBase[i][0])
            newLine += ','
            # DMA
            newLine += str(100 * sortASI_Acc[i][1][0] / sortBase[i][0])
            newLine += ','
            # Spandex
            newLine += str(100 * sortASI_Acc[i][2][0] / sortBase[i][0])
            newLine += ','
        # GEMM
        for i in range(gemmSizeLength):
            # MESI
            newLine += str(100 * gemmASI_Acc[i][0][0] / gemmBase[i][0])
            newLine += ','
            # DMA
            newLine += str(100 * gemmASI_Acc[i][1][0] / gemmBase[i][0])
            newLine += ','
            # Spandex
            newLine += str(100 * gemmASI_Acc[i][2][0] / gemmBase[i][0])
            newLine += ','
        # Endline
        newLine += '\n'
        f.write(newLine)

        f.write('\n')
        f.close()



'''
Writing the data for 5b into the .csv output file
'''
def generate5b():
    with open('results.csv', mode='a') as f:
        # Workload
        newLine = 'Audio\n'
        f.write(newLine)

        # Chaining vs. Pipelining
        newLine = ', OS, '
        newLine += 'Accelerator Chaining, , , '
        newLine += 'Accelerator Pipelining, , , '
        newLine += '\n'
        f.write(newLine)

        # Coherence
        newLine = ', , '
        for _ in range(2):  # Chaining and Pipelining
            newLine += ', '.join(coherence)
            newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # Audio components
        for i in range(audioComponentLength):
            newLine = audioComponent[i]
            if i == 0:
                newLine += ' Filter, '
            elif i == 1:
                newLine += ' Order, '
            elif i == 2:
                newLine += ' Process, '
            elif i == 3:
                newLine += ' Filter, '
            else:
                print("Unknown component in audio: " + audioComponent[i])

            # OS
            newLine += str(100 * audioLinux[i][0] / audioBase[0])
            newLine += ', '
            # Chaining
            for j in range(coherenceLength):
                newLine += str(100 * audioASI_chain[i][j][0] / audioBase[0])
                newLine += ', '
            # Pipelining
            for j in range(coherenceLength):
                newLine += str(100 * audioASI_pipe[i][j][0] / audioBase[0])
                newLine += ', '
            newLine += '\n'
            f.write(newLine)

        f.write('\n')
        f.close()

'''
Writing the data for 5c into the .csv output file
'''
def generate5c():
    with open('results.csv', mode='a') as f:
        # Workload
        newLine = 'Mini-ERA\n'
        f.write(newLine)

        # Chaining
        newLine = ', OS , '
        newLine += 'Accelerator Chaining, , , '
        newLine += '\n'
        f.write(newLine)

        # Coherence
        newLine = ', , '
        newLine += ', '.join(coherence)
        newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # Mini-ERA components
        for i in range(eraComponentLength):
            newLine = eraComponent[i]
            newLine += ', '

            # OS
            newLine += str(100 * eraLinux[i][0] / eraBase[0])
            newLine += ', '
            # Chaining
            for j in range(coherenceLength):
                newLine += str(100 * eraASI[i][j][0] / eraBase[0])
                newLine += ', '
            newLine += '\n'
            f.write(newLine)

        f.write('\n')
        f.close()

'''
Writing the data for 5d into the .csv output file
'''
def generate5d():
    with open('results.csv', mode='a') as f:
        # Workload
        newLine = 'FCNN\n'
        f.write(newLine)

        # Chaining vs. Pipelining
        newLine = ', OS, '
        newLine += 'Accelerator Chaining, , , '
        newLine += 'Accelerator Pipelining, , , '
        newLine += '\n'
        f.write(newLine)

        # Coherence
        newLine = ', , '
        for _ in range(2):  # Chaining and Pipelining
            newLine += ', '.join(coherence)
            newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # FCNN
        newLine = 'FCNN,'
        # OS
        newLine += str(100 * fcnnLinux[0] / fcnnBase[0])
        newLine += ', '
        # Chaining
        for j in range(coherenceLength):
            newLine += str(100 * fcnnASI_chain[j][0] / fcnnBase[0])
            newLine += ', '
        # Pipelining
        for j in range(coherenceLength):
            newLine += str(100 * fcnnASI_pipe[j][0] / fcnnBase[0])
            newLine += ', '
        newLine += '\n'
        f.write(newLine)

        f.write('\n')
        f.close()


'''
Writing the data for 6 into the .csv output file
'''
def generate6():
    with open('results.csv', mode='a') as f:
        # Workload
        newLine = 'Audio Mono vs. Disag\n'
        f.write(newLine)

        # Chaining vs. Pipelining
        newLine = ', Linux, , '
        newLine += 'Accelerator Chaining, , , , , , '
        newLine += 'Accelerator Pipelining, , , , , ,'
        newLine += '\n'
        f.write(newLine)

        # Coherence
        newLine = ', , , '
        for _ in range(2):  # Chaining and Pipelining
            newLine += ', , '.join(coherence)
            newLine += ', , '
        newLine += '\n'
        f.write(newLine)

        # Mono vs. Disag
        newLine = ', '
        for _ in range(1 + 2 * coherenceLength):  # OS and Chaining and Pipelining
            newLine += 'Mono, Disag, '
        newLine += '\n'
        f.write(newLine)

        # Audio components
        for i in range(audioComponentLength):
            newLine = audioComponent[i]
            if i == 0:
                newLine += ' Filter, '
            elif i == 1:
                newLine += ' Order, '
            elif i == 2:
                newLine += ' Process, '
            elif i == 3:
                newLine += ' Filter, '
            else:
                print("Unknown component in audio: " + audioComponent[i])

            # OS
            newLine += str(100 * monoAudioLinux[i][0] / audioBase[0])
            newLine += ', '
            newLine += str(100 * audioLinux[i][0] / audioBase[0])
            newLine += ', '
            # Chaining
            for j in range(coherenceLength):
                newLine += str(100 * monoAudioASI_chain[i][j][0] / audioBase[0])
                newLine += ', '
                newLine += str(100 * audioASI_chain[i][j][0] / audioBase[0])
                newLine += ', '
            # Pipelining
            for j in range(coherenceLength):
                newLine += str(100 * monoAudioASI_pipe[i][j][0] / audioBase[0])
                newLine += ', '
                newLine += str(100 * audioASI_pipe[i][j][0] / audioBase[0])
                newLine += ', '
            newLine += '\n'
            f.write(newLine)

        f.write('\n')
        f.close()

'''
Writing the data for chaining of synth-small into the .csv output file
This figure is not included in the current accepted version, but we will include it in the final camera-ready version
'''
def generateSynSmallChain():
    with open('results.csv', mode='a') as f:
        # Workload
        newLine = 'Synth-small Chaining\n'
        f.write(newLine)

        # No. acc
        newLine = 'Number of Accelerators, '
        for i in range(1, 16, 1):
            newLine += str(i)
            newLine += ','
        newLine += '\n'
        f.write(newLine)

        # Baseline
        newLine = 'Base (Linux), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[0][0] / synLinux[0][synAcc.index(str(i))][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+MESI
        newLine = 'M+ (ASI+MESI), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[0][0] / synASI_chain[0][synAcc.index(str(i))][0][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+DMA
        newLine = 'D+ (ASI+DMA), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[0][0] / synASI_chain[0][synAcc.index(str(i))][1][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+Spandex
        newLine = 'Mozart (ASI+Spandex), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[0][0] / synASI_chain[0][synAcc.index(str(i))][2][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        f.write('\n')
        f.close()

'''
Writing the data for chaining of synth-large into the .csv output file
This figure is not included in the current accepted version, but we will include it in the final camera-ready version
'''
def generateSynLargeChain():
    with open('results.csv', mode='a') as f:
        # Workload
        newLine = 'Synth-large Chaining\n'
        f.write(newLine)

        # No. acc
        newLine = 'Number of Accelerators, '
        for i in range(1, 16, 1):
            newLine += str(i)
            newLine += ','
        newLine += '\n'
        f.write(newLine)

        # Baseline
        newLine = 'Base (Linux), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[1][0] / synLinux[1][synAcc.index(str(i))][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+MESI
        newLine = 'M+ (ASI+MESI), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[1][0] / synASI_chain[1][synAcc.index(str(i))][0][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+DMA
        newLine = 'D+ (ASI+DMA), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[1][0] / synASI_chain[1][synAcc.index(str(i))][1][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+Spandex
        newLine = 'Mozart (ASI+Spandex), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[1][0] / synASI_chain[1][synAcc.index(str(i))][2][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        f.write('\n')
        f.close()

'''
Writing the data for pipelining of synth-small into the .csv output file
This figure is not included in the current accepted version, but we will include it in the final camera-ready version
'''
def generateSynSmallPipe():
    with open('results.csv', mode='a') as f:
        # Workload
        newLine = 'Synth-small Pipelining\n'
        f.write(newLine)

        # No. acc
        newLine = 'Number of Accelerators, '
        for i in range(1, 16, 1):
            newLine += str(i)
            newLine += ','
        newLine += '\n'
        f.write(newLine)

        # ASI+MESI
        newLine = 'M+ (ASI+MESI), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[0][0] / synASI_pipe[0][synAcc.index(str(i))][0][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+DMA
        newLine = 'D+ (ASI+DMA), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[0][0] / synASI_pipe[0][synAcc.index(str(i))][1][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+Spandex
        newLine = 'Mozart (ASI+Spandex), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[0][0] / synASI_pipe[0][synAcc.index(str(i))][2][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        f.write('\n')
        f.close()

'''
Writing the data for pipelining of synth-large into the .csv output file
This figure is not included in the current accepted version, but we will include it in the final camera-ready version
'''
def generateSynLargePipe():
    with open('results.csv', mode='a') as f:
        # Workload
        newLine = 'Synth-large Pipelining\n'
        f.write(newLine)

        # No. acc
        newLine = 'Number of Accelerators, '
        for i in range(1, 16, 1):
            newLine += str(i)
            newLine += ','
        newLine += '\n'
        f.write(newLine)

        # ASI+MESI
        newLine = 'M+ (ASI+MESI), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[1][0] / synASI_pipe[1][synAcc.index(str(i))][0][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+DMA
        newLine = 'D+ (ASI+DMA), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[1][0] / synASI_pipe[1][synAcc.index(str(i))][1][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+Spandex
        newLine = 'Mozart (ASI+Spandex), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[1][0] / synASI_pipe[1][synAcc.index(str(i))][2][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        f.write('\n')
        f.close()

'''
Writing the data for mono vs. disag pipelining of synth-small into the .csv output file
This figure is not included in the current accepted version, but we will include it in the final camera-ready version
'''
def generateSynSmallPipeCompare():
    with open('results.csv', mode='a') as f:
        # Workload
        newLine = 'Synth-small Mono vs. Disag Pipelining\n'
        f.write(newLine)

        # No. acc
        newLine = ', Number of Accelerators, '
        for i in range(1, 16, 1):
            newLine += str(i)
            newLine += ','
        newLine += '\n'
        f.write(newLine)

        # Disag
        # ASI+MESI
        newLine = 'Disag, M+ (ASI+MESI), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[0][0] / synASI_pipe[0][synAcc.index(str(i))][0][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+DMA
        newLine = ', D+ (ASI+DMA), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[0][0] / synASI_pipe[0][synAcc.index(str(i))][1][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+Spandex
        newLine = ', Mozart (ASI+Spandex), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[0][0] / synASI_pipe[0][synAcc.index(str(i))][2][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # Mono
        # ASI+MESI
        newLine = 'Mono, M+ (ASI+MESI), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[0][0] / monoSynASI_pipe[0][synAcc.index(str(i))][0][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+DMA
        newLine = ', D+ (ASI+DMA), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[0][0] / monoSynASI_pipe[0][synAcc.index(str(i))][1][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+Spandex
        newLine = ', Mozart (ASI+Spandex), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[0][0] / monoSynASI_pipe[0][synAcc.index(str(i))][2][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        f.write('\n')
        f.close()

'''
Writing the data for mono vs. disag pipelining of synth-large into the .csv output file
This figure is not included in the current accepted version, but we will include it in the final camera-ready version
'''
def generateSynLargePipeCompare():
    with open('results.csv', mode='a') as f:
        # Workload
        newLine = 'Synth-large Mono vs. Disag Pipelining\n'
        f.write(newLine)

        # No. acc
        newLine = ', Number of Accelerators, '
        for i in range(1, 16, 1):
            newLine += str(i)
            newLine += ','
        newLine += '\n'
        f.write(newLine)

        # Disag
        # ASI+MESI
        newLine = 'Disag, M+ (ASI+MESI), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[1][0] / synASI_pipe[1][synAcc.index(str(i))][0][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+DMA
        newLine = ', D+ (ASI+DMA), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[1][0] / synASI_pipe[1][synAcc.index(str(i))][1][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+Spandex
        newLine = ', Mozart (ASI+Spandex), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[1][0] / synASI_pipe[1][synAcc.index(str(i))][2][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # Mono
        # ASI+MESI
        newLine = 'Mono, M+ (ASI+MESI), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[1][0] / monoSynASI_pipe[1][synAcc.index(str(i))][0][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+DMA
        newLine = ', D+ (ASI+DMA), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[1][0] / monoSynASI_pipe[1][synAcc.index(str(i))][1][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        # ASI+Spandex
        newLine = ', Mozart (ASI+Spandex), '
        for i in range(1, 16, 1):
            if str(i) in synAcc:
                newLine += str(synBase[1][0] / monoSynASI_pipe[1][synAcc.index(str(i))][2][0])
                newLine += ', '
            else:
                newLine += ', '
        newLine += '\n'
        f.write(newLine)

        f.write('\n')
        f.close()




if __name__ == '__main__':
    dataParsing(sys.argv[1])
    if os.path.isfile('./results.csv'):
        os.system('rm results.csv')
    generate4a()
    generate4b()
    generate5b()
    generate5c()
    generate5d()
    generate6()
    generateSynSmallChain()
    generateSynLargeChain()
    generateSynSmallPipe()
    generateSynLargePipe()
    generateSynSmallPipeCompare()
    generateSynLargePipeCompare()
