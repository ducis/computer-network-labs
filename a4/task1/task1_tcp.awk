awk '$1== "r" && $7==3 && $17==1 && $9=="tcp" {a += $11} END {print a}' task1_out.nam
