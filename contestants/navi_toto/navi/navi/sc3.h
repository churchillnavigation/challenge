#pragma once

#include "utils.h"
#include "grid.h"

class SearchContext
{
private:
	class Level
	{
	public:
		std::vector<Point> points;
	};

private:
	SearchContext( const SearchContext& );
	SearchContext& operator= ( const SearchContext& );

	Grid grid;

	std::vector<Level> m_mipx;
	std::vector<Level> m_mipy;
	unsigned int m_size;
	Rect m_bound;

public:
	SearchContext()
    {
    }
	~SearchContext()
    {
    }

	bool create( const Point* it_begin, const Point* it_end )
	{
        if( it_begin == it_end )
            return false;

		if( !grid.create(it_begin, it_end) )
			return false;

		std::vector<Point> points;

		//copy and sort points
		{
            Rect zone;
            zone.lx = -100000.0f;
            zone.ly = -100000.0f;
            zone.hx = +100000.0f;
            zone.hy = +100000.0f;

            m_size = it_end-it_begin;
			points.reserve(m_size);
			rect_set(m_bound,*it_begin);

			const Point* point = it_begin;
			while( point != it_end )
			{
                if( rect_intersect( zone, *point) )
                {
    				points.push_back(*point);
	    			rect_expand(m_bound,*point);
                }

				point++;
			}

			std::sort( points.begin(), points.end(), compare_rank );
		}

		m_mipx.resize(16);
		m_mipy.resize(16);
		for( int mip = 0; mip < m_mipx.size(); mip++ )
		{
			int index1 = points.size() >> (mip+0);
			int index0 = points.size() >> (mip+1);

			if( mip+1 == m_mipx.size() )
				index0 = 0;

			m_mipx[mip].points.resize( index1-index0 );
			m_mipy[mip].points.resize( index1-index0 );

			for( int index = 0; index < m_mipx[mip].points.size(); index++ )
			{
				m_mipx[mip].points[index] = points[index0+index];
				m_mipy[mip].points[index] = points[index0+index];
			}

			std::sort( m_mipx[mip].points.begin(), m_mipx[mip].points.end(), compare_x );
			std::sort( m_mipy[mip].points.begin(), m_mipy[mip].points.end(), compare_y );
		}

		return true;
	}

	int32_t search(const Rect rect, const int32_t capacity, Point* results)
	{
		float w = rect.hx-rect.lx;
		float h = rect.hy-rect.ly;
		float s = w*h;

		float bw = m_bound.hx-m_bound.lx;
		float bh = m_bound.hy-m_bound.ly;
		float bs = bw*bh;

		float nw = w / bw;
		float nh = h / bh;
		float ns = s / bs;

		int32_t mip = m_mipx.size()-1;

		static std::vector<Point> list;
		list.clear();

		while( mip >= 0 )
		{
			Point lpoint;
			lpoint.x = rect.lx;
			lpoint.y = rect.ly;

			if( nw <= nh )
			{
				if( (m_size>>mip)*nw > m_size * ns )
				{
					return grid.search(rect, capacity, results);
				}

				Level& level = m_mipx[mip];
				std::vector<Point>::iterator it = std::lower_bound( level.points.begin(), level.points.end(), lpoint, compare_x );
				while( it != level.points.end() && it->x <= rect.hx )
				{
					if( it->x >= rect.lx && it->y >= rect.ly && it->y <= rect.hy )
					{
						list.push_back(*it);
					}

					it++;
				}
			}
			else
			{
				if( (m_size>>mip)*nh > m_size * ns )
				{
					return grid.search(rect, capacity, results);
				}

				Level& level = m_mipy[mip];
				std::vector<Point>::iterator it = std::lower_bound( level.points.begin(), level.points.end(), lpoint, compare_y );
				while( it != level.points.end() && it->y <= rect.hy )
				{
					if( it->y >= rect.ly && it->x >= rect.lx && it->x <= rect.hx )
					{
						list.push_back(*it);
					}

					it++;
				}
			}


			if( list.size() >= capacity )
				break;

			mip--;
		}

		//sort data in list
		std::sort( list.begin(), list.end(), compare_rank );

		int32_t size = (list.size() < capacity) ? list.size() : capacity;

		for( int32_t index = 0; index < size; index++ )
		{
			results[index] = list[index];
		}

		return size;
	}
};


