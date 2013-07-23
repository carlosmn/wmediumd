set title "Stable throughput"
set xlabel "Virtual Machines"
set ylabel "Mbps"

#set grid
set xtics 0,8,32
#set ytics nomirror
plot "stable-throughput" using 1:2 with lines title 'Throughput', \
     "stable-throughput" using 1:3 with lines title 'Attempt at 0.2', \
     "stable-throughput" using 1:4 with lines title 'Actual at 0.2'


set terminal pdf color
set output "stable-throughput.pdf"
replot
