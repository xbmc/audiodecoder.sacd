/*
* SACD Decoder plugin
* Copyright (c) 2011-2021 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
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

#include <thread>
#include <vector>

#include "semaphore.h"
#include "DSDPCMConverter.h"

using std::thread;
using std::vector;
using std::ref;

void log_printf(const char* fmt, ...);

template<typename real_t>
class DSDPCMConverterSlot {
public:
	uint8_t*  dsd_data;
	int       dsd_samples;
	real_t*   pcm_data;
	int       pcm_samples;
	semaphore dsd_semaphore;
	semaphore pcm_semaphore;
	bool      run_slot;
	thread    run_thread;
	DSDPCMConverter<real_t>* converter;
	DSDPCMConverterSlot() {
		run_slot = false;
		dsd_data = nullptr;
		dsd_samples = 0;
		pcm_data = nullptr;
		pcm_samples = 0;
		converter = nullptr;
	}
	DSDPCMConverterSlot(const DSDPCMConverterSlot<real_t>& slot) {
		run_slot = slot.run_slot;
		dsd_data = slot.dsd_data;
		dsd_samples = slot.dsd_samples;
		pcm_data = slot.pcm_data;
		pcm_samples = slot.pcm_samples;
		converter = slot.converter;
	}
};

class DSDPCMConverterEngine {
	int   channels;
	int   framerate;
	int   dsd_samplerate;
	int   pcm_samplerate;
	float dB_gain;
	float conv_delay;
	conv_type_e conv_type;
	bool        conv_fp64;
	bool        conv_called;
	bool        conv_need_init;
	vector<DSDPCMConverterSlot<float>>  convSlots_fp32;
	DSDPCMFilterSetup<float>            fltSetup_fp32;
	vector<DSDPCMConverterSlot<double>> convSlots_fp64;
	DSDPCMFilterSetup<double>           fltSetup_fp64;
	uint8_t swap_bits[256];
public:
	DSDPCMConverterEngine();
	~DSDPCMConverterEngine();
	float get_delay();
	void set_gain(float p_dB_gain);
	bool is_convert_called();
	void need_init();
	int init(int p_channels, int p_framerate, int p_dsd_samplerate, int p_pcm_samplerate, conv_type_e p_conv_type, bool p_conv_fp64, double* p_fir_coefs, int p_fir_length);
	int free();
	int convert(uint8_t* p_dsd_data, int p_dsd_samples, float* p_pcm_data);
private:
	template<typename real_t> bool init_slots(vector<DSDPCMConverterSlot<real_t>>& convSlots, DSDPCMFilterSetup<real_t>& fltSetup);
	template<typename real_t> void free_slots(vector<DSDPCMConverterSlot<real_t>>& convSlots);
	template<typename real_t> int convert(vector<DSDPCMConverterSlot<real_t>>& convSlots, uint8_t* dsd_data, int dsd_samples, float* pcm_data);
	template<typename real_t> int convertL(vector<DSDPCMConverterSlot<real_t>>& convSlots, uint8_t* dsd_data, int dsd_samples);
	template<typename real_t> int convertR(vector<DSDPCMConverterSlot<real_t>>& convSlots, float* pcm_data);
	template<typename real_t> void extrapolateL(float* data, int samples);
};
