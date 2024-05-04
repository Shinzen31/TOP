#!/bin/bash
mkdir -p ../results
mkdir -p ../speed_up

cd ../src

cmake .
make

i=16
perf record ./my_executable ../config.txt ../results/res_100_$i.txt
perf report > ../speed_up/perf_100_$i.txt

perf record ./my_executable ../config_500.txt ../results/res_500_$i.txt
perf report > ../speed_up/perf_500_$i.txt

perf record ./my_executable ../config_1000.txt ../results/res_1000_$i.txt
perf report > ../speed_up/perf_1000_$i.txt

cd ../scripts
./clean.sh
