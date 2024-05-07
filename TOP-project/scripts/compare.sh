#!/bin/bash

for i in 2 4 8 16 24 48
do
python3 ../scripts/compare.py ../results/res_100_1.txt ../results/res_100_$i.txt > ../speed_up/scal_100_new_$i.txt
python3 ../scripts/compare.py ../results/res_500_1.txt ../results/res_500_$i.txt > ../speed_up/scal_500_new_$i.txt
python3 ../scripts/compare.py ../results/res_1000_1.txt ../results/res_1000_$i.txt > ../speed_up/scal_1000_new_$i.txt
done