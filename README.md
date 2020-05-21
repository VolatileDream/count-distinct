count-distinct
==============

Small command line utility taking advantage of HyperLogLog to count distinct lines.

HyperLogLog: https://en.wikipedia.org/wiki/HyperLogLog

Murmur Hash 3 used in current implementation:  
http://en.wikipedia.org/wiki/MurmurHash#MurmurHash3

Usage
=====

Use `tup` to compile, `count-distinct` to see usage information.

Provides both temporary and persistent filters.
