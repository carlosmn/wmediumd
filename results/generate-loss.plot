set xlabel "Configured loss"
set ylabel "Packet loss"
set yrange [-0.1:1.1]
plot "benchmark-loss" using 2:(1-$2) title 'soll' with lines, \
     "benchmark-loss" using 2:3 title 'ist' with lines
set terminal postscript color
set output "results-loss.ps"
replot
