#!/bin/sh
                 
#ASI SPX                                                                   
echo ""
echo "Synthetic CFA-large: Chaining SPX"
echo ""
set -- 1 3 5 7 10 12 13 14 15
counter=0
while [ "$counter" -lt 9 ]; do
: $((counter+=1))
./tiled_app_stratus_cfa_SPX.exe $1  1024 1024 $((1500/$1)) 1024 1;
shift                        
done     
                              
#Pipelining SPX  
echo ""
echo "Synthetic CFA-large: Pipelining SPX"
echo ""
set -- 1 3 5 7 10 12 13 14 15                                             
counter=0
while [ "$counter" -lt 9 ]; do
: $((counter+=1))
./tiled_app_stratus_cfa_SPX.exe $1  1024 1024 $((1500/$1)) 1024 2;
shift                        
done     
