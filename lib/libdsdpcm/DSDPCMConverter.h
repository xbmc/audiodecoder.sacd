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

#include "DSDPCMFilterSetup.h"

#ifndef _USE_IPP
#include "DSDPCMFir.h"
#include "PCMPCMFir.h"
#else
#include "DSDPCMFir_IPP.h"
#include "PCMPCMFir_IPP.h"
#endif

enum class conv_type_e {
	DSDPCM_CONV_UNKNOWN    = -1,
	DSDPCM_CONV_MULTISTAGE =  0,
	DSDPCM_CONV_DIRECT     =  1,
	DSDPCM_CONV_USER       =  2
};

template<typename real_t>
class DSDPCMConverter {
protected:
	int     framerate;
	int     dsd_samplerate;
	int     pcm_samplerate;
	float   delay;
	real_t* pcm_temp1;
	real_t* pcm_temp2;
public:
	DSDPCMConverter() {
		pcm_temp1 = nullptr;
		pcm_temp2 = nullptr;
	}
	virtual ~DSDPCMConverter() {
		free_pcm_temp1();
		free_pcm_temp2();
	}
	float get_delay() {
		return delay;
	}
	virtual void init(DSDPCMFilterSetup<real_t>& flt_setup, int dsd_samples) = 0;
	virtual int convert(uint8_t* dsd_data, real_t* m_pcm_data, int dsd_samples) = 0;
protected:
	void alloc_pcm_temp1(int pcm_samples) {
		free_pcm_temp1();
		pcm_temp1 = (real_t*)DSDPCMUtil::mem_alloc(pcm_samples * sizeof(real_t));
	}
	void alloc_pcm_temp2(int pcm_samples) {
		free_pcm_temp2();
		pcm_temp2 = (real_t*)DSDPCMUtil::mem_alloc(pcm_samples * sizeof(real_t));
	}
	void free_pcm_temp1() {
		DSDPCMUtil::mem_free(pcm_temp1);
		pcm_temp1 = nullptr;
	}
	void free_pcm_temp2() {
		DSDPCMUtil::mem_free(pcm_temp2);
		pcm_temp2 = nullptr;
	}
};
