#!/bin/bash
mkdir -p ../results
mkdir -p ../speed_up

cd ../src

cmake .
make

<<<<<<< HEAD
perf record ./my_executable ../config.txt ../results/res_100_24.txt
perf report > ../speed_up/perf_100_24.txt

perf record ./my_executable ../config_500.txt ../results/res_500_24.txt
perf report > ../speed_up/perf_500_24.txt

perf record ./my_executable ../config_1000.txt ../results/res_1000_24.txt
=======
perf record mpirun -np 24 ./my_executable ../config.txt ../results/res_100_24.txt
perf report > ../speed_up/perf_100_24.txt

perf record mpirun -np 24 ./my_executable ../config_500.txt ../results/res_500_24.txt
perf report > ../speed_up/perf_500_24.txt

perf record mpirun -np 24 ./my_executable ../config_1000.txt ../results/res_1000_24.txt
>>>>>>> c77133c941f26f0e248e23fd9ed8fcbece9c7f35
perf report > ../speed_up/perf_1000_24.txt

cd ../scripts
./clean.sh
