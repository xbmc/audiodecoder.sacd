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

#include <algorithm>
#include "DSDPCMConstants.h"
#include "Fir_IPP.h"

using std::max;

template<typename real_t>
class DSDPCMFir {
	using ctable_t = real_t[256];
	ctable_t* fir_ctables;
	int       fir_order;
	int       fir_length;
	int       decimation;

	Ipp8u*  fir_dly;
	real_t* fir_out;
public:
	DSDPCMFir() {
		if (!g_ipp_initialized) {
			ippInit();
			g_ipp_initialized = true;
		}
		fir_ctables = nullptr;
		fir_order = 0;
		fir_length = 0;
		decimation = 0;
		fir_dly = nullptr;
	}
	~DSDPCMFir() {
		free();
	}
	int get_decimation() {
		return decimation;
	}
	float get_delay() {
		return (float)fir_order / 2 / 8 / decimation;
	}
	void init(ctable_t* fir_ctables, int fir_length, int decimation) {
		this->fir_ctables = fir_ctables;
		this->fir_order = fir_length - 1;
		this->fir_length = CTABLES(fir_length);
		this->decimation = decimation / 8;
		this->fir_dly = ippsMalloc_8u(this->fir_length);
		memset(this->fir_dly, DSD_SILENCE_BYTE, this->fir_length);
		this->fir_out = (sizeof(real_t) == sizeof(float)) ? reinterpret_cast<real_t*>(ippsMalloc_32f(fir_length)) : reinterpret_cast<real_t*>(ippsMalloc_64f(fir_length));
	}
	void free() {
		if (fir_dly) {
			ippsFree(fir_dly);
			fir_dly = nullptr;
		}
		if (fir_out) {
			ippsFree(fir_out);
			fir_out = nullptr;
		}
	}
	int run(uint8_t* dsd_data, real_t* m_pcm_data, int dsd_samples) {
		int pcm_samples = dsd_samples / decimation;
		int fir_index = 0;
		for (int sample = 0; sample < pcm_samples; sample++) {
			for (int j = 0; j < fir_length - fir_index; j++) {
				fir_out[j] = fir_ctables[j][fir_dly[fir_index + j]];
			}
			for (int j = max(fir_length - fir_index, 0); j < fir_length; j++) {
				fir_out[j] = fir_ctables[j][dsd_data[j - (fir_length - fir_index)]];
			}
			fir_index += decimation;
			Fir_IPP::Sum(fir_out, fir_length, &m_pcm_data[sample]);
		}
		ippsCopy_8u(&dsd_data[dsd_samples - fir_length], fir_dly, fir_length);
		return pcm_samples;
	}
};
