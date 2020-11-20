rm *.tmp
mkfifo fifo_0.tmp fifo_1.tmp
exec 3<> fifo_1.tmp
./host 1 5555 0
