Hall_3.dll

Drew Hall
andrew.danger.hall@gmail.com
(626) 298-1746

All work is my own (other than the point_search.h header file
provided by Churchill) and is hereby released to the public domain.

Hall_3.dll is a single threaded, R-Tree approach using STR bulk loading.
It beats my previous Hall_2.dll by about 10x (and the reference.dll implementation
by 50-100x) on my laptop.

The RTree is heavily optimized for cache performance and to minimize
branching instructions.  I will likely submit a final multithreaded adaptation of
this if time permits.

Built with Visual C++ 2013 (project files included).

Cheers & thanks for a fun contest,
Drew