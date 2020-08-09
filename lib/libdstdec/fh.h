/*
* Direct Stream Transfer (DST) codec
* ISO/IEC 14496-3 Part 3 Subpart 10: Technical description of lossless coding of oversampled audio
*/

#ifndef FH_H
#define FH_H

#include <stdint.h>
#include <array>
#include <vector>
#include "consts.h"
#include "segment.h"

using std::array;
using std::vector;

namespace dst
{

class fh_t {
public:
	int       NrOfChannels;                                       // Number of channels in the recording
	int       NrOfFilters;                                        // Number of filters used for this frame
	int       NrOfPtables;                                        // Number of Ptables used for this frame
	vector<int> PredOrder;                                        // Prediction order used for this frame
	vector<int> PtableLen;                                        // Nr of Ptable entries used for this frame
	vector<array<int16_t, 1 << SIZE_CODEDPREDORDER>> ICoefA;      // Integer coefs for actual coding
	bool      DSTCoded;                                           // true if DST coded is put in DST stream, false if DSD is put in DST stream
	int       CalcNrOfBytes;                                      // Contains number of bytes of the complete channel stream after arithmetic encoding (also containing bytestuff-, ICoefA-bits, etc.)
	int       CalcNrOfBits;                                       // Contains number of bits of the complete channel stream after arithmetic encoding (also containing bytestuff-, ICoefA-bits, etc.)
	vector<int> HalfProb;                                         // Defines per channel which probability is applied for the first PredOrder[] bits of a frame (0 = use Ptable entry, 1 = 128)
	vector<int> NrOfHalfBits;                                     // Defines per channel how many bits at the start of each frame are optionally coded with p=0.5
	segment_t FSegment;                                           // Contains segmentation data for filters
	vector<vector<uint8_t>> Filter4Bit;                           // Filter4Bit[ChNr][BitNr]
	segment_t PSegment;                                           // Contains segmentation data for Ptables
	vector<vector<uint8_t>> Ptable4Bit;                           // Ptable4Bit[ChNr][BitNr]
	bool      PSameSegAsF;                                        // true if segmentation is equal for F and P
	bool      PSameMapAsF;                                        // true if mapping is equal for F and P
	bool      FSameSegAllCh;                                      // true if all channels have same Filtersegm
	bool      FSameMapAllCh;                                      // true if all channels have same Filtermap
	bool      PSameSegAllCh;                                      // true if all channels have same Ptablesegm
	bool      PSameMapAllCh;                                      // true if all channels have same Ptablemap
	int       SegAndMapBits;                                      // Number of bits in the stream for Seg&Map
	int       MaxNrOfFilters;                                     // Max. nr. of filters allowed per frame
	int       MaxNrOfPtables;                                     // Max. nr. of Ptables allowed per frame
	int       MaxFrameLen;                                        // Max frame length of stream
	int       ByteStreamLen;                                      // MaxFrameLen * NrOfChannels
	int       BitStreamLen;                                       // ByteStreamLen * RESOL
	int       NrOfBitsPerCh;                                      // MaxFrameLen * RESOL
public:
	void init(int channels, int channel_frame_size) {
		NrOfChannels = channels;
		MaxFrameLen = channel_frame_size;
		ByteStreamLen = MaxFrameLen * NrOfChannels;
		BitStreamLen = ByteStreamLen * 8;
		NrOfBitsPerCh = MaxFrameLen * 8;
		MaxNrOfFilters = 2 * NrOfChannels;
		MaxNrOfPtables = 2 * NrOfChannels;
		PredOrder.resize(MaxNrOfFilters);
		PtableLen.resize(MaxNrOfPtables);
		ICoefA.resize(MaxNrOfFilters);
		HalfProb.resize(channels);
		NrOfHalfBits.resize(channels);
		Filter4Bit.resize(channels);
		for (auto& t : Filter4Bit) t.resize(4 * channel_frame_size);
		Ptable4Bit.resize(channels);
		for (auto& t : Ptable4Bit) t.resize(4 * channel_frame_size);
		FSegment.init(channels);
		PSegment.init(channels);
	}
};

}

#endif

