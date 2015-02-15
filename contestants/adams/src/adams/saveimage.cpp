#include "stdafx.h"
#include <iostream>
#include <memory>
#include <vector>
#include <assert.h>
#include <fstream>

using namespace std;

struct Image {
	int w;
	int h;
	unsigned char* red;
	unsigned char* green;
	unsigned char* blue;

	Image() {
		w = 5000;
		h = 5000;
		red = new unsigned char[w*h]();
		green = new unsigned char[w*h]();
		blue = new unsigned char[w*h]();
	}

	void SaveImage() {
		char* img = new char[3*w*h];
		int filesize = 54 + 3*w*h;

		for(int i=0; i<w; i++)
		{
			for(int j=0; j<h; j++)
			{
				int r = red[i+w*j];
				int g = green[i+w*j];
				int b = blue[i+w*j];
				r = min(r, 255);
				g = min(g, 255);
				b = min(b, 255);
				img[(i+j*w)*3+2] = r;
				img[(i+j*w)*3+1] = g;
				img[(i+j*w)*3+0] = b;
			}
		}

		char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
		char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
		char bmppad[3] = {};

		bmpfileheader[ 2] = (unsigned char)(filesize    );
		bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
		bmpfileheader[ 4] = (unsigned char)(filesize>>16);
		bmpfileheader[ 5] = (unsigned char)(filesize>>24);

		bmpinfoheader[ 4] = (unsigned char)(       w    );
		bmpinfoheader[ 5] = (unsigned char)(       w>> 8);
		bmpinfoheader[ 6] = (unsigned char)(       w>>16);
		bmpinfoheader[ 7] = (unsigned char)(       w>>24);
		bmpinfoheader[ 8] = (unsigned char)(       h    );
		bmpinfoheader[ 9] = (unsigned char)(       h>> 8);
		bmpinfoheader[10] = (unsigned char)(       h>>16);
		bmpinfoheader[11] = (unsigned char)(       h>>24);

		ofstream f;
		f.open("img.bmp", ios::out | ios::trunc | ios::binary);
		f.write(bmpfileheader, 14);
		f.write(bmpinfoheader, 40);
		for(int i=0; i<h; i++)
		{
			f.write(img+(w*(h-i-1)*3),3*w);
			f.write(bmppad,(4-(w*3)%4)%4);
		}
		f.close();
	}

	void AddPoint(float x_, float y_)
	{
		float scale = 1 * 5000;
		int x = (int)(x_ * w / scale + w/2);
		int y = (int)(y_ * h / scale + h/2);

		int ptr = x + w*y;
		ptr = max(0, min(w*h-1, ptr));

		red[ptr] = min(255, red[ptr] + 25);
		green[ptr] = min(255, green[ptr] + 25);
		blue[ptr] = min(255, blue[ptr] + 25);
	}
};