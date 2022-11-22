#pragma once

#include<cstdlib>
#include<vector>
#include<fstream>
#include<iostream>

struct Colour
{
	float r, g, b;

	Colour() = default;

	Colour(float r,float g, float b);
	
	
	Colour& operator=(const Colour& img)
	{


		if (this != &img)
		{
			this->r = img.r;
			this->g = img.g;
			this->b = img.b;


		}
		else

			return *this;
	}

};





class Image
{
public:

	Image(int width, int height);

	~Image();

	Colour GetColour(int x,int y) const;

	void  SetColour(const Colour& colour, int x, int y);

	void Export(const char* path);

	void ReadImage(const char* path);

    Colour find_nearest_pixel(Colour& colour, Image& img);

	Image find_nearest_color_forwholeimage(Image& img_1,Image& img_2);





	const inline std::vector<Colour>& ImageAcess()  {

		return m_colours;

	};

private:

	int m_width;
	int m_height;

	std::vector<Colour>m_colours;

};
