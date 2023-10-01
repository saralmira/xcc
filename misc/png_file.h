#pragma once

#include "cc_file_sh.h"
#include "cc_structures.h"
#include "virtual_image.h"

class Cpng_file : public Ccc_file_sh<t_png_header>
{
public:
	int decode(Cvirtual_image& d) const;

	bool is_valid() const
	{
		return !(get_size() < sizeof(t_png_header) || memcmp(&header(), png_id, sizeof(t_png_header)));
	}
};

int png_file_write(Cvirtual_file& f, const byte* image, const t_palet_entry* palet, int cx, int cy, int pixel);
int png_file_write(const string& name, const byte* image, const t_palet_entry* palet, int cx, int cy, int pixel);


inline byte linear2sRGB(const unsigned short l)
{
	double lx = l / static_cast<double>(0xffff);
	return static_cast<byte>(lx * 0xff);
	// return static_cast<byte>((lx < 0.0031308 ? lx * 12.92 : (1.055 * pow(lx, (double)1.0 / 2.4) - 0.055)) * 0xff);
}
