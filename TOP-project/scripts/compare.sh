#!/bin/bash

i=48
python3 ../scripts/compare.py ../reference/ref_100.txt ../results/res_100_$i.txt > ../speed_up/scal_100_$i.txt
python3 ../scripts/compare.py ../reference/ref_500.txt ../results/res_500_$i.txt > ../speed_up/scal_500_$i.txt
python3 ../scripts/compare.py ../reference/ref_1000.txt ../results/res_1000_$i.txt > ../speed_up/scal_1000_$i.txt

