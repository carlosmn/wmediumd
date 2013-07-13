set xlabel "Processes"
set ylabel "Received packets"
set yrange [0.3:1.1]
plot "benchmark-10" title '10ms' with lines, \
     "benchmark-20" title '20ms' with lines, \
     "benchmark-50" title '50ms' with lines
set terminal pdf color
set output "load-loss.pdf"
replot
