#include "stdafx.h"
#include "tga_file.h"
#include "png_file.h"

bool Ctga_file::is_valid() const
{
	const t_tga_header& h = header();
	int size = get_size();
	return !(sizeof(t_tga_header) > size
		|| h.map_t
		|| h.image_t != 2 && h.image_t != 3
		|| h.map_first
		|| h.map_size
		|| h.cb_pixel != 8 && h.cb_pixel != 16 && h.cb_pixel != 24 && h.cb_pixel != 32
		|| h.horizontal
		|| sizeof(t_tga_header) + cx() * cy() * cb_pixel() > size);
}

int Ctga_file::decode(Cvirtual_image& d) const
{
	switch (cb_pixel())
	{
	case 1:
		{
			t_palet palet;
			for (int i = 0; i < 0x100; i++)
				palet[i].r = palet[i].g = palet[i].b = i;
			d.load(image(), cx(), cy(), cb_pixel(), palet);
		}
		break;
	case 2:
		{
			d.load(NULL, cx(), cy(), 3, NULL);
			const __int16* r = reinterpret_cast<const __int16*>(image());
			t_palet_entry* w = reinterpret_cast<t_palet_entry*>(d.image_edit());
			for (int i = 0; i < cx() * cy(); i++)
			{
				int v = *r++;
				w->r = (v >> 10 & 0x1f) * 255 / 31;
				w->g = (v >> 5 & 0x1f) * 255 / 31;
				w->b = (v & 0x1f) * 255 / 31;
				w++;
			}
		}
		break;
	default:
		d.load(image(), cx(), cy(), cb_pixel(), NULL);
		switch (d.cb_pixel())
		{
		case 4:
			d.remove_alpha();
		case 3:
			d.swap_rb();
			break;			
		}		

	}
	if (!header().vertical)
		d.flip();	
	return 0;
}

Cvirtual_file tga_file_write(const byte* image, int cx, int cy, int cb_pixel)
{
	Cvirtual_binary d;
	int tga_pixel;
	tga_pixel = cb_pixel >= 6 ? (cb_pixel / 2) : cb_pixel;
	byte* w = d.write_start(sizeof(t_tga_header) + cx * cy * tga_pixel);
	t_tga_header& header = *reinterpret_cast<t_tga_header*>(w);
	memset(&header, 0, sizeof(t_tga_header));
	header.image_t = tga_pixel == 1 ? 3 : 2;
	header.cx = cx;
	header.cy = cy;
	header.cb_pixel = tga_pixel << 3;
	header.alpha = tga_pixel == 4 ? 8 : 0;
	header.vertical = true;
	w += sizeof(t_tga_header);
	if (cb_pixel == 1)
		memcpy(w, image, cx * cy * cb_pixel);
	else
	{
		switch (cb_pixel)
		{
		case 3:
		{
			auto* r = reinterpret_cast<const t_palet_entry*>(image);
			for (int i = 0; i < cx * cy; i++)
			{
				*w++ = r->b;
				*w++ = r->g;
				*w++ = r->r;
				r++;
			}
			break;
		}
		case 4:
		{
			auto* r32 = reinterpret_cast<const t_palet32_entry*>(image);
			for (int i = 0; i < cx * cy; i++)
			{
				*w++ = r32->b;
				*w++ = r32->g;
				*w++ = r32->r;
				*w++ = r32->a;
				r32++;
			}
			break;
		}
		case 6:
		{
			auto* r48 = reinterpret_cast<const t_palet48_entry*>(image);
			for (int i = 0; i < cx * cy; i++)
			{
				*w++ = linear2sRGB(r48->b);
				*w++ = linear2sRGB(r48->g);
				*w++ = linear2sRGB(r48->r);
				r48++;
			}
			break;
		}
		case 8:
		{
			auto* r64 = reinterpret_cast<const t_palet64_entry*>(image);
			for (int i = 0; i < cx * cy; i++)
			{
				*w++ = linear2sRGB(r64->b);
				*w++ = linear2sRGB(r64->g);
				*w++ = linear2sRGB(r64->r);
				*w++ = linear2sRGB(r64->a);
				r64++;
			}
			break;
		}
		default:
			break;
		}
	}
	return d;
}

Cvirtual_file tga_file_write(const byte* image, int cx, int cy, const t_palet_entry* palet)
{
	Cvirtual_image vi;
	vi.load(image, cx, cy, 1, palet);
	vi.increase_color_depth(3);
	return vi.save(ft_tga);
}
