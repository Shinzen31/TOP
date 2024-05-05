set terminal png
set key left


set output 'ss_100.png'
set title 'Strong Scalability 100*100*100'

set xlabel 'number of threads'
set ylabel 'average speed-up'

set logscale x 2
f(x) = x * 7.3148

plot 'scal_100.dat' with points title 'real speed-up', \
     f(x) with lines title 'theoretic speed-up'



set output 'ss_500.png'
set title 'Strong Scalability 500*500*500'

set xlabel 'number of threads'
set ylabel 'average speed-up'

set logscale x 2
f(x) = x * 1.7042

plot 'scal_500.dat' with points title 'real speed-up', \
     f(x) with lines title 'theoretic speed-up'



set output 'ss_1000.png'
set title 'Strong Scalability 1000*1000*1000'

set xlabel 'number of threads'
set ylabel 'average speed-up'

set logscale x 2
f(x) = x * 1.7018

plot 'scal_1000.dat' with points title 'real speed-up', \
     f(x) with lines title 'theoretic speed-up'