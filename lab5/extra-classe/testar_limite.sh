#!/bin/bash
for i in {1..3}; do
    curl -s http://127.0.0.1:8080/ > /dev/null &
done
wait
