/*
* SACD Decoder plugin
* Copyright (c) 2011-2020 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with FFmpeg; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#pragma once

#include <memory.h>
#include <stdlib.h>

#include "DSDPCMConstants.h"

class DSDPCMUtil {
	static constexpr int MEM_ALIGN = 64;
public:
	static void* mem_alloc(size_t size) {
		auto memory = _aligned_malloc(size, MEM_ALIGN);
		if (memory) {
			memset(memory, 0, size);
		}
		return memory;
	}
	static void mem_free(void* memory) {
		if (memory) {
			_aligned_free(memory);
		}
	}
};
