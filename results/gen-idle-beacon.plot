set title "Traffic and load from beacons"
set xlabel "Virtual Machines"
set ylabel "Kbps"
set y2label "%CPU"

#set grid
set xtics 0,8,32
set ytics nomirror
set y2tics 0,2,13
plot "idle-beacons" using 1:2 axes x1y1 with lines title 'Traffic' lc rgb 'red', \
     "idle-beacons" using 1:3 axes x1y2 with lines title 'CPU' lc rgb 'blue' \

set terminal pdf color
set output "idle-beacons.pdf"
replot
