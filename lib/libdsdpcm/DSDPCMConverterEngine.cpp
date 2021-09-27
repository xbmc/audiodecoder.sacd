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

#include "DSDPCMConverterMultistage.h"
#include "DSDPCMConverterDirect.h"
#include "DSDPCMConverterEngine.h"
#include <algorithm>
#include <math.h>
#include <stdio.h>

#ifdef BUILD_KODI_ADDON
#include <kodi/General.h>

#define LOG_ERROR   ADDON_LOG_ERROR
#define LOG_WARNING ADDON_LOG_WARNING
#define LOG_INFO    ADDON_LOG_INFO
#define LOG(p1, p2) kodi::Log(p1, "%s", p2)
#else
#define LOG_ERROR   ("Error: ")
#define LOG_WARNING ("Warning: ")
#define LOG_INFO    ("Info: ")
#define LOG(p1, p2) fprintf(stderr, "%s%s", p1, p2)
#endif

using std::min;
using std::max;

template<typename real_t>
static void converter_thread(DSDPCMConverterSlot<real_t>& slot) {
	while (slot.run_slot) {
		slot.dsd_semaphore.wait();
		if (slot.run_slot) {
			slot.pcm_samples = slot.converter->convert(slot.dsd_data, slot.pcm_data, slot.dsd_samples);
		}
		else {
			slot.pcm_samples = 0;
		}
		slot.pcm_semaphore.notify();
	}
}

DSDPCMConverterEngine::DSDPCMConverterEngine() {
	channels = 0;
	framerate = 0;
	dsd_samplerate = 0;
	pcm_samplerate = 0;
	dB_gain = 0.0f;
	conv_delay = 0.0f;
	conv_type = conv_type_e::UNKNOWN;
	conv_called = false;
	conv_need_init = true;
	for (int i = 0; i < 256; i++) {
		swap_bits[i] = 0;
		for (int j = 0; j < 8; j++) {
			swap_bits[i] |= (uint8_t)(((i >> j) & 1) << (7 - j));
		}
	}
}

DSDPCMConverterEngine::~DSDPCMConverterEngine() {
	free();
	conv_delay = 0.0f;
}

float DSDPCMConverterEngine::get_delay() {
	return conv_delay;
}

void DSDPCMConverterEngine::set_gain(float p_dB_gain) {
	conv_need_init = conv_need_init || (dB_gain != p_dB_gain);
	dB_gain = p_dB_gain;
}

bool DSDPCMConverterEngine::is_convert_called() {
	return conv_called;
}

void DSDPCMConverterEngine::need_init() {
	conv_need_init = true;
}

int DSDPCMConverterEngine::init(int p_channels, int p_framerate, int p_dsd_samplerate, int p_pcm_samplerate, conv_type_e p_conv_type, bool p_conv_fp64, double* p_fir_coefs, int p_fir_length) {
	if (!conv_need_init && channels == p_channels && framerate == p_framerate && dsd_samplerate == p_dsd_samplerate && pcm_samplerate == p_pcm_samplerate && conv_type == p_conv_type && conv_fp64 == p_conv_fp64) {
		return 1;
	}
	if (p_conv_type == conv_type_e::USER) {
		if (!(p_fir_coefs && p_fir_length > 0)) {
			return -2;
		}
	}
	free();
	channels = p_channels;
	framerate = p_framerate;
	dsd_samplerate = p_dsd_samplerate;
	pcm_samplerate = p_pcm_samplerate;
	conv_type = p_conv_type;
	conv_fp64 = p_conv_fp64;
	if (p_conv_fp64) {
		fltSetup_fp64.set_gain(dB_gain);
		fltSetup_fp64.set_fir1_64_coefs(p_fir_coefs, p_fir_length);
		init_slots<double>(convSlots_fp64, fltSetup_fp64);
		conv_delay = convSlots_fp64[0].converter->get_delay();
	}
	else {
		fltSetup_fp32.set_gain(dB_gain);
		fltSetup_fp32.set_fir1_64_coefs(p_fir_coefs, p_fir_length);
		init_slots<float>(convSlots_fp32, fltSetup_fp32);
		conv_delay = convSlots_fp32[0].converter->get_delay();
	}
	conv_called = false;
	conv_need_init = false;
	return 0;
}

int DSDPCMConverterEngine::free() {
	if (conv_fp64) {
		free_slots<double>(convSlots_fp64);
	}
	else {
		free_slots<float>(convSlots_fp32);
	}
	return 0;
}

int DSDPCMConverterEngine::convert(uint8_t* p_dsd_data, int p_dsd_samples, float* p_pcm_data) {
	int pcm_samples = 0;
	if (!p_dsd_data) {
		if (conv_fp64) {
			pcm_samples = convertR<double>(convSlots_fp64, p_pcm_data);
		}
		else {
			pcm_samples = convertR<float>(convSlots_fp32, p_pcm_data);
		}
		return pcm_samples;
	}
	if (!conv_called) {
		if (conv_fp64) {
			convertL<double>(convSlots_fp64, p_dsd_data, p_dsd_samples);
		}
		else {
			convertL<float>(convSlots_fp32, p_dsd_data, p_dsd_samples);
		}
	}
	if (conv_fp64) {
		pcm_samples = convert<double>(convSlots_fp64, p_dsd_data, p_dsd_samples, p_pcm_data);
	}
	else {
		pcm_samples = convert<float>(convSlots_fp32, p_dsd_data, p_dsd_samples, p_pcm_data);
	}
	if (!conv_called) {
		extrapolateL<float>(p_pcm_data, pcm_samples);
		conv_called = true;
	}
	return pcm_samples;
}

template<typename real_t>
bool DSDPCMConverterEngine::init_slots(vector<DSDPCMConverterSlot<real_t>>& convSlots, DSDPCMFilterSetup<real_t>& fltSetup) {
	convSlots.resize(channels);
	int dsd_samples = dsd_samplerate / 8 / framerate;
	int pcm_samples = pcm_samplerate / framerate;
	int decimation = dsd_samplerate / pcm_samplerate;
	for (auto& slot : convSlots) {
		slot.dsd_data = (uint8_t*)DSDPCMUtil::mem_alloc(dsd_samples * sizeof(uint8_t));
		slot.dsd_samples = dsd_samples;
		slot.pcm_data = (real_t*)DSDPCMUtil::mem_alloc(pcm_samples * sizeof(real_t));
		slot.pcm_samples = 0;
		switch (conv_type) {
		case conv_type_e::MULTISTAGE:
		{
			DSDPCMConverter<real_t>* pConv = nullptr;
			switch (decimation) {
			case 1024:
				pConv = new DSDPCMConverterMultistage<real_t, 1024>();
				break;
			case 512:
				pConv = new DSDPCMConverterMultistage<real_t, 512>();
				break;
			case 256:
				pConv = new DSDPCMConverterMultistage<real_t, 256>();
				break;
			case 128:
				pConv = new DSDPCMConverterMultistage<real_t, 128>();
				break;
			case 64:
				pConv = new DSDPCMConverterMultistage<real_t, 64>();
				break;
			case 32:
				pConv = new DSDPCMConverterMultistage<real_t, 32>();
				break;
			case 16:
				pConv = new DSDPCMConverterMultistage<real_t, 16>();
				break;
			case 8:
				pConv = new DSDPCMConverterMultistage<real_t, 8>();
				break;
			}
			pConv->init(fltSetup, dsd_samples);
			slot.converter = pConv;
			break;
		}
		case conv_type_e::DIRECT:
		case conv_type_e::USER:
		{
			DSDPCMConverter<real_t>* pConv = nullptr;
			switch (decimation) {
			case 1024:
				pConv = new DSDPCMConverterDirect<real_t, 1024>();
				break;
			case 512:
				pConv = new DSDPCMConverterDirect<real_t, 512>();
				break;
			case 256:
				pConv = new DSDPCMConverterDirect<real_t, 256>();
				break;
			case 128:
				pConv = new DSDPCMConverterDirect<real_t, 128>();
				break;
			case 64:
				pConv = new DSDPCMConverterDirect<real_t, 64>();
				break;
			case 32:
				pConv = new DSDPCMConverterDirect<real_t, 32>();
				break;
			case 16:
				pConv = new DSDPCMConverterDirect<real_t, 16>();
				break;
			case 8:
				pConv = new DSDPCMConverterDirect<real_t, 8>();
				break;
			}
			pConv->init(fltSetup, dsd_samples);
			slot.converter = pConv;
			break;
		}
		default:
			break;
		}
		slot.run_slot = true;
		slot.run_thread = thread(converter_thread<real_t>, ref(slot));
		if (!slot.run_thread.joinable()) {
			LOG(LOG_ERROR, ("Could not start decoder thread"));
			return false;
		}
	}
	return true;
}

template<typename real_t>
void DSDPCMConverterEngine::free_slots(vector<DSDPCMConverterSlot<real_t>>& convSlots) {
	for (auto& slot : convSlots) {
		slot.run_slot = false;
		slot.dsd_semaphore.notify(); // Release worker (decoding) thread for exit
		slot.run_thread.join(); // Wait until worker (decoding) thread exit
		delete slot.converter;
		slot.converter = nullptr;
		DSDPCMUtil::mem_free(slot.dsd_data);
		slot.dsd_data = nullptr;
		slot.dsd_samples = 0;
		DSDPCMUtil::mem_free(slot.pcm_data);
		slot.pcm_data = nullptr;
		slot.pcm_samples = 0;
	}
	convSlots.resize(0);
}

template<typename real_t>
int DSDPCMConverterEngine::convert(vector<DSDPCMConverterSlot<real_t>>& convSlots, uint8_t* dsd_data, int dsd_samples, float* pcm_data) {
	int pcm_samples = 0;
	int ch = 0;
	for (auto& slot : convSlots)	{
		slot.dsd_samples = dsd_samples / channels;
		for (int sample = 0; sample < slot.dsd_samples; sample++)	{
			slot.dsd_data[sample] = dsd_data[sample * channels + ch];
		}
		slot.dsd_semaphore.notify(); // Release worker (decoding) thread on the loaded slot
		ch++;
	}
	ch = 0;
	for (auto& slot : convSlots)	{
		slot.pcm_semaphore.wait();	// Wait until worker (decoding) thread is complete
		for (int sample = 0; sample < slot.pcm_samples; sample++)	{
			pcm_data[sample * channels + ch] = (float)slot.pcm_data[sample];
		}
		pcm_samples += slot.pcm_samples;
		ch++;
	}
	return pcm_samples;
}

template<typename real_t>
int DSDPCMConverterEngine::convertL(vector<DSDPCMConverterSlot<real_t>>& convSlots, uint8_t* dsd_data, int dsd_samples) {
	int ch = 0;
	for (auto& slot : convSlots)	{
		slot.dsd_samples = dsd_samples / channels;
		for (int sample = 0; sample < slot.dsd_samples; sample++)	{
			slot.dsd_data[sample] = swap_bits[dsd_data[(slot.dsd_samples - 1 - sample) * channels + ch]];
		}
		slot.dsd_semaphore.notify(); // Release worker (decoding) thread on the loaded slot
		ch++;
	}
	for (auto& slot : convSlots)	{
		slot.pcm_semaphore.wait(); // Wait until worker (decoding) thread is complete
	}
	return 0;
}

template<typename real_t>
int DSDPCMConverterEngine::convertR(vector<DSDPCMConverterSlot<real_t>>& convSlots, float* pcm_data) {
	int pcm_samples = 0;
	for (auto& slot : convSlots)	{
		for (int sample = 0; sample < slot.dsd_samples / 2; sample++)	{
			uint8_t temp = slot.dsd_data[slot.dsd_samples - 1 - sample];
			slot.dsd_data[slot.dsd_samples - 1 - sample] = swap_bits[slot.dsd_data[sample]];
			slot.dsd_data[sample] = swap_bits[temp];
		}
		slot.dsd_semaphore.notify(); // Release worker (decoding) thread on the loaded slot
	}
	int ch = 0;
	for (auto& slot : convSlots)	{
		slot.pcm_semaphore.wait();	// Wait until worker (decoding) thread is complete
		for (int sample = 0; sample < slot.pcm_samples; sample++)	{
			pcm_data[sample * channels + ch] = (float)slot.pcm_data[sample];
		}
		pcm_samples += slot.pcm_samples;
		ch++;
	}
	return pcm_samples;
}

template<typename real_t>
void DSDPCMConverterEngine::extrapolateL(float* data, int samples) {
	auto t0 = (int)(2.0f * get_delay() + 0.5f);
	if (2 * t0 > samples) {
		return;
	}
	for (auto ch = 0; ch < channels; ch++) {
		auto d0 = data[t0 * channels + ch];
		for (auto sample = 0; sample < t0; sample++) {
			auto c0 = (float)(t0 - 1 - sample) / (float)t0;
			data[(t0 - 1 - sample) * channels + ch] = pow(c0, 1.25f) * (d0 + (d0 - data[(t0 + 1 + sample) * channels + ch]));
		}
	}
}
