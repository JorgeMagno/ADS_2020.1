#!/bin/bash

#echo $(grep "/NodeList/1/" dumbbellTeste.tr) >> node1.txt
#echo $(grep -v "/NodeList/2/" resultados0.txt) >> node1.txt 

for ((i=11; i<311; i++))
do
numero=0
        for ((j=0; j<=9; j++))
        do
        
        numero1=$(grep -c "r $i.$j" ads1.tr)
        
        numero=$(($numero+$numero1))

        echo $numero >> /home/jorge/testes/ads/pesada/droptail/hurst/coleta41.txt
        
        done

done
exit 0

