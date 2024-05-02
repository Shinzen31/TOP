#!/bin/bash
mkdir -p ../results
mkdir -p ../speed_up

cd ../src

cmake .
make

./my_executable ../config.txt ../results/res_100.txt
./my_executable ../config_500.txt ../results/res_500.txt
./my_executable ../config_1000.txt ../results/res_1000.txt

python3 ../scripts/compare.py ../reference/ref_100.txt ../results/res_100.txt >> ../speed_up/perf_100.txt
python3 ../scripts/compare.py ../reference/ref_500.txt ../results/res_500.txt >> ../speed_up/perf_500.txt
python3 ../scripts/compare.py ../reference/ref_1000.txt ../results/res_1000.txt >> ../speed_up/perf_1000.txt

cd ../scripts
./clean.sh