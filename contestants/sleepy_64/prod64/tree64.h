#include "geom.h"
#include <array>
#include <vector>

namespace search2d{
using std::array;
using std::vector;
using namespace geom2d;
typedef uint32_t PtIndex;
typedef uint32_t NodeIndex;
typedef uint32_t PtIndex;

struct Range
{
    PtIndex b, e;
    PtIndex size()const { return e - b; }
};

struct FoundPoint
{
    geom2d::Point p;
    int32_t rank;
    int8_t payload;
    bool operator ==(const FoundPoint& oth)const
    {
        return p == oth.p && rank == oth.rank && payload == oth.payload;
    }
    bool operator !=(const FoundPoint& oth)const
    {
        return !(*this == oth);
    }
};

const size_t kMaxNodeChildren = 64;
const uint32_t kMaxNodePoints = 4096;

struct Node
{
    Rectangle bbox;
    Rectangle ownBbox;
    array<Rectangle, kMaxNodeChildren> bboxes;
    Range own;
    array<NodeIndex, kMaxNodeChildren> children;
};

struct SearchContext
{
    Rectangle global_bbox;
    vector<Node> nodes;
    vector<geom2d::Point> points;
    vector<int32_t> ranks;
    vector<int8_t> payload;
};
size_t search(const SearchContext& c, const Rectangle& r, const uint32_t maxOutput, vector<FoundPoint>& outPoints);
SearchContext* create_context(vector<FoundPoint>& points); // may reorder or destroy input
void destroy_context(SearchContext*);
}