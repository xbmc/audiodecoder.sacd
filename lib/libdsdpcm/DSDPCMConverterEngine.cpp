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

#include <math.h>
#include <stdio.h>
#include "DSDPCMConverterEngine.h"

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

template<typename real_t>
void converter_thread(DSDPCMConverterSlot<real_t>* slot) {
	while (slot->run_slot) {
		slot->dsd_semaphore.wait();
		if (slot->run_slot) {
			slot->pcm_samples = slot->converter->convert(slot->dsd_data, slot->pcm_data, slot->dsd_samples);
		}
		else {
			slot->pcm_samples = 0;
		}
		slot->pcm_semaphore.notify();
	}
}

DSDPCMConverterEngine::DSDPCMConverterEngine() {
	channels = 0;
	framerate = 0;
	dsd_samplerate = 0;
	pcm_samplerate = 0;
	dB_gain = 0.0f;
	conv_delay = 0.0f;
	conv_type = conv_type_e::DSDPCM_CONV_UNKNOWN;
	conv_called = false;
	conv_need_reinit = false;
	for (int i = 0; i < 256; i++) {
		swap_bits[i] = 0;
		for (int j = 0; j < 8; j++) {
			swap_bits[i] |= ((i >> j) & 1) << (7 - j);
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

void DSDPCMConverterEngine::set_gain(float dB_gain) {
	if (this->dB_gain != dB_gain) {
		conv_need_reinit = true;
		this->dB_gain = dB_gain;
	}
}


bool DSDPCMConverterEngine::is_convert_called() {
	return conv_called;
}

int DSDPCMConverterEngine::init(int channels, int framerate, int dsd_samplerate, int pcm_samplerate, conv_type_e conv_type, bool conv_fp64, double* fir_coefs, int fir_length) {
	if (!conv_need_reinit && this->channels == channels && this->framerate == framerate && this->dsd_samplerate == dsd_samplerate && this->pcm_samplerate == pcm_samplerate && this->conv_type == conv_type && this->conv_fp64 == conv_fp64) {
		return 1;
	}
	if (conv_type == conv_type_e::DSDPCM_CONV_USER) {
		if (!(fir_coefs && fir_length > 0)) {
			return -2;
		}
	}
	free();
	this->channels = channels;
	this->framerate = framerate;
	this->dsd_samplerate = dsd_samplerate;
	this->pcm_samplerate = pcm_samplerate;
	this->conv_type = conv_type;
	this->conv_fp64 = conv_fp64;
	if (conv_fp64) {
		fltSetup_fp64.set_gain(dB_gain);
		fltSetup_fp64.set_fir1_64_coefs(fir_coefs, fir_length);
		init_slots<double>(convSlots_fp64, fltSetup_fp64);
		conv_delay = convSlots_fp64[0].converter->get_delay();
	}
	else {
		fltSetup_fp32.set_gain(dB_gain);
		fltSetup_fp32.set_fir1_64_coefs(fir_coefs, fir_length);
		init_slots<float>(convSlots_fp32, fltSetup_fp32);
		conv_delay = convSlots_fp32[0].converter->get_delay();
	}
	conv_called = false;
	conv_need_reinit = false;
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

int DSDPCMConverterEngine::convert(uint8_t* dsd_data, int dsd_samples, float* m_pcm_data) {
	int pcm_samples = 0;
	if (!dsd_data) {
		if (conv_fp64) {
			pcm_samples = convertR<double>(convSlots_fp64, m_pcm_data);
		}
		else {
			pcm_samples = convertR<float>(convSlots_fp32, m_pcm_data);
		}
		return pcm_samples;
	}
	if (!conv_called) {
		if (conv_fp64) {
			convertL<double>(convSlots_fp64, dsd_data, dsd_samples);
		}
		else {
			convertL<float>(convSlots_fp32, dsd_data, dsd_samples);
		}
		conv_called = true;
	}
	if (conv_fp64) {
		pcm_samples = convert<double>(convSlots_fp64, dsd_data, dsd_samples, m_pcm_data);
	}
	else {
		pcm_samples = convert<float>(convSlots_fp32, dsd_data, dsd_samples, m_pcm_data);
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
		case conv_type_e::DSDPCM_CONV_MULTISTAGE:
		{
			DSDPCMConverterMultistage<real_t>* pConv = nullptr;
			switch (decimation) {
			case 1024:
				pConv = new DSDPCMConverterMultistage_x1024<real_t>();
				break;
			case 512:
				pConv = new DSDPCMConverterMultistage_x512<real_t>();
				break;
			case 256:
				pConv = new DSDPCMConverterMultistage_x256<real_t>();
				break;
			case 128:
				pConv = new DSDPCMConverterMultistage_x128<real_t>();
				break;
			case 64:
				pConv = new DSDPCMConverterMultistage_x64<real_t>();
				break;
			case 32:
				pConv = new DSDPCMConverterMultistage_x32<real_t>();
				break;
			case 16:
				pConv = new DSDPCMConverterMultistage_x16<real_t>();
				break;
			case 8:
				pConv = new DSDPCMConverterMultistage_x8<real_t>();
				break;
			}
			pConv->init(fltSetup, dsd_samples);
			slot.converter = pConv;
			break;
		}
		case conv_type_e::DSDPCM_CONV_DIRECT:
		case conv_type_e::DSDPCM_CONV_USER:
		{
			DSDPCMConverterDirect<real_t>* pConv = nullptr;
			switch (decimation) {
			case 1024:
				pConv = new DSDPCMConverterDirect_x1024<real_t>();
				break;
			case 512:
				pConv = new DSDPCMConverterDirect_x512<real_t>();
				break;
			case 256:
				pConv = new DSDPCMConverterDirect_x256<real_t>();
				break;
			case 128:
				pConv = new DSDPCMConverterDirect_x128<real_t>();
				break;
			case 64:
				pConv = new DSDPCMConverterDirect_x64<real_t>();
				break;
			case 32:
				pConv = new DSDPCMConverterDirect_x32<real_t>();
				break;
			case 16:
				pConv = new DSDPCMConverterDirect_x16<real_t>();
				break;
			case 8:
				pConv = new DSDPCMConverterDirect_x8<real_t>();
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
		slot.run_thread = thread(converter_thread<real_t>, &slot);
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
