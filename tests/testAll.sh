#! /bin/bash

echo "Test largest log rotation policy started"
./testLargest.sh

echo "Test largest log rotation policy with X variable started"
./testLargestVar.sh

echo "Test oldest log rotation policy started"
./testOldest.sh

echo "Test oldest log rotation policy with X variable started"
./testOldestVar.sh
