Hall_2.dll

Drew Hall
andrew.danger.hall@gmail.com
(626) 298-1746

All work is my own (other than the point_search.h header file
provided by Churchill) and is hereby released to the public domain.

Hall_2.dll is a multithreaded, binned quadtree approach that beats
my initial Hall_1.dll by about 60% (and the reference.dll implementation
by 10-15x) on my laptop.  It uses 4 query threads each with their own
QuadTree containing 1/4 of the points, then merges the results using 2
merge threads.  The main thread coordinates these actions using
concurrent queues to send tasking.

The QuadTrees themselves are heavily optimized for cache performance
and to minimize branching instructions.  I think this is about as
far as a quad-tree based solution can go.  My next submission will
likely switch to an R-tree algorithm instead.

Built with Visual C++ 2013 (project files included).

Cheers & thanks for a fun contest,
Drew