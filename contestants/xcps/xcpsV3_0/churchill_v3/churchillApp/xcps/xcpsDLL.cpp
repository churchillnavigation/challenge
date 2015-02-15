#include "stdafx.h"
#include <iostream>
#include "xcpsDLL.h"
#include "xcError.h"
#include "psMan.h"

/*
 Notes:
 1. The design idea is to divide the big x/y spaces into cells. During the load time, sort the cells first by rank.
    - During the search time, the overlap between the cells and search rectangles contains all interesting points;
    - For the inner cells,  we don't need compare and just merge them over. Also it implements a recursive way to 
        collect the best points.
    - The margin cells, we need compare the points and just merge them conditionally.

 2. I used the Visual Stuio Express 2013 (free download from Microsoft) to compile the code. I don't have profile tools
    but I manually profile a few points for optimization purpose.

 3. The optimization is based on the points of 10 million data sets, 20 interesting points. The code works for any 
    interesting points less than 256. If it is greater than 256, the code still works but the performance is degraded. 
    However, the code can be tuned for large interesting points (the code is not hooked it into yet.). If the 
    interesting points is much less 256, we can tune the code performance better.

    A few more optimizations could be done with AVX instructions. 

 4. The data is organized this way:
    x       - all x coordinate of the input points is stored in one array (pXBase);
    y       - all y coordinate of the input points is stored in one array (pYBase);
    id      - all id of the input points is stored in one array (pIdBase);
    rank    - all rank of the input points is stored with an index of the data (indexRank_t structure)
                (1) The index is used to reference x/y/id.
                (2) The combined data is in 64-bit integer, so sorting rank is equivalent to sort 64 bit integers.

 5. Memory: based on the pointer_search.exe measurement, for 10 million data, it used about 286 MB memory.

 6. Performance: for 10 million data set, 20 interesting points, choose 20 random seeds. query 1000 times.
                                                    load time   load time       query time query time
                                                    xcps.dll    reference.dll   xcps.dll   reference.dll
    EA7EBE39-E0F6D069-EA7EBE39-F02596BF-C1B651E9, (2707.856 ms, 1537.717 ms), (  9.202 ms, 1909.543 ms), ratio: 207.5
    F5A4882F-FF2CE67F-F5A4882F-EFFFA0A9-DE6C67FF, (2710.302 ms, 1538.071 ms), (  9.073 ms, 1292.982 ms), ratio: 142.5
    CCFCC3F5-C674ADA5-CCFCC3F5-D6A7EB73-E7342C25, (2418.065 ms, 1540.317 ms), (  8.998 ms, 1581.999 ms), ratio: 175.8
    D77339F8-DDFB57A8-D77339F8-CD28117E-FCBBD628, (2188.663 ms, 1543.948 ms), (  7.611 ms, 881.075 ms), ratio: 115.8
    A38606A3-A90E68F3-A38606A3-B9DD2E25-884EE973, (2583.678 ms, 1534.305 ms), (  8.929 ms, 917.340 ms), ratio: 102.7
    B8104829-B2982679-B8104829-A24B60AF-93D8A7F9, (2292.474 ms, 1548.413 ms), ( 10.139 ms, 3483.785 ms), ratio: 343.6
    D379A851-D9F1C601-D379A851-C92280D7-F8B14781, (2855.015 ms, 1539.096 ms), (  8.749 ms, 1470.366 ms), ratio: 168.1
    FE2C9A9F-F4A4F4CF-FE2C9A9F-E477B219-D5E4754F, (2277.834 ms, 1543.367 ms), ( 10.823 ms, 4499.692 ms), ratio: 415.8
    ED97765D-E71F180D-ED97765D-F7CC5EDB-C65F998D, (2893.458 ms, 1524.621 ms), (  8.792 ms, 1315.591 ms), ratio: 149.6
    1F4BEFA3-15C381F3-1F4BEFA3-510C725-34830073, (2841.305 ms, 1535.299 ms), (  9.291 ms, 1582.584 ms), ratio: 170.3
    11B7B452-1B3FDA02-11B7B452-BEC9CD4-3A7F5B82, (2658.153 ms, 1552.648 ms), (  8.878 ms, 1612.025 ms), ratio: 181.6
    96C305BC-9C4B6BEC-96C305BC-8C982D3A-BD0BEA6C, (2490.377 ms, 1536.393 ms), (  9.577 ms, 1330.797 ms), ratio: 139.0
    3ED48C8-9652698-3ED48C8-19B6604E-2825A718, (2936.036 ms, 1538.329 ms), (  8.923 ms, 1456.826 ms), ratio: 163.3
    338A4962-39022732-338A4962-29D161E4-1842A6B2, (2287.138 ms, 1542.711 ms), (  9.866 ms, 2544.677 ms), ratio: 257.9
    4DB8F7CB-4730999B-4DB8F7CB-57E3DF4D-6670181B, (2294.592 ms, 1530.809 ms), (  9.545 ms, 3233.500 ms), ratio: 338.8
    7CABBAD5-7623D485-7CABBAD5-66F09253-57635505, (2927.997 ms, 1542.725 ms), (  9.053 ms, 1874.804 ms), ratio: 207.1
    69733F0A-63FB515A-69733F0A-7328178C-42BBD0DA, (2612.668 ms, 1533.013 ms), (  8.805 ms, 2420.151 ms), ratio: 274.9
    9965FAC6-93ED9496-9965FAC6-833ED240-B2AD1516, (2810.042 ms, 1539.725 ms), (  9.060 ms, 1444.008 ms), ratio: 159.4
    BFF71A9A-B57F74CA-BFF71A9A-A5AC321C-943FF54A, (2847.556 ms, 1540.868 ms), (  8.366 ms, 1051.351 ms), ratio: 125.7
    B195699E-BB1D07CE-B195699E-ABCE4118-9A5D864E, (2300.710 ms, 1546.077 ms), (  9.182 ms, 3289.241 ms), ratio: 358.2
    --------------------------------------------------------------------------------------------------
                                                    total_query = 182.862 ms, 39192.337 ms, ratio: 214.3
                                                                               xcps.dll  , reference.dll

    (1) The average query time is about 214 times faster 
    (2) The performance is based on my old 6-year PC machine. [i7 core 920 @2.67GHz, first generation of i7 core]
*/

//----------------------------------------------------------------------------------
// interface create function
DLLEXPORT SearchContext* __stdcall create(const Point* points_begin, const Point* points_end) {
    try {
        int32_t dataCount = (int32_t)(points_end - points_begin);
        XC::verify(dataCount < 20000000, "support searching up to 20 millions points");

        SearchContext* sc = new SearchContext(dataCount, points_begin);

        XC::verify(!!sc, "no enough memory to create the search object.");

        return sc;
    }
    catch (XC::errorExcept_t& ex) {
        std::cout << "Error: ********** " << ex.what() << std::endl;
    }
    catch (...) {
        std::cout << "Error: ********** unknown (create)" << std::endl;
    }
    return (SearchContext*)0;
}

//-------------------------------------------------------------------------------------------------
// interface search function
DLLEXPORT int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points) {
    try {
        return (sc->*(sc->find))(rect, count, out_points);
    }
    catch (XC::errorExcept_t& ex) {
        std::cout << "Error: ********** " << ex.what() << std::endl;
    }
    catch (...) {
        std::cout << "Error: ********** unknown (search) " << std::endl;
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------
// interface destroy function
DLLEXPORT SearchContext* __stdcall destroy(SearchContext* sc) {
    try {
        delete sc;
    }
    catch (XC::errorExcept_t& ex) {
        std::cout << "Error: *********** " << ex.what() << std::endl;
    }
    catch (...) {
        std::cout << "Error: *********** unknown (destroy)" << std::endl;
    }
    return 0;
}

