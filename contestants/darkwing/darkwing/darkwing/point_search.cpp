#include "stdafx.h"

//#include "fastSherlock.h"

#include "point_search.h"
#include <iterator>
#include <list>
#include <algorithm>

// no class, no instances, thats too slow...
//
// create()
// First, read all the points into an unsorted list and get min / max coordintes and ranks.
// A layer is holding a range of ranks. (10 mio ranks, 1k layers -> 10k ranks / layer)
// Get the points frequency and create areas, that could hold one point per layer (1k layers -> 1k average points)
// [This leads us to the lucky situation that, in averge, we find the n most important points in an area with checking n layers...]
// For each layer in each area, create a vec.
// For each area, create an array holding the pointers to the layer vec.
// Create an array holding pointers to the area arrays.
// Now, putting the points to the lists: for each point in the big list...
// Find the area by dividing x/y position by the area size, cast to int and we have the indexes of the area.
// Find the layer by dividing rank by number of layers, cast to int and we have the layer.
//
// search()
// Translate rectangle to area indexes.
// Now, we have two kinds of areas: all points inside rectangle, or only some points.
// When processing "parted" areas, we have to check, if the points are inside the rectangle, otherwise not.
// Searching means now: 
// Go through all highest rank layers of all areas inside the rectangle and add the points to a list.
// Then, sort the list by rank and count the points. 
// If there are more points than we need, return top n and we are done.
// Otherwise, add the next layer of all areas inside the rectangle until we are done...

namespace pointSearch
{
	struct SearchContext
	{
		SearchContext(const Point* points_begin, const Point* points_end);
		int32_t search(const Rect rect, const int32_t count, Point* out_points);
	};


	// compare rank of 2 points
	bool compare_rank(const Point& first, const Point& second)
	{
		return (first.rank < second.rank);
	}

	//
	SearchContext::SearchContext(const Point* points_begin, const Point* points_end)
	{
		m_pointsToFind = 20;
		// count points
		m_pointCount = points_end - points_begin;
		if(m_pointCount < 1)
			return;
		m_points = new Point[m_pointCount];
		
		// get first
		m_currentPoint = points_begin;

		// init min max
		m_xMin = m_currentPoint->x;
		m_xMax = m_currentPoint->x;
		m_yMin = m_currentPoint->y;
		m_yMax = m_currentPoint->y;
		m_rankMin = m_currentPoint->rank;
		m_rankMax = m_currentPoint->rank;
		m_points[0] = *m_currentPoint;

		for(m_i = 1; m_i < (m_pointCount); m_i++)
		{
			m_currentPoint++;

			// store min max
			if(m_currentPoint->rank > m_rankMax)
				m_rankMax = m_currentPoint->rank;
			else if(m_currentPoint->rank < m_rankMin)
				m_rankMin = m_currentPoint->rank;

			if ((m_currentPoint->x < 10000000000) && (m_currentPoint->x > -10000000000))
			{
				if(m_currentPoint->x > m_xMax)
					m_xMax = m_currentPoint->x;
				else if(m_currentPoint->x < m_xMin)
					m_xMin = m_currentPoint->x; 
			}

			if ((m_currentPoint->y < 10000000000) && (m_currentPoint->y > -10000000000))
			{
				if(m_currentPoint->y > m_yMax)
					m_yMax = m_currentPoint->y;
				else if(m_currentPoint->y < m_yMin)
					m_yMin = m_currentPoint->y; 
			}
			
			// store point
			m_points[m_i] = *m_currentPoint;
		}

		// get frequency
		m_frequency = m_pointCount / ((m_xMax - m_xMin) * (m_yMax - m_yMin));

		// define layer
		m_rankLayerCount = 1000;

		// get area size
		m_sqare = sqrt(m_rankLayerCount * m_pointsToFind / m_frequency);
		if(m_sqare == 0)
			m_sqare = 1;
		m_xAreas = (int)((m_xMax - m_xMin) / m_sqare) + 1;
		m_yAreas = (int)((m_yMax - m_yMin) / m_sqare) + 1;

		// create array holding pointers
		m_AreaLayer = new vector<Point>***[m_xAreas];
		for(m_i = 0; m_i < m_xAreas; m_i++)
		{
			m_AreaLayer[m_i] = new vector<Point>**[m_yAreas];
			for(m_j = 0; m_j < m_yAreas; m_j++)
			{
				m_AreaLayer[m_i][m_j] = new vector<Point>*[m_rankLayerCount + 1];
				for(m_k = 0; m_k < m_rankLayerCount + 1; m_k++)
				{
					m_AreaLayer[m_i][m_j][m_k] = new vector<Point>;
				}
			}
		}

		// insert points
		for(m_i = 0; m_i < m_pointCount; m_i++)
		{
			if (m_points[m_i].x <= m_xMax)
			{
				if (m_points[m_i].x >= m_xMin)
				{
					m_x = (int)((m_points[m_i].x - m_xMin) / m_sqare);  
				}
				else
				{
					m_x = 0;
				}
			}
			else
			{
				m_x = m_xAreas - 1;
			}
			if (m_points[m_i].y <= m_yMax)
			{
				if (m_points[m_i].y >= m_yMin)
				{
					m_y = (int)((m_points[m_i].y - m_yMin) / m_sqare);  
				}
				else
				{
					m_y = 0;
				}
			}
			else
			{
				m_y = m_yAreas - 1;
			}
			m_r = (int)((m_points[m_i].rank - m_rankMin) / ((m_rankMax - m_rankMin) / (float)m_rankLayerCount));
			m_AreaLayer[m_x][m_y][m_r]->push_back(m_points[m_i]);
		}
		m_i = 0;
	}

	int32_t
	SearchContext::search(const Rect rect, const int32_t count, Point* out_points)
	{
		m_outPointsCount = 0;
		if((m_xAreas == 0) || (m_yAreas == 0))
			return 0;

		// left
		m_firstAreaX = (int)((rect.lx - m_xMin) / m_sqare);
		if(m_firstAreaX >= m_xAreas)
			m_firstAreaX = m_xAreas - 1;
		if(m_firstAreaX < 0)
			m_firstAreaX = 0;
		
		// right
		m_lastAreaX = (int)((rect.hx - m_xMin) / m_sqare);
		if(m_lastAreaX >= m_xAreas)
			m_lastAreaX = m_xAreas - 1;
		if(m_lastAreaX < 1)
			m_lastAreaX = 0;

		// bottom
		m_firstAreaY = (int)((rect.ly - m_yMin) / m_sqare);
		if(m_firstAreaY >= m_yAreas)
			m_firstAreaY = m_yAreas - 1;
		if(m_firstAreaY < 0)
			m_firstAreaY = 0;
		
		// top
		m_lastAreaY = (int)((rect.hy - m_yMin) / m_sqare);
		if(m_lastAreaY >= m_yAreas)
			m_lastAreaY = m_yAreas - 1;
		if(m_lastAreaY < 1)
			m_lastAreaY = 0;
		m_rankLayer = 0;

		while ((m_outPointsCount < 20) && (m_rankLayer < m_rankLayerCount))
		{
			m_pointsInRect.clear();
			// areas complete inside rect
			if((m_lastAreaX - m_firstAreaX) > 1)
			{
				if((m_lastAreaY - m_firstAreaY) > 1)
				{
					for(m_x = m_firstAreaX + 1; m_x < m_lastAreaX; m_x++)
					{
						for(m_y = m_firstAreaY + 1; m_y < m_lastAreaY; m_y++)
						{
							m_pointsInRect.insert(m_pointsInRect.end(), m_AreaLayer[m_x][m_y][m_rankLayer]->begin(), 
								m_AreaLayer[m_x][m_y][m_rankLayer]->end());
						}
					}
				}
			}

			// left and bottom parted, cant be checked for top and right
			m_leftParted = (((rect.lx - m_xMin) / m_sqare) > m_firstAreaX);
			m_bottomParted = (((rect.ly - m_yMin) / m_sqare) > m_firstAreaY);

			if (m_firstAreaX != m_lastAreaX)
			{
				// areas on the left without corners
				if (m_leftParted)
				{
					for(m_y = m_firstAreaY + 1; m_y < m_lastAreaY; m_y++)
					{
						for (int m_i = 0; m_i < m_AreaLayer[m_firstAreaX][m_y][m_rankLayer]->size(); m_i++)
						{
							if (m_AreaLayer[m_firstAreaX][m_y][m_rankLayer]->at(m_i).x >= rect.lx)
							{
								m_pointsInRect.push_back(m_AreaLayer[m_firstAreaX][m_y][m_rankLayer]->at(m_i));
							} 
						}
					} 
				}
				else
				{
					for(m_y = m_firstAreaY + 1; m_y < m_lastAreaY; m_y++)
					{
						m_pointsInRect.insert(m_pointsInRect.end(), m_AreaLayer[m_firstAreaX][m_y][m_rankLayer]->begin(), 
							m_AreaLayer[m_firstAreaX][m_y][m_rankLayer]->end());
					} 
				}

				// parted areas on the right without corners
				for(m_y = m_firstAreaY + 1; m_y < m_lastAreaY; m_y++)
				{
					for (int m_i = 0; m_i < m_AreaLayer[m_lastAreaX][m_y][m_rankLayer]->size(); m_i++)
					{
						if (m_AreaLayer[m_lastAreaX][m_y][m_rankLayer]->at(m_i).x <= rect.hx)
						{
							m_pointsInRect.push_back(m_AreaLayer[m_lastAreaX][m_y][m_rankLayer]->at(m_i));
						} 
					}		
				}  
			}
			else
			{
				// parted areas without corners
				for(m_y = m_firstAreaY + 1; m_y < m_lastAreaY; m_y++)
				{
					for (int m_i = 0; m_i < m_AreaLayer[m_firstAreaX][m_y][m_rankLayer]->size(); m_i++)
					{
						if ((m_AreaLayer[m_firstAreaX][m_y][m_rankLayer]->at(m_i).x <= rect.hx) &&
							(m_AreaLayer[m_firstAreaX][m_y][m_rankLayer]->at(m_i).x >= rect.lx))
						{
							m_pointsInRect.push_back(m_AreaLayer[m_firstAreaX][m_y][m_rankLayer]->at(m_i));
						} 
					}		
				}  
			}

			if (m_firstAreaY != m_lastAreaY)
			{
				// areas on the bottom without corners
				if(m_bottomParted)
				{
					for(m_x = m_firstAreaX + 1; m_x < m_lastAreaX; m_x++)
					{
						for (int m_i = 0; m_i < m_AreaLayer[m_x][m_firstAreaY][m_rankLayer]->size(); m_i++)
						{
							if (m_AreaLayer[m_x][m_firstAreaY][m_rankLayer]->at(m_i).y >= rect.ly)
							{
								m_pointsInRect.push_back(m_AreaLayer[m_x][m_firstAreaY][m_rankLayer]->at(m_i));
							} 
						}			
					}
				}
				else
				{
					for(m_x = m_firstAreaX + 1; m_x < m_lastAreaX; m_x++)
					{
						m_pointsInRect.insert(m_pointsInRect.end(), m_AreaLayer[m_x][m_firstAreaY][m_rankLayer]->begin(), 
							m_AreaLayer[m_x][m_firstAreaY][m_rankLayer]->end());
					}
				}

				// parted areas on the top without corners
				for(m_x = m_firstAreaX + 1; m_x < m_lastAreaX; m_x++)
				{
					for (int m_i = 0; m_i < m_AreaLayer[m_x][m_lastAreaY][m_rankLayer]->size(); m_i++)
					{
						if (m_AreaLayer[m_x][m_lastAreaY][m_rankLayer]->at(m_i).y <= rect.hy)
						{
							m_pointsInRect.push_back(m_AreaLayer[m_x][m_lastAreaY][m_rankLayer]->at(m_i));
						} 
					}		
				} 
			}
			else
			{
				// parted areas without corners
				for(m_x = m_firstAreaX + 1; m_x < m_lastAreaX; m_x++)
				{
					for (int m_i = 0; m_i < m_AreaLayer[m_x][m_firstAreaY][m_rankLayer]->size(); m_i++)
					{
						if ((m_AreaLayer[m_x][m_firstAreaY][m_rankLayer]->at(m_i).y <= rect.hy) &&
							(m_AreaLayer[m_x][m_firstAreaY][m_rankLayer]->at(m_i).y >= rect.ly))
						{
							m_pointsInRect.push_back(m_AreaLayer[m_x][m_firstAreaY][m_rankLayer]->at(m_i));
						} 
					}		
				} 
			}

			if(m_firstAreaX == m_lastAreaX)
			{
				if (m_firstAreaY == m_lastAreaY)
				{
					// just 1 corner area
					for (m_i = 0; m_i < m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->size(); m_i++)
					{

						if ((m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i).y <= rect.hy) &&
							(m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i).y >= rect.ly) &&
							(m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i).x >= rect.lx) &&
							(m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i).x <= rect.hx))
						{
							m_pointsInRect.push_back(m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i));
						} 
					}  
				}
				else
				{
					// 1 top and 1 bottom corner area
					// top
					for (m_i = 0; m_i < m_AreaLayer[m_firstAreaX][m_lastAreaY][m_rankLayer]->size(); m_i++)
					{
						if ((m_AreaLayer[m_firstAreaX][m_lastAreaY][m_rankLayer]->at(m_i).y <= rect.hy) &&
							(m_AreaLayer[m_firstAreaX][m_lastAreaY][m_rankLayer]->at(m_i).x >= rect.lx) &&
							(m_AreaLayer[m_firstAreaX][m_lastAreaY][m_rankLayer]->at(m_i).x <= rect.hx))
						{
							m_pointsInRect.push_back(m_AreaLayer[m_firstAreaX][m_lastAreaY][m_rankLayer]->at(m_i));
						} 
					}
					// bottom
					for (m_i = 0; m_i < m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->size(); m_i++)
					{
						if ((m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i).y >= rect.ly) &&
							(m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i).x >= rect.lx) &&
							(m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i).x <= rect.hx))
						{
							m_pointsInRect.push_back(m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i));
						} 
					}
				}
			}
			else
			{
				if (m_firstAreaY == m_lastAreaY)
				{
					// 1 left and 1 right corner area
					// left
					for (m_i = 0; m_i < m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->size(); m_i++)
					{
						if ((m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i).y <= rect.hy) &&
							(m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i).y >= rect.ly) &&
							(m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i).x >= rect.lx))
						{
							m_pointsInRect.push_back(m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i));
						} 
					}
					// rigth
					for (m_i = 0; m_i < m_AreaLayer[m_lastAreaX][m_firstAreaY][m_rankLayer]->size(); m_i++)
					{
						if ((m_AreaLayer[m_lastAreaX][m_firstAreaY][m_rankLayer]->at(m_i).y <= rect.hy) &&
							(m_AreaLayer[m_lastAreaX][m_firstAreaY][m_rankLayer]->at(m_i).y >= rect.ly) &&
							(m_AreaLayer[m_lastAreaX][m_firstAreaY][m_rankLayer]->at(m_i).x <= rect.hx))
						{
							m_pointsInRect.push_back(m_AreaLayer[m_lastAreaX][m_firstAreaY][m_rankLayer]->at(m_i));
						} 
					}
				}
				else
				{
					// upper left corner
					for (m_i = 0; m_i < m_AreaLayer[m_firstAreaX][m_lastAreaY][m_rankLayer]->size(); m_i++)
					{
						if ((m_AreaLayer[m_firstAreaX][m_lastAreaY][m_rankLayer]->at(m_i).y <= rect.hy) &&
							(m_AreaLayer[m_firstAreaX][m_lastAreaY][m_rankLayer]->at(m_i).x >= rect.lx))
						{
							m_pointsInRect.push_back(m_AreaLayer[m_firstAreaX][m_lastAreaY][m_rankLayer]->at(m_i));
						} 
					} 

					// upper right corner
					for (m_i = 0; m_i < m_AreaLayer[m_lastAreaX][m_lastAreaY][m_rankLayer]->size(); m_i++)
					{
						if ((m_AreaLayer[m_lastAreaX][m_lastAreaY][m_rankLayer]->at(m_i).y <= rect.hy) &&
							(m_AreaLayer[m_lastAreaX][m_lastAreaY][m_rankLayer]->at(m_i).x <= rect.hx))
						{
							m_pointsInRect.push_back(m_AreaLayer[m_lastAreaX][m_lastAreaY][m_rankLayer]->at(m_i));
						} 
					}

					// lower left corner
					for (m_i = 0; m_i < m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->size(); m_i++)
					{
						if ((m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i).y >= rect.ly) &&
							(m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i).x >= rect.lx))
						{
							m_pointsInRect.push_back(m_AreaLayer[m_firstAreaX][m_firstAreaY][m_rankLayer]->at(m_i));
						} 
					}

					// lower right corner
					for (m_i = 0; m_i < m_AreaLayer[m_lastAreaX][m_firstAreaY][m_rankLayer]->size(); m_i++)
					{
						if ((m_AreaLayer[m_lastAreaX][m_firstAreaY][m_rankLayer]->at(m_i).y >= rect.ly) &&
							(m_AreaLayer[m_lastAreaX][m_firstAreaY][m_rankLayer]->at(m_i).x <= rect.hx))
						{
							m_pointsInRect.push_back(m_AreaLayer[m_lastAreaX][m_firstAreaY][m_rankLayer]->at(m_i));
						} 
					}
				}
			}
			

			std::sort(m_pointsInRect.begin(), m_pointsInRect.end(), compare_rank);

			for (std::vector<Point>::iterator it = m_pointsInRect.begin();it != m_pointsInRect.end(); ++it)
			{
				out_points[m_outPointsCount] = *it;
				m_outPointsCount++;
				if(m_outPointsCount > 19)
					break;
			}
			m_rankLayer++;
		}
	
		return m_outPointsCount;
	}

	extern "C" __declspec(dllexport)
	SearchContext* __stdcall
	create(const Point* points_begin, const Point* points_end)
	{
		return new SearchContext(points_begin, points_end);
	}

	extern "C" __declspec(dllexport)
	int32_t __stdcall
	search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
	{
		return sc->search(rect, count, out_points);
	}

	extern "C" __declspec(dllexport)
	SearchContext* __stdcall
	destroy(SearchContext* sc)
	{
		delete sc;
		return nullptr;
	}
}
