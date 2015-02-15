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
    interesting points is much less 256, we can tune the code performance.

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
    EA7EBE39-E0F6D069-EA7EBE39-F02596BF-C1B651E9, (1667.100 ms, 1569.939 ms), ( 15.267 ms, 1085.643 ms), ratio: 71.1
    F5A4882F-FF2CE67F-F5A4882F-EFFFA0A9-DE6C67FF, (1514.357 ms, 1560.718 ms), ( 25.575 ms, 1241.591 ms), ratio: 48.5
    CCFCC3F5-C674ADA5-CCFCC3F5-D6A7EB73-E7342C25, (1249.795 ms, 1577.132 ms), ( 25.104 ms, 1174.616 ms), ratio: 46.8
    D77339F8-DDFB57A8-D77339F8-CD28117E-FCBBD628, (1217.510 ms, 1561.338 ms), ( 17.620 ms, 1275.597 ms), ratio: 72.4
    A38606A3-A90E68F3-A38606A3-B9DD2E25-884EE973, (1373.786 ms, 1553.646 ms), ( 25.923 ms, 1649.405 ms), ratio: 63.6
    B8104829-B2982679-B8104829-A24B60AF-93D8A7F9, (1283.576 ms, 1551.653 ms), ( 15.514 ms, 3900.465 ms), ratio: 251.4
    D379A851-D9F1C601-D379A851-C92280D7-F8B14781, (1592.653 ms, 1559.638 ms), ( 22.750 ms, 1386.167 ms), ratio: 60.9
    FE2C9A9F-F4A4F4CF-FE2C9A9F-E477B219-D5E4754F, (1204.258 ms, 1579.788 ms), ( 16.325 ms, 6358.144 ms), ratio: 389.5
    ED97765D-E71F180D-ED97765D-F7CC5EDB-C65F998D, (1549.692 ms, 1560.596 ms), ( 25.455 ms, 1182.916 ms), ratio: 46.5
    1F4BEFA3-15C381F3-1F4BEFA3-510C725-34830073,  (1609.812 ms, 1567.766 ms), ( 14.379 ms, 1246.180 ms), ratio: 86.7
    11B7B452-1B3FDA02-11B7B452-BEC9CD4-3A7F5B82,  (1427.757 ms, 1553.804 ms), ( 26.143 ms, 1563.569 ms), ratio: 59.8
    96C305BC-9C4B6BEC-96C305BC-8C982D3A-BD0BEA6C, (1552.029 ms, 1575.678 ms), ( 26.172 ms, 1195.070 ms), ratio: 45.7
    3ED48C8-9652698-3ED48C8-19B6604E-2825A718,    (1571.686 ms, 1554.339 ms), ( 25.304 ms, 975.261 ms), ratio: 38.5
    338A4962-39022732-338A4962-29D161E4-1842A6B2, (1366.632 ms, 1549.246 ms), ( 31.855 ms, 2573.450 ms), ratio: 80.8
    4DB8F7CB-4730999B-4DB8F7CB-57E3DF4D-6670181B, (1188.626 ms, 1563.834 ms), ( 20.220 ms, 3204.746 ms), ratio: 158.5
    7CABBAD5-7623D485-7CABBAD5-66F09253-57635505, (1599.564 ms, 1560.047 ms), ( 24.073 ms, 1405.854 ms), ratio: 58.4
    69733F0A-63FB515A-69733F0A-7328178C-42BBD0DA, (1571.419 ms, 1554.439 ms), ( 15.745 ms, 2547.273 ms), ratio: 161.8
    9965FAC6-93ED9496-9965FAC6-833ED240-B2AD1516, (1581.802 ms, 1549.538 ms), ( 19.877 ms, 971.711 ms), ratio: 48.9
    BFF71A9A-B57F74CA-BFF71A9A-A5AC321C-943FF54A, (1533.526 ms, 1556.584 ms), ( 24.199 ms, 1157.191 ms), ratio: 47.8
    B195699E-BB1D07CE-B195699E-ABCE4118-9A5D864E, (1328.243 ms, 1553.317 ms), ( 23.769 ms, 3435.952 ms), ratio: 144.6
    --------------------------------------------------------------------------------------------------------------------
                                                                 total_query = 441.269 ms, 39530.801 ms, ratio: 89.6
                                                                               xcps.dll  , reference.dll

    (1) The average query time is about 80-90 times faster 
    (2) The media time is about 60 times faster.
    (3) The performance is based on my old 6-year PC machine. [i7 core 920 @2.67GHz, first generation of i7 core]
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

