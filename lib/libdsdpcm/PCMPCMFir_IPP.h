/*
* SACD Decoder plugin
* Copyright (c) 2011-2015 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
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

using IppsFIRSpec = void;

template<typename real_t>
class PCMPCMFir {
	real_t* fir_coefs;
	int     fir_order;
	int     fir_length;
	int     decimation;

	real_t*      fir_dly;
	Ipp8u*       fir_buf;
	IppsFIRSpec* fir_spec;
public:
	PCMPCMFir() {
		if (!g_ipp_initialized) {
			ippInit();
			g_ipp_initialized = true;
		}
		fir_coefs = nullptr;
		fir_order = 0;
		fir_length = 0;
		decimation = 0;

		fir_dly = nullptr;
		fir_buf = nullptr;
		fir_spec = nullptr;
	}
	~PCMPCMFir() {
		free();
	}
	int get_decimation() {
		return decimation;
	}
	float get_delay() {
		return (float)fir_order / 2 / decimation;
	}
	void init(real_t* p_fir_coefs, int p_fir_length, int p_decimation) {
		fir_coefs = p_fir_coefs;
		fir_length = p_fir_length;
		decimation = p_decimation;
		fir_order = p_fir_length - 1;
		fir_dly = (sizeof(real_t) == sizeof(float)) ? reinterpret_cast<real_t*>(ippsMalloc_32f(p_fir_length)) : reinterpret_cast<real_t*>(ippsMalloc_64f(p_fir_length));
		int specSize = 0;
		int bufSize = 0;
		ippsFIRMRGetSize(p_fir_length, 1, p_decimation, (sizeof(real_t) == sizeof(float)) ? ipp32f : ipp64f, &specSize, &bufSize);
		fir_buf = ippsMalloc_8u(bufSize);
		fir_spec = reinterpret_cast<IppsFIRSpec*>(ippsMalloc_8u(specSize));
		Fir_IPP::FIRMRInit(p_fir_coefs, p_fir_length, 1, 0, p_decimation, 0, fir_spec);
 	}
	void free() {
		if (fir_spec) {
			ippsFree(fir_spec);
			fir_spec = nullptr;
		}
		if (fir_buf) {
			ippsFree(fir_buf);
			fir_buf = nullptr;
		}
		if (fir_dly) {
			ippsFree(fir_dly);
			fir_dly = nullptr;
		}
	}
	int run(real_t* m_pcm_data, real_t* out_data, int pcm_samples) {
		int out_samples = pcm_samples / decimation;
		Fir_IPP::FIRMR(m_pcm_data, out_data, out_samples, fir_spec, fir_dly, fir_dly, fir_buf);
		return out_samples;
	}
};
