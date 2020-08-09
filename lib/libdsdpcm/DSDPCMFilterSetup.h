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
#include "DSDPCMUtil.h"

#include <math.h>

template<typename real_t>
class DSDPCMFilterSetup	{
	using ctable_t = real_t[256];
	ctable_t* dsd_fir1_8_ctables;
	ctable_t* dsd_fir1_16_ctables;
	ctable_t* dsd_fir1_64_ctables;
	real_t*   pcm_fir2_2_coefs;
	real_t*   pcm_fir3_2_coefs;
	double*   dsd_fir1_64_coefs;
	int       dsd_fir1_64_length;
	bool      dsd_fir1_64_modified;
	float     dsd_fir1_dB_gain;
	double    dsd_fir1_gain;
public:
	DSDPCMFilterSetup() {
		dsd_fir1_8_ctables = nullptr;
		dsd_fir1_16_ctables = nullptr;
		dsd_fir1_64_ctables = nullptr;
		pcm_fir2_2_coefs = nullptr;
		pcm_fir3_2_coefs = nullptr;
		dsd_fir1_64_coefs = nullptr;
		dsd_fir1_64_length = 0;
		dsd_fir1_64_modified = false;
		dsd_fir1_dB_gain = 0;
		dsd_fir1_gain = 1;
	}
	~DSDPCMFilterSetup() {
		flush_fir1_ctables();
		DSDPCMUtil::mem_free(pcm_fir2_2_coefs);
		DSDPCMUtil::mem_free(pcm_fir3_2_coefs);
	}
	void flush_fir1_ctables() {
		DSDPCMUtil::mem_free(dsd_fir1_8_ctables);
		dsd_fir1_8_ctables = nullptr;
		DSDPCMUtil::mem_free(dsd_fir1_16_ctables);
		dsd_fir1_16_ctables = nullptr;
		DSDPCMUtil::mem_free(dsd_fir1_64_ctables);
		dsd_fir1_64_ctables = nullptr;
	}
	static const double NORM_I(const int scale = 0) {
		return (double)1 / (double)((unsigned int)1 << (31 - scale));
	}
	ctable_t* get_fir1_8_ctables() {
		if (!dsd_fir1_8_ctables) {
			dsd_fir1_8_ctables = (ctable_t*)DSDPCMUtil::mem_alloc(CTABLES(DSDFIR1_8_LENGTH) * sizeof(ctable_t));
			set_ctables(DSDFIR1_8_COEFS, DSDFIR1_8_LENGTH, NORM_I(3) * dsd_fir1_gain , dsd_fir1_8_ctables);
		}
		return dsd_fir1_8_ctables;
	}
	int get_fir1_8_length() {
		return DSDFIR1_8_LENGTH;
	}
	ctable_t* get_fir1_16_ctables() {
		if (!dsd_fir1_16_ctables) {
			dsd_fir1_16_ctables = (ctable_t*)DSDPCMUtil::mem_alloc(CTABLES(DSDFIR1_16_LENGTH) * sizeof(ctable_t));
			set_ctables(DSDFIR1_16_COEFS, DSDFIR1_16_LENGTH, NORM_I(3) * dsd_fir1_gain, dsd_fir1_16_ctables);
		}
		return dsd_fir1_16_ctables;
	}
	int get_fir1_16_length() {
		return DSDFIR1_16_LENGTH;
	}
	ctable_t* get_fir1_64_ctables() {
		if (dsd_fir1_64_modified && dsd_fir1_64_coefs && dsd_fir1_64_length > 0) {
			DSDPCMUtil::mem_free(dsd_fir1_64_ctables);
			dsd_fir1_64_ctables = (ctable_t*)DSDPCMUtil::mem_alloc(CTABLES(dsd_fir1_64_length) * sizeof(ctable_t));
			set_ctables(dsd_fir1_64_coefs, dsd_fir1_64_length, dsd_fir1_gain, dsd_fir1_64_ctables);
			dsd_fir1_64_modified = false;
		}
		if (!dsd_fir1_64_ctables) {
			dsd_fir1_64_ctables = (ctable_t*)DSDPCMUtil::mem_alloc(CTABLES(DSDFIR1_64_LENGTH) * sizeof(ctable_t));
			set_ctables(DSDFIR1_64_COEFS, DSDFIR1_64_LENGTH, NORM_I() * dsd_fir1_gain, dsd_fir1_64_ctables);
		}
		return dsd_fir1_64_ctables;
	}
	int get_fir1_64_length() {
		return (dsd_fir1_64_coefs && dsd_fir1_64_length > 0) ? dsd_fir1_64_length : DSDFIR1_64_LENGTH;
	}
	real_t* get_fir2_2_coefs() {
		if (!pcm_fir2_2_coefs) {
			pcm_fir2_2_coefs = (real_t*)DSDPCMUtil::mem_alloc(PCMFIR2_2_LENGTH * sizeof(real_t));
			set_coefs(PCMFIR2_2_COEFS, PCMFIR2_2_LENGTH, NORM_I(), pcm_fir2_2_coefs);
		}
		return pcm_fir2_2_coefs;
	}
	int get_fir2_2_length() {
		return PCMFIR2_2_LENGTH;
	}
	real_t* get_fir3_2_coefs() {
		if (!pcm_fir3_2_coefs) {
			pcm_fir3_2_coefs = (real_t*)DSDPCMUtil::mem_alloc(PCMFIR3_2_LENGTH * sizeof(real_t));
			set_coefs(PCMFIR3_2_COEFS, PCMFIR3_2_LENGTH, NORM_I(), pcm_fir3_2_coefs);
		}
		return pcm_fir3_2_coefs;
	}
	int get_fir3_2_length() {
		return PCMFIR3_2_LENGTH;
	}
	void set_fir1_64_coefs(double* fir_coefs, int fir_length) {
		dsd_fir1_64_modified = dsd_fir1_64_coefs || fir_coefs;
		dsd_fir1_64_coefs = fir_coefs;
		dsd_fir1_64_length = fir_length;
	}
	void set_gain(float dB_gain) {
		if (dB_gain != dsd_fir1_dB_gain) {
			flush_fir1_ctables();
			dsd_fir1_dB_gain = dB_gain;
			dsd_fir1_gain = pow(10.0, dsd_fir1_dB_gain / 20.0);
		}
	}
private:
	int set_ctables(const double* fir_coefs, const int fir_length, const double fir_gain, ctable_t* out_ctables) {
		int ctables = CTABLES(fir_length);
		for (int ct = 0; ct < ctables; ct++) {
			int k = fir_length - ct * 8;
			if (k > 8) {
				k = 8;
			}
			if (k < 0) {
				k = 0;
			}
			for (int i = 0; i < 256; i++) {
				double cvalue = 0.0;
				for (int j = 0; j < k; j++) {
					cvalue += (((i >> (7 - j)) & 1) * 2 - 1) * fir_coefs[fir_length - 1 - (ct * 8 + j)];
				}
				out_ctables[ct][i] = (real_t)(cvalue * fir_gain);
			}
		}
		return ctables;
	}
	void set_coefs(const double* fir_coefs, const int fir_length, const double fir_gain, real_t* out_coefs) {
		for (int i = 0; i < fir_length; i++) {
			out_coefs[i] = (real_t)(fir_coefs[fir_length - 1 - i] * fir_gain);
		}
	}
};

