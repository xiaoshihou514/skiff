#! /bin/bash
gcc -lgit2 main.c -o skiff
mkdir -p ~/.local/bin
mv skiff ~/.local/bin/
echo "done"
