This is the final package for the Churchill Navigation Programming Challenge:

  \churchill    : The original challenge files together with the unmodified
                  churchill.dll, the source code for the test harness, the
                  reference and the fast solutions, as well as the FCIV
                  utility to compute the SHA-1 hashes.
  \contestants  : The submission of all the contestants, binaries and source
                  (where we have received it).
  \test_scripts : Here you'll find some scripts that we used for testing.

The random seed that we used for the final results is:
75BCD15-159A55E5-1F123BB5-5491333-34DAD465

Some notes on testing:
Since some of the submitted DLLs leak large amounts of memory, or hinder the
system in some other way, testing all of the solutions in a single run is not
practical, and would also be unfair. So we have decided to run each solution
individually, and then compare the results. Also, the query times for the top
contestants were so close, that we decided to run the same queries a hundred
times, and then compute the average query time, and use that for the final
ordering.

