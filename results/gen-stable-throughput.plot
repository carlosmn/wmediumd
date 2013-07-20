set title "Stable throughput"
set xlabel "Virtual Machines"
set ylabel "Mbps"

#set grid
set xtics 0,8,32
#set ytics nomirror
plot "stable-throughput" using 1:2 with lines title 'Throughput'

set terminal pdf color
set output "stable-throughput.pdf"
replot
