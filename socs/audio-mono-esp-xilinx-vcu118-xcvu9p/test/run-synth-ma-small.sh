#!/bin/sh

set -- 1 3 5 7 10 12 13 14 15
counter=0

#SW Runs
echo ""
echo "Synthetic MA-small: SW"
echo ""
./tiled_app_stratus_ma_MESI.exe 1  1024 1024 150 1024 5;
                             
#OS
echo ""
echo "Synthetic MA-small: OS"
echo ""
set -- 1 3 5 7 10 12 13 14 15
counter=0
while [ "$counter" -lt 9 ]; do                                    
: $((counter+=1))
./tiled_app_stratus_ma_MESI.exe $1  1024 1024 $((150/$1)) 1024 0;
shift 
done          
                             
#ASI MESI
echo ""
echo "Synthetic MA-small: Chaining MESI"
echo ""
set -- 1 3 5 7 10 12 13 14 15
counter=0
while [ "$counter" -lt 9 ]; do                                    
: $((counter+=1))
./tiled_app_stratus_ma_MESI.exe $1  1024 1024 $((150/$1)) 1024 1;
shift 
done                          
                 
#ASI DMA                                                                   
echo ""
echo "Synthetic MA-small: Chaining DMA"
echo ""
set -- 1 3 5 7 10 12 13 14 15
counter=0
while [ "$counter" -lt 9 ]; do
: $((counter+=1))
./tiled_app_stratus_ma_DMA.exe $1  1024 1024 $((150/$1)) 1024 1;
shift                        
done     
                              
#Pipelining MESI 
echo ""
echo "Synthetic MA-small: Pipelining MESI"
echo ""
set -- 1 3 5 7 10 12 13 14 15                                              
counter=0
while [ "$counter" -lt 9 ]; do
: $((counter+=1))
./tiled_app_stratus_ma_MESI.exe $1  1024 1024 $((150/$1)) 1024 2;
shift                        
done     
                              
#Pipelining DMA  
echo ""
echo "Synthetic MA-small: Pipelining DMA"
echo ""
set -- 1 3 5 7 10 12 13 14 15                                             
counter=0
while [ "$counter" -lt 9 ]; do
: $((counter+=1))
./tiled_app_stratus_ma_DMA.exe $1  1024 1024 $((150/$1)) 1024 2;
shift                        
done     
