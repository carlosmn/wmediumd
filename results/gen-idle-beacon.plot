set title "Traffic from beacons"
set xlabel "Virtual Machines"
set ylabel "Kbps"
#set yrange [0.3:1.1]
unset label
plot "idle-beacons" with lines title '', \
     "idle-beacons" title ''

set terminal pdf color
set output "idle-beacons.pdf"
replot
