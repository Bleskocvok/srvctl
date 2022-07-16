#!/bin/bash

echo '(f) starting'

for i in $(seq 10); do
    sleep 1
    echo "(f) $i"
done

echo '(f) exitting'
