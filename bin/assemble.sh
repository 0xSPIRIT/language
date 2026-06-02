#!/bin/bash
gcc -m32 -no-pie prog.s -o prog # Automatically links with CRT
