#pragma once

#include "cc_file.h"
#include "cc_structures.h"
#include "mix_file.h"

class Cmix_file_rd : public Cmix_file  
{
public:
	int post_open();
	bool is_valid();
	Cmix_file_rd();
};
