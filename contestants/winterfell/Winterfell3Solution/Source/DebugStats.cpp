#include "DebugStats.h"

#ifdef _ENABLE_STATS_LOGGING

#include <iostream>
#include <ctime>
#include <functional>

static const int kVersion = 2;

StatsAllQueries::StatsAllQueries()
{
	mSessions.reserve(1000);
	std::srand((int) std::time(0));
}

void StatsAllQueries::CommitCurrent()
{
	mSessions.push_back(mCurrent);
	mCurrent = StatsSession();
}

void StatsAllQueries::ExportLog()
{
	if (mSessions.size() <= 1)
	{
		return;
	}

	const int kBufSize = 256;
	char buffer[kBufSize];

	int hash = std::rand();
	_snprintf(buffer, kBufSize, "Stats_v%i_(q%i)_%i.txt", kVersion, mSessions.size(), hash);

	FILE* file = fopen(buffer, "w");
	if (nullptr == file)
	{
		return;
	}

	{
		int count = _snprintf(buffer, kBufSize, "KDTreeCount: %i\n", mSessions[0].mKDTreeCount);
		int toWrite = (count < kBufSize) ? count : kBufSize;
		fwrite(buffer, 1, toWrite, file);
	}

	{
		int count = _snprintf(buffer, kBufSize, "KDTreeMaxDepth: %i\n", mSessions[0].mKDTreeMaxDepth);
		int toWrite = (count < kBufSize) ? count : kBufSize;
		fwrite(buffer, 1, toWrite, file);
	}

	typedef std::function< int (const StatsSession& inSesh) > GetStatsValuePred;
	typedef std::function< float (const StatsSession& inSesh) > GetStatsHitRatePred;

	auto writeMinMaxAvg = [&] (const char* inValueName, const GetStatsValuePred& inGetValue)
	{
		int min = INT_MAX, max = 0, total = 0;
		int sessionCount = 0;
		for (auto sesh : mSessions)
		{
			int val = inGetValue(sesh);
			if (val == 0)
			{
				continue;
			}
			total += val;
			min = std::min(min, val);
			max = std::max(max, val);

			++sessionCount;
		}

		float avg = (float) total / (float) sessionCount;

		int count = _snprintf(buffer, kBufSize, "%s: min=%i | max=%i | avg=%.3f (sessions = %i)\n", inValueName, min, max, avg, sessionCount);
		int toWrite = (count < kBufSize) ? count : kBufSize;
		fwrite(buffer, 1, toWrite, file);
	};

	auto writeMinMaxAvgHitRates = [&] (const char* inValueName, const GetStatsValuePred& inGetHitValue, const GetStatsValuePred& inGetTestCount)
	{
		float min = FLT_MAX, max = 0;
		int sessionCount = 0;
		float hitRateSum = 0;
		for (auto sesh : mSessions)
		{
			int hits		= inGetHitValue(sesh);
			int testCount	= inGetTestCount(sesh);

			if (testCount == 0)
			{
				continue;
			}

			float hitRate = (float) hits / (float) testCount;
			hitRate *= 100.0f;

			hitRateSum += hitRate;
			min = std::min(min, hitRate);
			max = std::max(max, hitRate);

			++sessionCount;
		}

		float avg = hitRateSum / ((float) sessionCount);

		int count = _snprintf(buffer, kBufSize, "%s: min=%.4f%% | max=%.4f%% | avg=%.4f%% (sessions = %i)\n", inValueName, min, max, avg, sessionCount);
		int toWrite = (count < kBufSize) ? count : kBufSize;
		fwrite(buffer, 1, toWrite, file);
	};

	auto writeHitRates = [&] (const char* inValueName, const GetStatsValuePred& inGetHitValue, const GetStatsValuePred& inGetMissValue)
	{
		char nameBuffer[256];

		auto testCountGetter = [&] (const StatsSession& inSesh)
		{
			return inGetHitValue(inSesh) + inGetMissValue(inSesh);
		};

		_snprintf(nameBuffer, 256, "%sHitRate", inValueName);
		writeMinMaxAvgHitRates(nameBuffer, inGetHitValue, testCountGetter);

		_snprintf(nameBuffer, 256, "%sTestCount", inValueName);
		writeMinMaxAvg(nameBuffer, testCountGetter);
	};

	writeMinMaxAvg("IterCount",			[] (const StatsSession& inSesh) { return inSesh.mIteratorCount;			});
	writeMinMaxAvg("IterMaxDepth",		[] (const StatsSession& inSesh) { return inSesh.mIteratorMaxDepth;		});
	writeMinMaxAvg("IterAccessCount",	[] (const StatsSession& inSesh) { return inSesh.mIteratorAccessCount;	});

	writeHitRates
	(	"CullCheckPoints", 
		[] (const StatsSession& inSesh) { return inSesh.mCullPointsCheckHit;	},
		[] (const StatsSession& inSesh) { return inSesh.mCullPointsCheckMiss;	}
	);
	/*
	writeHitRates
	(	"CullClusters",
		[] (const StatsSession& inSesh) { return inSesh.mCullClusterHit;	},
		[] (const StatsSession& inSesh) { return inSesh.mCullClusterMiss;	}
	);
	
	writeHitRates
	(	"RectInsideRect",
		[] (const StatsSession& inSesh) { return inSesh.mRectFullyInsideRectHit;	},
		[] (const StatsSession& inSesh) { return inSesh.mRectFullyInsideRectMiss;	}
	);
	
	writeHitRates
	(	"ClusterRectInsideRect",
		[] (const StatsSession& inSesh) { return inSesh.mClusterRectFullyInsideRectHit;		},
		[] (const StatsSession& inSesh) { return inSesh.mClusterRectFullyInsideRectMiss;	}
	);
	*/
	writeHitRates
	(	"Point",
		[] (const StatsSession& inSesh) { return inSesh.mPointInRectHit;	},
		[] (const StatsSession& inSesh) { return inSesh.mPointInRectMiss;	}
	);

	fclose(file);
}

#endif
