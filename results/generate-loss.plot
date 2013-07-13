set xlabel "Configured loss"
set ylabel "Packet loss"
set yrange [-0.1:1.1]
plot "benchmark-loss" using 2:(1-$2) title 'target' with lines, \
     "benchmark-loss" using 2:3 title 'actual' with lines
set terminal pdf color
set output "loss-accuracy.pdf"
replot
