#!/bin/sh
                              
#Pipelining MESI 
echo ""
echo "Synthetic MA-large: Pipelining MESI"
echo ""
set -- 1 3 5 7 10 12 13 14 15                                              
counter=0
while [ "$counter" -lt 9 ]; do
: $((counter+=1))
./tiled_app_stratus_ma_MESI.exe $1  1024 1024 $((1500/$1)) 1024 2;
shift                        
done     
                              
#Pipelining DMA  
echo ""
echo "Synthetic MA-large: Pipelining DMA"
echo ""
set -- 1 3 5 7 10 12 13 14 15                                             
counter=0
while [ "$counter" -lt 9 ]; do
: $((counter+=1))
./tiled_app_stratus_ma_DMA.exe $1  1024 1024 $((1500/$1)) 1024 2;
shift                        
done     
