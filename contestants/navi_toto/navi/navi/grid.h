#pragma once

#include "utils.h"

class Grid
{
private:
	class Sector
	{
	public:
		Sector(	Point* b = 0, Point* e = 0, float l = 0.0f, float h = 0.0f )
		: begin(b)
		, end(e)
		, lo(l)
		, hi(h)
		{}

		Point* begin;
		Point* end;
		float lo;
		float hi;

		bool operator < (const Sector& ref) { return hi < ref.lo; }
	};

public:
	Grid( const Grid& );
	Grid& operator= ( const Grid& );

	std::vector<Point> m_points;
	std::vector<Sector> m_sectors;

public:
	Grid()
    {
    }
	~Grid()
    {
    }

	bool create( const Point* it_begin, const Point* it_end, unsigned int cellsize = 256 )
	{
        if( it_begin == it_end )
            return false;

		//copy and sort points
		{
            Rect zone;
            zone.lx = -100000.0f;
            zone.ly = -100000.0f;
            zone.hx = +100000.0f;
            zone.hy = +100000.0f;

			m_points.reserve(it_end-it_begin);

			const Point* point = it_begin;
			while( point != it_end )
			{
                if( rect_intersect( zone, *point) )
                {
    				m_points.push_back(*point);
                }

				point++;
			}

			std::sort( m_points.begin(), m_points.end(), compare_y );
		}

		int snumb = sqrt( double(m_points.size()/cellsize) );
		snumb = snumb > 1 ? snumb : 1;

		int ssize = m_points.size() / snumb;
		ssize = ssize > 1 ? ssize : 1;

		for( int index = 0; index < m_points.size(); index++ )
		{
			Point& point = m_points[index];

			if( m_sectors.empty() || (m_sectors.back().end-m_sectors.back().begin >= ssize) && m_points[index].y != m_points[index-1].y )
			{
				
				m_sectors.push_back( Sector(&point, &point, point.y, point.y) );
			}

			Sector& sector = m_sectors.back();

			sector.end = &point+1;
			sector.hi = point.y;
		}
		
		for( int index = 0; index < m_sectors.size(); index++ )
		{
			std::sort( m_sectors[index].begin, m_sectors[index].end, compare_x );
		}

		return true;
	}

	int32_t search(const Rect rect, const int32_t capacity, Point* results)
	{
		static std::vector<Point> list;
		list.reserve(capacity);
		list.clear();

		Point lp, hp;
		lp.x = rect.lx;
		lp.y = rect.ly;
		hp.x = rect.hx;
		hp.y = rect.hy;

		std::vector<Sector>::iterator sector = std::lower_bound( m_sectors.begin(), m_sectors.end(), Sector(0,0,rect.ly,0) );

		while( sector != m_sectors.end() && sector->lo <= rect.hy )
		{
			Point* point = std::lower_bound( sector->begin, sector->end, lp, compare_x );
			while( point != sector->end && point->x <= rect.hx )
			{
				if( point->y >= rect.ly && point->y <= rect.hy )
					list_insort(list,*point);
					//list.push_back(*point);

				point++;
			}

			sector++;
		}

		//sort data in list
		//std::sort( list.begin(), list.end(), compare_rank );

		int32_t size = (list.size() < capacity) ? list.size() : capacity;

		for( int32_t index = 0; index < size; index++ )
		{
			results[index] = list[index];
		}

		return size;
	}
};
