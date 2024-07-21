#!/bin/sh
                              
#Pipelining SPX  
echo ""
echo "Synthetic MA-small: Pipelining SPX"
echo ""
set -- 1 3 5 7 10 12 13 14 15                                             
counter=0
while [ "$counter" -lt 9 ]; do
: $((counter+=1))
./tiled_app_stratus_ma_SPX.exe $1  2048 512 $((150/$1)) 512 2;
shift                        
done     
