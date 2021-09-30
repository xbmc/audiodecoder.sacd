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

#include <memory.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "dst_decoder_mt.h"

#define DSD_SILENCE_BYTE 0x69

static void dst_run_thread(frame_slot_t& slot) {
	while (slot.run_slot) {
		slot.dst_semaphore.wait();
		if (slot.run_slot) {
			slot.state = slot_state_t::SLOT_RUNNING;
			slot.dec.decode(slot.dst_data, slot.dst_size * 8, slot.dsd_data);
			slot.state = slot_state_t::SLOT_READY;
		}
		else {
			slot.dsd_data = nullptr;
			slot.dst_size = 0;
		}
		slot.dsd_semaphore.notify();
	}
}

dst_decoder_t::dst_decoder_t(unsigned int threads) {
	frame_slots.resize(threads);
	slot_nr            = 0;
	channel_count      = 0;
	channel_frame_size = 0;
}

dst_decoder_t::~dst_decoder_t() {
	for (auto& slot : frame_slots) {
		slot.state = slot_state_t::SLOT_TERMINATING;
		slot.dec.close();
		slot.run_slot = false;
		slot.dst_semaphore.notify(); // Release worker (decoding) thread for exit
		slot.run_thread.join(); // Wait until worker (decoding) thread exit
	}
}   

unsigned int dst_decoder_t::get_slot_nr() {
	return slot_nr;
}

int dst_decoder_t::init(unsigned int channels, unsigned int samplerate, unsigned int framerate) {
	channel_count = channels;
	channel_frame_size = samplerate / 8 / framerate;
	for (auto& slot : frame_slots)	{
		if (slot.dec.init(channel_count, channel_frame_size) == 0) {
			slot.channel_count = channel_count;
			slot.channel_frame_size = channel_frame_size;
			slot.dsd_size = (size_t)(channel_count * channel_frame_size);
			slot.run_slot = true;
			slot.run_thread = thread(dst_run_thread, ref(slot));
			if (!slot.run_thread.joinable()) {
				kodiLog(ADDON_LOG_ERROR, ("Could not start decoder thread"));
				return -1;
			}
		}
		else {
			kodiLog(ADDON_LOG_ERROR, ("Could not initialize decoder slot"));
			return -1;
		}
	}
	return 0;
}

int dst_decoder_t::decode(uint8_t* dst_data, size_t dst_size, uint8_t** dsd_data, size_t* dsd_size) {

	/* Get current slot */
	frame_slot_t& slot_set = frame_slots[slot_nr];

	/* Allocate encoded frame into the slot */ 
	slot_set.dsd_data = *dsd_data;
	slot_set.dst_data = dst_data;
	slot_set.dst_size = dst_size;
    
	/* Release worker (decoding) thread on the loaded slot */
	if (dst_size > 0)	{
		slot_set.state = slot_state_t::SLOT_LOADED;
		slot_set.dst_semaphore.notify();
	}
	else {
		slot_set.state = slot_state_t::SLOT_EMPTY;
	}

	/* Move to the oldest slot */
	slot_nr = (slot_nr + 1) % frame_slots.size();
	frame_slot_t& slot_get = frame_slots[slot_nr];

	/* Dump decoded frame */
	if (slot_get.state != slot_state_t::SLOT_EMPTY) {
		slot_get.dsd_semaphore.wait();
	}
	switch (slot_get.state) {
	case slot_state_t::SLOT_READY:
		*dsd_data = slot_get.dsd_data;
		*dsd_size = (size_t)(channel_count * channel_frame_size);
		break;
	case slot_state_t::SLOT_READY_WITH_ERROR:
		*dsd_data = slot_get.dsd_data;
		*dsd_size = (size_t)(channel_count * channel_frame_size);
		memset(*dsd_data, DSD_SILENCE_BYTE, *dsd_size);
		break;
	default:
		*dsd_data = nullptr;
		*dsd_size = 0;
		break;
	}
	return 0;
}
