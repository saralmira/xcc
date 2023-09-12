#include <stdafx.h>
#include "mix_file_rd.h"

#include "big_file.h"
#include "blowfish.h"
#include "crc.h"
#include "id_log.h"
#include "mix_cache.h"
#include "mix_decode.h"
#include "mix_rg_file.h"
#include "pak_file.h"
#include "string_conversion.h"
#include "xcc_lmd_file.h"
#include "cc_file.h"
#include "mix_file.h"

Cmix_file_rd::Cmix_file_rd() :
	Cmix_file()
{
}

bool Cmix_file_rd::is_valid()
{
	const byte* data = get_data();
	if (!data)
		return false;
	const t_mix_header& header = *reinterpret_cast<const t_mix_header*>(data);
	int size = get_size();
	if (sizeof(t_mix_header) > size)
		return false;
	if (header.c_files && sizeof(t_mix_header) + header.c_files * sizeof(t_mix_index_entry) + header.size == size)
		return true;
	m_has_checksum = header.flags & mix_checksum;
	m_is_encrypted = header.flags & mix_encrypted;
	if (m_is_encrypted)
	{
		Cblowfish bf;
		std::array<byte, cb_mix_key> key;
		get_blowfish_key(data + 4, key);
		bf.set_key(key);
		byte e[8];
		bf.decipher(data + 84, e, 8);
		t_mix_header* header = reinterpret_cast<t_mix_header*>(e);
		if (!header->c_files)// || 84 + (sizeof(t_mix_header) + header->c_files * sizeof(t_mix_index_entry) + 7 & ~7) + header->size + (m_has_checksum ? 20 : 0) != size)
			return false;
	}
	else
	{
		const t_mix_header* header = reinterpret_cast<const t_mix_header*>(data + 4);
		if (!header->c_files)// || 4 + sizeof(t_mix_header) + header->c_files * sizeof(t_mix_index_entry) + header->size + (m_has_checksum ? 20 : 0) != size)
			return false;
	}
	return true;
}

#define test_fail(res) { int v = res; if (v) { close(); return v; }}

int Cmix_file_rd::post_open()
{
#ifndef NO_FT_SUPPORT
	bool index_read = false;
	if (!index_read)
#endif
	{
		t_mix_header header;
		seek(0);
		test_fail(read(&header, sizeof(t_mix_header)));
		m_has_checksum = header.flags & mix_checksum;
		m_is_encrypted = header.flags & mix_encrypted;
		bool aligned = true;
		std::vector<int> entry_discards;
		Cblowfish bf;
		seek(4);
		if (m_is_encrypted)
		{
			byte key_source[cb_mix_key_source];
			read(key_source, cb_mix_key_source);
			std::array<byte, cb_mix_key> key;
			get_blowfish_key(key_source, key);
			bf.set_key(key);
			byte e[8];
			read(e, 8);
			bf.decipher(e, e, 8);
			memcpy(&header, e, sizeof(t_mix_header));
			int c_files = header.c_files;
			const int cb_index = c_files * sizeof(t_mix_index_entry);
			const int cb_f = cb_index + 5 & ~7;
			if (c_files < 0)
				test_fail(1);
			if (c_files)
			{
				Cvirtual_binary f;
				read(f.write_start(cb_f), cb_f);
				bf.decipher(f.data_edit(), f.data_edit(), cb_f);
				m_index.resize(c_files);
				memcpy(&m_index[0], e + 6, 2);
				memcpy(reinterpret_cast<byte*>(&m_index[0]) + 2, f.data(), cb_index - 2);
				auto body_size = get_size() - 92 - cb_f - (m_has_checksum ? 20 : 0);
				for (int i = 0; i < c_files; i++)
				{
					if (m_index[i].offset < 0 || m_index[i].size < 0 || (m_index[i].offset + m_index[i].size) > body_size)
					{
						entry_discards.push_back(i);
					}
				}
				for (auto i_e : entry_discards)
				{
					m_index.erase(m_index.begin() + i_e);
					c_files = --header.c_files;
				}
				for (int i = 0; i < c_files; i++)
				{
					if (m_index[i].offset & 0xf)
						aligned = false;
					m_index[i].offset += 92 + cb_f;
				}
			}
		}
		else
		{
			read(&header, sizeof(header));
			int c_files = header.c_files;
			const int cb_index = c_files * sizeof(t_mix_index_entry);
			auto mix_size = get_size();
			if (c_files < 0)
				test_fail(1);
			m_index.resize(c_files);
			read(&m_index[0], cb_index);
			auto body_size = get_size() - (sizeof(t_mix_header) + cb_index) - (m_has_checksum ? 20 : 0);
			for (int i = 0; i < c_files; i++)
			{
				if (m_index[i].offset < 0 || m_index[i].size < 0 || (m_index[i].offset + m_index[i].size) > body_size)
				{
					entry_discards.push_back(i);
				}
			}
			for (auto i_e : entry_discards)
			{
				m_index.erase(m_index.begin() + i_e);
				c_files = --header.c_files;
			}
			m_index.resize(c_files);
			for (int i = 0; i < c_files; i++)
			{
				if (m_index[i].offset & 0xf)
					aligned = false;
				m_index[i].offset += 4 + sizeof(t_mix_header) + cb_index;
			}
		}
		m_game = is_encrypted() ? game_ra : game_ts;
		for (int i = 0; i < get_c_files(); i++)
		{
			if (get_id(i) == 0x763c81dd)
			{
				m_game = game_ts;
				break;
			}
		}
		if (m_game == game_ra && aligned)
			m_game = game_ts;
	}
	for (int i = 0; i < get_c_files(); i++)
		m_id_index[get_id(i)] = i;
#ifndef NO_FT_SUPPORT
	if (m_ft_support)
	{
		switch (m_game)
		{
		case game_dune2:
		case game_rg:
			break;
		default:
			int count[game_unknown] = {0};
			for (int i = 0; i < get_c_files(); i++)
			{
				int id = get_id(i);
				for (int game = game_td; game < game_unknown; game++)
					count[game] += mix_database::get_name(static_cast<t_game>(game), id).empty();
			}
			int min = count[0];
			for (int game = 0; game < game_unknown; game++)
			{
				if (count[game] < min)
				{
					m_game = static_cast<t_game>(game);
					min = count[game];
				}
			}
		}
		if (vdata().size() == get_size())
		{
			int crc = compute_crc(&m_index[0], get_c_files() * sizeof(t_mix_index_entry));
			Cvirtual_binary s = mix_cache::get_data(crc);
			m_index_ft.resize(get_c_files());
			if (s.size() == get_c_files() * sizeof(t_file_type))
				memcpy(&m_index_ft[0], s.data(), get_c_files() * sizeof(t_file_type));
			else
			{
				using t_block_map = multimap<int, int>;

				t_block_map block_map;
				for (int i = 0; i < get_c_files(); i++)
					block_map.insert(t_block_map::value_type(get_offset(get_id(i)), i));
				for (auto& i : block_map)
				{
					Ccc_file f(false);
					f.open(get_id(i.second), *this);
					m_index_ft[i.second] = f.get_file_type();
				}
				mix_cache::set_data(crc, Cvirtual_binary(&m_index_ft[0], get_c_files() * sizeof(t_file_type)));
			}
			for (int i = 0; i < get_c_files(); i++)
			{
				int id = get_id(i);
				Cxcc_lmd_file f;
				if (get_type(id) != ft_xcc_lmd || f.open(id, *this) || !f.is_valid())
					continue;
				m_game = f.get_game();
				int count = f.get_c_fnames();
				const char* r = f.get_fnames();
				while (count--)
				{
					string name = r;
					r += name.length() + 1;
					mix_database::add_name(m_game, name, "-");
				}
			}
		}
	}
#endif
	if (m_mix_expansion)
	{
		int c_files = get_c_files();
		for (int i = 0; i < c_files; i++)
		{
			if (get_type(m_index[i].id) != ft_mix)
				continue;
			Cmix_file_rd f;
			f.open(m_index[i].id, *this);				
			int c_files = get_c_files();
			int new_c_files = f.get_c_files();
			m_index.resize(c_files + new_c_files);
			for (int j = 0; j < new_c_files; j++)
			{
				int id = f.get_id(j);
				m_index[c_files + j] = t_mix_index_entry(id, f.get_offset(id) + get_offset(m_index[i].id), f.get_size(id));
				m_id_index[id] = c_files + j;
			}
			m_index_ft.resize(c_files + new_c_files);
			memcpy(&m_index_ft[c_files], &f.m_index_ft[0], new_c_files * sizeof(t_file_type));
		}
	}
    return 0;
}
