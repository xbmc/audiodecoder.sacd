/*
* SACD Decoder plugin
* Copyright (c) 2011-2016 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
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

#include "DSDPCMConstants.h"
#include "DSDPCMUtil.h"

template<typename real_t>
class DSDPCMFir {
	using ctable_t = real_t[256];
	ctable_t* fir_ctables;
	int       fir_order;
	int       fir_length;
	int       decimation;
	uint8_t*  fir_buffer;
	int       fir_index;
public:
	DSDPCMFir() {
		fir_ctables = nullptr;
		fir_order = 0;
		fir_length = 0;
		decimation = 1;
		fir_buffer = nullptr;
		fir_index = 0;
	}
	~DSDPCMFir() {
		free();
	}
	void init(ctable_t* fir_ctables, int fir_length, int decimation) {
		this->fir_ctables = fir_ctables;
		this->fir_order = fir_length - 1;
		this->fir_length = CTABLES(fir_length);
		this->decimation = decimation / 8;
		int buf_size = 2 * this->fir_length * sizeof(uint8_t);
		this->fir_buffer = (uint8_t*)DSDPCMUtil::mem_alloc(buf_size);
		memset(this->fir_buffer, DSD_SILENCE_BYTE, buf_size);
		fir_index = 0;
	}
	void free() {
		if (fir_buffer) {
			DSDPCMUtil::mem_free(fir_buffer);
			fir_buffer = nullptr;
		}
	}
	int get_decimation() {
		return decimation;
	}
	float get_delay() {
		return (float)fir_order / 2 / 8 / decimation;
	}
	int run(uint8_t* dsd_data, real_t* m_pcm_data, int dsd_samples) {
		int pcm_samples = dsd_samples / decimation;
		for (int sample = 0; sample < pcm_samples; sample++) {
			for (int i = 0; i < decimation; i++) {
				fir_buffer[fir_index + fir_length] = fir_buffer[fir_index] = *(dsd_data++);
				fir_index = (++fir_index) % fir_length;
			}
			m_pcm_data[sample] = (real_t)0;
			for (int j = 0; j < fir_length; j++) {
				m_pcm_data[sample] += fir_ctables[j][fir_buffer[fir_index + j]];
			}
		}
		return pcm_samples;
	}
};
