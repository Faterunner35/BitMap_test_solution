#include "Image.h"

Colour::Colour(float r ,float g, float b) : r(r),g( g), b( b )
{};


Image::Image(int width, int height):m_width{width},m_height{height},m_colours(std::vector<Colour>(width*height))
{};


Image::~Image()
{

};

  void Image::SetColour(const Colour&color, int x, int y)
{
	m_colours.at((y) * m_width + x).r = color.r;

	m_colours.at((y) * m_width + x).g = color.g;

	m_colours.at((y) * m_width + x).b = color.b;
	
	if (m_colours.capacity() == m_colours.size())
	{
		m_colours.resize(808 * 800);
	};


};



Colour Image::GetColour(int x, int y) const
{
	return m_colours[y * m_width + x];
}


inline std::size_t hamming_distance(const Colour& c0, const Colour &c1)
{
	std::size_t result = 0;

	if (c0.r != c1.r) ++result;
	if (c0.g != c1.g) ++result;
	if (c0.b != c1.b) ++result;

	return result;
}




inline double weighted_distance(const unsigned char r0, const unsigned char g0, const unsigned char b0,
	const unsigned char r1, const unsigned char g1, const unsigned char b1)
{
	const double diff_r = /*0.30 */ (r0 - r1);
	const double diff_g = /*0.59 */ (g0 - g1);
	const double diff_b = /*0.11 */ (b0 - b1);

	return std::sqrt((diff_r * diff_r) + (diff_g * diff_g) + (diff_b * diff_b));
}


inline double weighted_distance(const Colour& c0, const Colour& c1)
{
	return weighted_distance(c0.r, c0.g, c0.b,
		c1.r, c1.g, c1.b);
}



Colour Image::find_nearest_pixel(Colour& colour,  Image& img)
{
	 auto it = img.ImageAcess();

	auto iter_img = it.begin();

	/*auto it_e = img.ImageAcess();

	auto it_end = it_e.end() - 1;*/


	double min_d = std::numeric_limits<double>::max();
	Colour result = *iter_img;


	for (auto it_begin:it)
	{


		double distance_nearest = weighted_distance(colour, it_begin);


		if (distance_nearest < min_d)
		{
			min_d = distance_nearest;

			result = *iter_img;
		}


	}
	return result;


};



Image Image::find_nearest_color_forwholeimage( Image& img_1, Image& img_2)
{

	Image* output = new Image(800, 600);


	auto it_begin = img_1.ImageAcess();

	auto it_begin_st = it_begin.begin();
	


	auto it = output->m_colours.begin();




	for (auto itter : it_begin)
	{


		*(it++) = find_nearest_pixel(itter, img_2);


	}

	return *output;

}


void Image::Export(const char* path)
{
	std::ofstream f;

	f.open(path, std::ios::binary | std::ios::out);


	if (!f.is_open())
	{
		std::cout << "File can't be opened" << std::endl;
		return;

	}

	unsigned char bmppad[3] = { 0,0,0 };

	const int paddingammount = (4 - (m_width * 3) % 4 % 4);

	const int fileheadersize = 14;
	const int InfoFileheaderSize = 40;

	const int filesize = InfoFileheaderSize + fileheadersize + m_width * m_height * 3 + paddingammount * m_height;

	unsigned char Fileheader[fileheadersize];

	Fileheader[0] = 'B';
	Fileheader[1] = 'M';

	Fileheader[2] = filesize;

	Fileheader[3] = filesize >> 8;

	Fileheader[4] = filesize >> 16;

	Fileheader[5] = filesize >> 24;


	Fileheader[6] = 0;
	Fileheader[7] = 0;
	Fileheader[8] = 0;
	Fileheader[9] = 0;
	Fileheader[10] = 0;
	Fileheader[11] = 0;
	Fileheader[12] = 0;
	Fileheader[13] = 0;
	


	unsigned char Informationheader[InfoFileheaderSize];


	Informationheader[0] = InfoFileheaderSize;

	Informationheader[1] = 0;
	Informationheader[2] = 0;
	Informationheader[3] = 0;

	Informationheader[4] = m_width;
	Informationheader[5] = m_width >> 8;
	Informationheader[6] = m_width >> 16;

	Informationheader[7] = m_width >> 24;

	Informationheader[8] = m_height;

	Informationheader[9] = m_height >> 8;

	Informationheader[10] = m_height >> 16;

	Informationheader[11] = m_height >> 24;

	Informationheader[12] = 1;


	Informationheader[13] = 0;

	Informationheader[14] = 24;

	Informationheader[15] = 0;


	Informationheader[16] = 0;


	Informationheader[17] = 0;
	Informationheader[18] = 0;
	Informationheader[19] = 0;
	Informationheader[20] = 0;
	Informationheader[21] = 0;
	Informationheader[22] = 0;
	Informationheader[23] = 0;
	Informationheader[24] = 0;
	Informationheader[25] = 0;
	Informationheader[26] = 0;
	Informationheader[27] = 0;
	Informationheader[28] = 0;
	Informationheader[29] = 0;
	Informationheader[30] = 0;
	Informationheader[31] = 0;
	Informationheader[32] = 0;
	Informationheader[33] = 0;
	Informationheader[34] = 0;
	Informationheader[35] = 0;
	Informationheader[36] = 0;
	Informationheader[37] = 0;
	Informationheader[38] = 0;
	Informationheader[39] = 0;
	 




	f.write(reinterpret_cast<char*>(Fileheader), fileheadersize);
	f.write(reinterpret_cast<char*>(Informationheader), InfoFileheaderSize);

	for (int y = 0; y < m_height; y++)
	{
		for (int x = 0; x < m_width; x++)
		{
			unsigned char r = static_cast<unsigned char>(GetColour(x, y).r * 255.0f);
			unsigned char g = static_cast<unsigned char>(GetColour(x, y).g * 255.0f);
			unsigned char b = static_cast<unsigned char>(GetColour(x, y).b * 255.0f);

			unsigned char colour[] = { r,g,b };

			f.write(reinterpret_cast<char*>(colour), 3);
		}
		f.write(reinterpret_cast<char*>(bmppad), paddingammount);

	}
	f.close();

	std::cout << "File created" << std::endl;
};


void Image::ReadImage(const char* path)
{

	std::ifstream f;

	f.open(path, std::ios::binary | std::ios::out);


	if (!f.is_open())
	{
		std::cout << "File can't be opened" << std::endl;
		return;

	}

	unsigned char bmppad[3] = { 0,0,0 };

	const int paddingammount = (4 - (m_width * 3) % 4 % 4);

	const int fileheadersize = 14;
	const int InfoFileheaderSize = 40;
	

	unsigned char Fileheader[fileheadersize];

	f.read(reinterpret_cast<char*>(Fileheader), fileheadersize);


	unsigned char Informationheader[InfoFileheaderSize];


	f.read(reinterpret_cast<char*>(Informationheader), InfoFileheaderSize);


	unsigned char filesize = Fileheader[2] + (Fileheader[3] << 8) + (Fileheader[4] << 16) + (Fileheader[5] << 24);

	m_width = Informationheader[4] + (Informationheader[5] << 8) + (Informationheader[6] << 16) + (Informationheader[7] << 24);

	m_height = Informationheader[4] + (Informationheader[5] << 8) + (Informationheader[6] << 16) + (Informationheader[7] << 24);

	m_colours.resize(m_width * m_height);


	for (int i = 0; i < m_height; i++)
	{
		for (int j = 0; j < m_width; j++)
		{
			unsigned color[3];

			f.read(reinterpret_cast<char*>(color), 3);

			m_colours.at(j * m_width + i).r = static_cast<float>(color[2] / 255.0f);

			m_colours.at(j * m_width + i).g = static_cast<float>(color[1] / 255.0f);

			m_colours.at(j * m_width + i).g = static_cast<float>(color[0] / 255.0f);



		}

		f.ignore(paddingammount);

	}
	f.close();


}


int main()
{
	const int width = 640;
	const int height = 480;

	Image image(width, height);


	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)

		{
			image.SetColour(Colour((float)j / (float)width, 1.0f - (float)j / (float)width, (float)i / (float)height), i, j);

		}
	}


	image.Export("Image.bmp");

	Image image_1(0, 0);

	image_1.ReadImage("Image.bmp");

	Image image_2(0,0);

	image_2.ReadImage("Image.bmp");


	Image image_3(0, 0);
	


	image_3=image_3.find_nearest_color_forwholeimage(image_1, image_2);

	image_3.Export("OUTOUT");



	return 0;

}




