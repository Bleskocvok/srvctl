#!/bin/bash

echo '(loop) starting'

for i in $(seq 10); do
    sleep 1
    echo '.'
done

echo '(loop) exitting'
