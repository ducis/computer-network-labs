awk '$1=="r" && $7==3 && $17==2 && $9=="cbr" {b += $11} END {print b}' task1_out.nam
