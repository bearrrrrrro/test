#!/bin/bash

PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin:~/bin
export PATH

[ $# -ne 2 ] && echo "usage: $0 [n_host] [n_player]" && exit 0

keys=()
mkfifo fifo_0.tmp
exec 3<> fifo_0.tmp
for i in $(seq 1 $1); do
    mkfifo fifo_${i}.tmp
    key=$RANDOM
    keys[$i]=$key
    # echo "./host $i $key 0"
    ./host $i $key 0 &
    eval "exec $(($i+3))<> fifo_${i}.tmp"
done

score=()
for i in $(seq 1 $2); do
    score[$i]=0
done

k=0
combstr=`
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
`

while IFS= read comb
do
    if [[ k -lt $1 ]]; then
        let k++
        echo "$comb > fifo_${k}.tmp"
        echo $comb > fifo_${k}.tmp
    fi
    read key <&3
    u=1
    until [ "${keys[u]}"==key ]; do
        let u++
    done
    for i in $(seq 1 8); do
        read player_id rank <&3
        score[$player_id]=$((${score[$player_id]}+8-$rank))
    done
    echo ${comb} > fifo_${u}.tmp
    echo "${comb} > fifo_${u}.tmp"
done <<< $combstr

for i in $(seq 1 $1); do
    echo "-1 -1 -1 -1 -1 -1 -1 -1" > fifo_${i}.tmp
done

# Print the final scores ordered by player id (ascending) to stdout.

for i in $(seq 1 $2); do
    echo "$i ${score[$i]}"
done

exec 3<&-
rm *.tmp

# Wait for all forked process to exit.
# echo "waiting ..."
wait
# echo "Done!"