#!/bin/bash

PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:~/bin
export PATH

[ $# -ne 2 ] && echo "usage: $0 [n_host] [n_player]" && exit 0

keys=()
mkfifo fifo_0.tmp
exec 3< fifo_0.tmp
for i in $(seq 1 $1); do
    mkfifo fifo_${i}.tmp
    key=$RANDOM
    keys+=($key)
    # echo "./host $i $key 0"
    ./host $i $key 0 &
    eval "exec $(($i+3))> fifo_${i}.tmp"
done

cyc=0
k=0
(
for a in $(seq 1 $2); do
for b in $(seq $(($a + 1)) $2); do
for c in $(seq $(($b + 1)) $2); do
for d in $(seq $(($c + 1)) $2); do
for e in $(seq $(($d + 1)) $2); do
for f in $(seq $(($e + 1)) $2); do
for g in $(seq $(($f + 1)) $2); do
for h in $(seq $(($g + 1)) $2); do
    echo "$a $b $c $d $e $f $g $h"
done;done;done;done;done;done;done;done
) | while IFS= read -r line
do
    if [[ k -lt $1 ]]; then
        let k++
        echo $line >> fifo_${cyc+1}.tmp
    fi
    let "cyc=(cyc+1)%$1"
    read key <&3
    # TODO: deal with fifo_0

done 

for i in $(seq 1 $1); do
    echo "-1 -1 -1 -1 -1 -1 -1 -1" >> fifo_${i}.tmp
done

# TODO: Print the final scores ordered by player id (ascending) to stdout.

rm *.tmp
exec 3<&-

# TODO: Wait for all forked process to exit.