#include "stdafx.h"
#include "cc_file.h"
#include "xcc_infantry.h"

namespace xcc_infantry
{
	t_infantry_data_entry infantry_data[256];
}

static const char* infantry_xif_fname = "infantry.xif";
Cxif_key infantry_key;

enum
{
	vi_id_long_name,
	vi_id_short_name,
	vi_id_cx,
	vi_id_cy,
	vi_id_flags,
};

dword xcc_infantry::c_infantry()
{
	for (long i = 0; i < 256; i++)
	{
		if (~infantry_data[i].flags & id_flags_in_use)
			return i;
	}
	return 0;
}

long xcc_infantry::load_data()
{
	Ccc_file f(false);
	f.open(xcc_dirs::get_data_dir() + infantry_xif_fname);
	if (!f.is_open())
		return 1;
	const dword size = f.get_size();
	byte* data = new byte[size];
	f.read(data, size);
	f.close();
	infantry_key.load_key(data, size);
	delete[] data;
	long infantry_i = 0;
	for (t_xif_key_map::iterator i = infantry_key.m_keys.begin(); i != infantry_key.m_keys.end(); i++)
	{
		t_infantry_data_entry& id = infantry_data[infantry_i];
		Cxif_key& ik = i->second;
		id.long_name = ik.get_value_string(vi_id_long_name);
		id.short_name = ik.get_value_string(vi_id_short_name);
		id.cx = ik.get_value_int(vi_id_cx);
		id.cy = ik.get_value_int(vi_id_cy);
		id.flags = ik.get_value_int(vi_id_flags);
		for (t_xif_value_map::const_iterator i = ik.m_values.begin(); i != ik.m_values.end(); i++)
		{
			switch (i->first)
			{
			}
		}
		infantry_i++;
	}
	return 0;
}

long xcc_infantry::save_data()
{
	typedef map<string, long> t_list;
	t_list list;	
	{
		for (long i = 0; i < 256; i++)
		{
			t_infantry_data_entry& id = infantry_data[i];
			if (~id.flags & id_flags_in_use)
				// don't save if not in use
				continue;
			list[static_cast<string>(id.short_name)] = i;
		}
	}
	infantry_key = Cxif_key(); // .delete_all_keys();
	long infantry_i = 0;
	for (t_list::const_iterator i = list.begin(); i != list.end(); i++)
	{
		t_infantry_data_entry& id = infantry_data[i->second];
		Cxif_key& ik = infantry_key.set_key(infantry_i);
		ik.set_value_string(vi_id_long_name, id.long_name);
		ik.set_value_string(vi_id_short_name, id.short_name);
		ik.set_value(vi_id_cx, static_cast<dword>(id.cx));
		ik.set_value(vi_id_cy, static_cast<dword>(id.cy));
		ik.set_value(vi_id_flags, id.flags);
		infantry_i++;
	}
	return infantry_key.vdata().export(xcc_dirs::get_data_dir() + infantry_xif_fname);
	/*
	Cfile32 f;
	if (f.open_write(xcc_dirs::get_data_dir() + infantry_xif_fname))
		return 1;
	dword size = infantry_key.key_size();
	byte* data = new byte[size];
	infantry_key.save_key(data);
	f.write(data, size);
	delete[] data;
	f.close();
	return 0;
	*/
}

long xcc_infantry::load_images(bool load_icons)
{
	long error = 0;
	static bool loaded = false;
	if (loaded)
		return 0;
	Cmix_file& conquer_mix = Cxcc_mixs::get_conquer_mix();
	for (long i = 0; i < 256; i++)
	{
		t_infantry_data_entry& id = infantry_data[i];
		if (~id.flags & id_flags_in_use)
			continue;
		Cshp_file f;
		// images
		f.open(static_cast<string>(id.short_name) + ".shp", conquer_mix);
		if (!f.is_open())
		{
			error = 1;
			continue;
		}
		if (shp_images::load_shp(f, id.images))
			error = 1;
		f.close();
		if (load_icons && id.flags & id_flags_icon)
		{
			// icon
			f.open(static_cast<string>(id.short_name) + "icon.shp", conquer_mix);
			if (!f.is_open())
			{
				error = 1;
				continue;
			}
			if (shp_images::load_shp(f, id.icon))
				error = 1;
			f.close();
		}
	}
	if (!error)
		loaded = true;
	return error;
}

void xcc_infantry::destroy()
{
	for (long i = 0; i < 256; i++)
	{
		t_infantry_data_entry& id = infantry_data[i];
	}
}

long xcc_infantry::get_id(const string& s)
{
	for (long i = 0; i < 256; i++)
	{
		t_infantry_data_entry& id = infantry_data[i];
		if (id.flags & id_flags_in_use && (id.short_name == s))
			return i;
	}
	return -1;
}