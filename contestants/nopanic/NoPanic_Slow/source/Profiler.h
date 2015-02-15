struct Profiler
{
	Profiler();

	static Profiler* get();
	
	void print();

	int Version; // Defaults to 1 : Tree. Other option 2: Tree2
	
	int Queries;
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

