#include "stdafx.h"
#include "csf_file.h"

#include <comdef.h>

string encode(string s)
{
	string r(s);
	for (int i = 0; i < s.size(); ++i)
	{
		r[i] = ~s[i];
	}
	return r;
}

wstring encode(wstring s)
{
	wstring r(s);
	for (int i = 0; i < s.size(); ++i)
	{
		r[i] = ~s[i];
	}
	return r;
}

int read_int(const byte*& r)
{
	int v = *reinterpret_cast<const __int32*>(r);
	r += 4;
	return v;
}

void write_int(byte*& w, int v)
{
	*reinterpret_cast<__int32*>(w) = v;
	w += 4;
}

string read_string(const byte*& r)
{
	int cb_s = read_int(r);
	string s = string(reinterpret_cast<const char*>(r), cb_s);
	r += cb_s;
	return s;
}

wstring read_wstring(const byte*& r)
{
	int cb_s = read_int(r);
	wstring s = encode(wstring(reinterpret_cast<const wchar_t*>(r), cb_s));
	r += cb_s << 1;
	return s;
}

void write_string(byte*& w, string v)
{
	int l = v.length();
	write_int(w, l);
	memcpy(w, v.c_str(), l);
	w += l;
}

void write_wstring(byte*& w, wstring v)
{
	int l = v.length();
	write_int(w, l);
	memcpy(w, encode(v).c_str(), l << 1);
	w += l << 1;
}

int Ccsf_file::post_open()
{
	if (!is_valid())
		return 1;
	if (vdata().size() != get_size())
		return 0;
	const byte* r = data() + sizeof(t_csf_header);
	for (int i = 0; i < header().count1; i++)
	{
		read_chunk(r);
	}
	return 0;
}

void Ccsf_file::read_chunk(const byte*& stm, const string* lblname)
{
	switch (read_int(stm))
	{
		case csf_label_id:
		{
			int num = read_int(stm);
			string name = read_string(stm);

			for (; num > 0; --num)
				read_chunk(stm, &name);

			break;
		}
		case csf_string_id:
		{
			wstring value = read_wstring(stm);
			set_value(lblname ? *lblname : string(), value, string());
			break;
		}
		case csf_string_w_id:
		{
			wstring value = read_wstring(stm);
			string extra_value = read_string(stm);
			set_value(lblname ? *lblname : string(), value, extra_value);
			break;
		}
	default:
		break;
	}
}

string Ccsf_file::convert2string(const wstring& s)
{
	string result;
	int len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), s.size(), NULL, 0, NULL, NULL);
	if (len <= 0)return result;
	char* buffer = new char[len + 1];
	if (buffer == NULL)return result;
	WideCharToMultiByte(CP_ACP, 0, s.c_str(), s.size(), buffer, len, NULL, NULL);
	buffer[len] = '\0';
	result.append(buffer);
	delete[] buffer;
	return result;
	//string r;
	//for (int i = 0; i < s.length(); i++)
	//	r += ~s[i];
	//return r;
}

wstring Ccsf_file::convert2wstring(const string& s)
{
	wstring result;
	int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), s.size(), NULL, 0);
	if (len < 0)return result;
	wchar_t* buffer = new wchar_t[len + 1];
	if (buffer == NULL)return result;
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), s.size(), buffer, len);
	buffer[len] = '\0';
	result.append(buffer);
	delete[] buffer;
	return result;
	//wstring r;
	//for (int i = 0; i < s.length(); i++)
	//	r += 0xff00 | ~s[i];
	//return r;
}

void Ccsf_file::erase_value(const string& name)
{
	m_map.erase(name);
}

string Ccsf_file::get_converted_value(const string& name) const
{
	return convert2string(get_value(name));
}

void Ccsf_file::set_value(const string& name, const wstring& value, const string& extra_value)
{
	t_map_entry& e = m_map[name];
	e.value = value;
	e.extra_value = extra_value;
}

int Ccsf_file::get_write_size() const
{
	int r = sizeof(t_csf_header);
	for (auto& i : m_map)
	{
		r += 20 + i.first.length() + (i.second.value.length() << 1);
		if (!i.second.extra_value.empty())
			r += 4 + i.second.extra_value.length();
	}
	return r;
}

void Ccsf_file::write(byte* d) const
{
	byte* w = d;
	t_csf_header& header = *reinterpret_cast<t_csf_header*>(w);
	w += sizeof(t_csf_header);
	header.id = csf_file_id;
	header.flags1 = 3;
	header.count1 = get_c_strings();
	header.count2 = get_c_strings();
	header.zero = 0;
	header.flags2 = 0;
	for (auto& i : m_map)
	{
		write_int(w, csf_label_id);
		write_int(w, 1);
		write_string(w, i.first);
		write_int(w, i.second.extra_value.empty() ? csf_string_id : csf_string_w_id);
		write_wstring(w, i.second.value);
		if (!i.second.extra_value.empty())
			write_string(w, i.second.extra_value);
	}
	assert(w - d == get_write_size());
}

Cvirtual_binary Ccsf_file::write() const
{
	Cvirtual_binary d;
	write(d.write_start(get_write_size()));
	return d;
}

string read_string2(const byte*& r, const int size)
{
	string s = string(reinterpret_cast<const char*>(r), size);
	r += size;
	return s;
}

wstring read_wstring2(const byte*& r, const int size)
{
	wstring s = encode(wstring(reinterpret_cast<const wchar_t*>(r), size));
	r += size << 1;
	return s;
}

int Ccsf_file_rd::post_open()
{
	if (!is_valid())
		return 1;
	int cb_s;
	auto size = vdata().size();
	auto ptr_end = data() + size;
	const byte* r = data() + sizeof(t_csf_header);
	for (int i = 0; i < header().count1; i++)
	{
		if (r + 12 > ptr_end) break;
		read_int(r);
		int flags = read_int(r);
		cb_s = read_int(r);
		if (r + cb_s > ptr_end) break;
		string name = read_string2(r, cb_s);
		if (flags & 1)
		{
			if (r + 8 > ptr_end) break;
			bool has_extra_value = read_int(r) == csf_string_w_id;
			cb_s = read_int(r);
			if (r + (cb_s << 1) > ptr_end) break;
			wstring value = read_wstring2(r, cb_s);
			string extra_value;
			if (has_extra_value)
			{
				cb_s = read_int(r);
				if (r + cb_s > ptr_end) break;
				extra_value = read_string2(r, cb_s);
			}
			set_value(name, value, extra_value);
		}
		else
			set_value(name, wstring(), string());
	}
	return 0;
}

