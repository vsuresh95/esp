#!/bin/sh

set -- 1 3 5 7 10 12 13 14 15
counter=0

#SW Runs
echo ""
echo "Synthetic CFA-large: SW"
echo ""
./tiled_app_stratus_cfa_MESI.exe 1  1024 1024 1500 1024 5;
                           
#OS
echo ""
echo "Synthetic CFA-large: OS"
echo ""
set -- 1 3 5 7 10 12 13 14 15
counter=0
while [ "$counter" -lt 9 ]; do                                    
: $((counter+=1))
./tiled_app_stratus_cfa_MESI.exe $1  1024 1024 $((1500/$1)) 1024 0;
shift 
done  
                             
#ASI MESI
echo ""
echo "Synthetic CFA-large: Chaining MESI"
echo ""
set -- 1 3 5 7 10 12 13 14 15
counter=0
while [ "$counter" -lt 9 ]; do                                    
: $((counter+=1))
./tiled_app_stratus_cfa_MESI.exe $1  1024 1024 $((1500/$1)) 1024 1;
shift 
done                          
                 
#ASI DMA                                                                   
echo ""
echo "Synthetic CFA-large: Chaining DMA"
echo ""
set -- 1 3 5 7 10 12 13 14 15
counter=0
while [ "$counter" -lt 9 ]; do
: $((counter+=1))
./tiled_app_stratus_cfa_DMA.exe $1  1024 1024 $((1500/$1)) 1024 1;
shift                        
done     
                              
#Pipelining MESI 
echo ""
echo "Synthetic CFA-large: Pipelining MESI"
echo ""
set -- 1 3 5 7 10 12 13 14 15                                              
counter=0
while [ "$counter" -lt 9 ]; do
: $((counter+=1))
./tiled_app_stratus_cfa_MESI.exe $1  1024 1024 $((1500/$1)) 1024 2;
shift                        
done     
                              
#Pipelining DMA  
echo ""
echo "Synthetic CFA-large: Pipelining DMA"
echo ""
set -- 1 3 5 7 10 12 13 14 15                                             
counter=0
while [ "$counter" -lt 9 ]; do
: $((counter+=1))
./tiled_app_stratus_cfa_DMA.exe $1  1024 1024 $((1500/$1)) 1024 2;
shift                        
done     
