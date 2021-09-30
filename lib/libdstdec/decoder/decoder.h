/*
* Direct Stream Transfer (DST) codec
* ISO/IEC 14496-3 Part 3 Subpart 10: Technical description of lossless coding of oversampled audio
*/

#ifndef DECODER_H
#define DECODER_H

#include <stdint.h>
#include <array>
#include <vector>
#include "segment.h"
#include "ct.h"
#include "fh.h"
#include "ac.h"
#include "fr.h"
#include "stream.h"

using std::array;
using std::vector;

namespace dst
{

class decoder_t {
	static int          GC_ICoefSign[256];
	static unsigned int GC_ICoefIndex[256];
	static bool         GC_ICoefInit;
public:
	fr_t m_fr;                                    // Contains frame based header information
	ft_t m_ft;                                    // Contains FIR-coef. compression data
	pt_t m_pt;                                    // Contains Ptable-entry compression data
	vector<array<unsigned int, AC_HISMAX>> P_one; // Probability table for arithmetic coder
	vector<uint8_t> AData;                        // Contains the arithmetic coded bit stream of a complete frame	vector<uint8_t> AData;               // Contains the arithmetic coded bit stream of a complete frame
	int             ADataLen;                     // Number of code bits contained in AData[]
	vector<array<array<int16_t, 256>, 16>> LT_ICoefI;
	vector<array<uint8_t, 16>>             LT_Status;
public:
	decoder_t();
	~decoder_t();
	int init(unsigned int channels, unsigned int channel_frame_size);
	int close();
	int decode(const uint8_t* dst_data, unsigned int dst_bits, uint8_t* dsd_data);
private:
	int unpack(const uint8_t* dst_data, uint8_t* dsd_data);
	int16_t reverse7LSBs(int16_t c);
	void fillTable4Bit(segment_t& S, vector<vector<uint8_t>>& Table4Bit);
	void LT_InitCoefTables(vector<array<array<int16_t, 256>, 16>>& ICoefI);
	void GC_InitCoefTables(vector<array<array<int16_t, 256>, 16>>& ICoefI);
	void LT_InitStatus(vector<array<uint8_t, 16>>& Status);
	int16_t LT_RunFilter(array<array<int16_t, 256>, 16>& FilterTable, array<uint8_t, 16>& ChannelStatus);
};

}

#endif
