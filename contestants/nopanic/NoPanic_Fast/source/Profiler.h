struct Profiler
{
	Profiler();

	static Profiler* get();
	
	void print();

	int Version; // Defaults to 1 : Tree; 2: Tree2 3: CFS

	int Queries;
	
	// Versions 1,2
	int NodesVisited;
	int OverlapFailed;
	
	// Version 1
	int EmptyQueries;
	int FullQueries;
	int FastQueries;
	int SlowQueries;
	int PointsChecked;
	int PointsInside;
	int PointsBypassed;
	int MaxQueue;
	int LongQueries;
	int EmptyNodes;
	int EmptyNodeTests;
	
	// Version 2
	int PointsOutputted;
	int PointsFound;
	int PointsUsed;
	int TotalTests;
	int EmptyTests;
	int SkippedTests;
	//  SlowQueries
	int SlowQueries2;
	int TestReturned;
	int OneReturned;
	int ResultsOverwritten;
	// EmptyNodes
	// EmptyNodeTests
	long long BytesCopied;

	// Version 3
	// PointsChecked
	// PointsInside
	int L1;
	int L2;
	int L3;
	int L4;

	// Temporary helper variables
	int tmp0;
	int tmp1;
	int tmp2;
};


#ifdef USE_PROFILER
#  define PROFILE(cmd) {Profiler *ctx = Profiler::get(); cmd;}
#else
#  define PROFILE(cmd)
#endif

