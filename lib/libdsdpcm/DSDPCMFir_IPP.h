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

#include "DSDPCMConstants.h"
#include "Fir_IPP.h"

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
	void init(ctable_t* p_fir_ctables, int p_fir_length, int p_decimation) {
		fir_ctables = p_fir_ctables;
		fir_order = p_fir_length - 1;
		fir_length = CTABLES(p_fir_length);
		decimation = p_decimation / 8;
		fir_dly = ippsMalloc_8u(fir_length);
		memset(fir_dly, DSD_SILENCE_BYTE, fir_length);
		fir_out = (sizeof(real_t) == sizeof(float)) ? reinterpret_cast<real_t*>(ippsMalloc_32f(p_fir_length)) : reinterpret_cast<real_t*>(ippsMalloc_64f(p_fir_length));
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
	int run(uint8_t* p_dsd_data, real_t* p_pcm_data, int p_dsd_samples) {
		auto pcm_samples = p_dsd_samples / decimation;
		auto fir_index = 0;
		for (auto sample = 0; sample < pcm_samples; sample++) {
			for (auto j = 0; j < fir_length - fir_index; j++) {
				fir_out[j] = fir_ctables[j][fir_dly[fir_index + j]];
			}
			for (auto j = (fir_length > fir_index) ? fir_length - fir_index : 0; j < fir_length; j++) {
				fir_out[j] = fir_ctables[j][p_dsd_data[j - (fir_length - fir_index)]];
			}
			fir_index += decimation;
			Fir_IPP::Sum(fir_out, fir_length, &p_pcm_data[sample]);
		}
		ippsCopy_8u(&p_dsd_data[p_dsd_samples - fir_length], fir_dly, fir_length);
		return pcm_samples;
	}
};
