/*
* Direct Stream Transfer (DST) codec
* ISO/IEC 14496-3 Part 3 Subpart 10: Technical description of lossless coding of oversampled audio
*/

#include <memory.h>
#include "decoder.h"

namespace dst
{

static unsigned int int_abs(int n) {
	int mask = n >> (8 * sizeof(int) - 1);
	return (unsigned int)((n + mask) ^ mask);
}

static unsigned int int_log2(unsigned int n) {
	return (n > 1) ? 1 + int_log2(n >> 1) : 0;
}

int          decoder_t::GC_ICoefSign[256];
unsigned int decoder_t::GC_ICoefIndex[256];
bool         decoder_t::GC_ICoefInit = false;

decoder_t::decoder_t() {
	if (!GC_ICoefInit) {
		GC_ICoefSign[0] = 0;
		GC_ICoefIndex[0] = (unsigned int)-1;
		for (int i = 1; i < 256; i++) {
			auto i_gray_delta = (i ^ (i >> 1)) - ((i - 1) ^ ((i - 1) >> 1));
			auto i_gray_delta_abs = int_abs(i_gray_delta);
			auto i_gray_index = int_log2(i_gray_delta_abs);
			if (i_gray_delta > 0) {
				GC_ICoefSign[i] = +1;
			}
			if (i_gray_delta < 0) {
				GC_ICoefSign[i] = -1;
			}
			GC_ICoefIndex[i] = i_gray_index;
		}
		GC_ICoefInit = true;
	}
}

decoder_t::~decoder_t() {
}

int decoder_t::init(unsigned int channels, unsigned int channel_frame_size) {
	m_fr.init(channels, channel_frame_size);
	m_ft.init(2 * channels);
	m_pt.init(2 * channels);
	P_one.resize(2 * channels);
	AData.resize(channels * channel_frame_size);
	LT_ICoefI.resize(2 * channels);
	LT_Status.resize(channels);
	return 0;
}

int decoder_t::close() {
	return 0;
}

// Decode a complete frame (all channels)
int decoder_t::decode(const uint8_t* dst_data, unsigned int dst_bits, uint8_t* dsd_data) {
	int     rv = 0;
	uint8_t ACError;

	auto NrOfBitsPerCh = m_fr.NrOfBitsPerCh;
	auto NrOfChannels = m_fr.NrOfChannels;

	m_fr.CalcNrOfBytes = dst_bits / 8;
	m_fr.CalcNrOfBits = m_fr.CalcNrOfBytes * 8;

	/* unpack DST frame: segmentation, mapping, arithmetic data */
	rv = unpack(dst_data, dsd_data);
	if (rv == -1) {
		return -1;
	}

	if (m_fr.DSTCoded == 1) {
		ac_t AC;

		fillTable4Bit(m_fr.FSegment, m_fr.Filter4Bit);
		fillTable4Bit(m_fr.PSegment, m_fr.Ptable4Bit);

		GC_InitCoefTables(LT_ICoefI);
		LT_InitStatus(LT_Status);

		AC.decodeBit_Init(AData.data(), ADataLen);
		AC.decodeBit_Decode(&ACError, reverse7LSBs(m_fr.ICoefA[0][0]), AData.data(), ADataLen);

		memset(dsd_data, 0, (NrOfBitsPerCh * NrOfChannels + 7) / 8);
		for (auto BitNr = 0u; BitNr < NrOfBitsPerCh; BitNr++) {
			for (auto ChNr = 0u; ChNr < NrOfChannels; ChNr++) {
				int16_t Predict;
				uint8_t Residual;
				int16_t BitVal;
				auto FilterNr = GET_NIBBLE(m_fr.Filter4Bit[ChNr].data(), BitNr);

				/* Calculate output value of the FIR filter */
				Predict = LT_RunFilter(LT_ICoefI[FilterNr], LT_Status[ChNr]);

				/* Arithmetic decode the incoming bit */
				if ((m_fr.HalfProb[ChNr]/* == 1*/) && (BitNr < m_fr.NrOfHalfBits[ChNr])) {
					AC.decodeBit_Decode(&Residual, AC_PROBS / 2, AData.data(), ADataLen);
				}
				else {
					auto PtableNr = GET_NIBBLE(m_fr.Ptable4Bit[ChNr].data(), BitNr);
					auto PtableIndex = AC.getPtableIndex(Predict, m_fr.PtableLen[PtableNr]);
					AC.decodeBit_Decode(&Residual, P_one[PtableNr][PtableIndex], AData.data(), ADataLen);
				}

				/* Channel bit depends on the predicted bit and BitResidual[][] */
				BitVal = (int16_t)(((((uint16_t)Predict) >> 15) ^ Residual) & 1);

				/* Shift the result into the correct bit position */
				dsd_data[(BitNr >> 3) * NrOfChannels + ChNr] |= (uint8_t)(BitVal << (7 - (BitNr & 7)));

				/* Update filter */
				{
					uint64_t* const st = reinterpret_cast<uint64_t*>(LT_Status[ChNr].data());
					st[1] = (st[1] << 1) | (st[0] >> 63);
					st[0] = (st[0] << 1) | BitVal;
				}
			}
		}

		/* Flush the arithmetic decoder */
		AC.decodeBit_Flush(&ACError, 0, AData.data(), ADataLen);

		if (ACError != 1) {
			log_printf("ERROR: Arithmetic decoding error");
			rv = -1;
		}
	}

	return rv;
}

// Read a complete frame from the DST input stream
int decoder_t::unpack(const uint8_t* dst_data, uint8_t* dsd_data) {
	m_fr.set_data(dst_data, m_fr.CalcNrOfBytes); // Assign DST data from input stream
	m_fr.DSTCoded = m_fr.get_bit(); // Read Processing_Mode (Table 10.4)
	if (!m_fr.DSTCoded) {
		int unused;
		unused = m_fr.get_uint(1); // Read DST_X_Bit (Table 10.4)
		unused = m_fr.get_uint(6); // Read Reserved (Table 10.4)
		if (unused) {
			log_printf("ERROR: Illegal stuffing pattern in frame");
			return -1;
		}
		m_fr.read_dsd_data(dsd_data); // Read DSD data and put in output stream
	}
	else {
		m_fr.read_segmentation(); // Read Segmentation (Table 10.4)
		m_fr.read_mapping(); // Read Mapping (Table 10.4)
		m_fr.read_filter_coef_sets(m_ft); // Read Filter_Coef_Sets (Table 10.4)
		m_fr.read_probability_tables(m_pt, P_one); // Read Probability_Tables (Table 10.4)
		ADataLen = m_fr.CalcNrOfBits - m_fr.get_offset();
		m_fr.read_arithmetic_coded_data(AData.data()); // Read Arithmetic_Coded_Data (Table 10.4)
		if (ADataLen > 0 && GET_BIT(AData.data(), 0) != 0) {
			log_printf("ERROR: Illegal arithmetic code in frame");
			return -1;
		}
	}
	return 0;
}

/* Take the 7 LSBs of a number consisting of SIZE_PREDCOEF bits */
/* (2's complement), reverse the bit order and add 1 to it.     */

int16_t decoder_t::reverse7LSBs(int16_t c) {
	const int16_t reverse[128] = {
		1, 65, 33, 97, 17, 81, 49, 113, 9, 73, 41, 105, 25, 89, 57, 121,
		5, 69, 37, 101, 21, 85, 53, 117, 13, 77, 45, 109, 29, 93, 61, 125,
		3, 67, 35, 99, 19, 83, 51, 115, 11, 75, 43, 107, 27, 91, 59, 123,
		7, 71, 39, 103, 23, 87, 55, 119, 15, 79, 47, 111, 31, 95, 63, 127,
		2, 66, 34, 98, 18, 82, 50, 114, 10, 74, 42, 106, 26, 90, 58, 122,
		6, 70, 38, 102, 22, 86, 54, 118, 14, 78, 46, 110, 30, 94, 62, 126,
		4, 68, 36, 100, 20, 84, 52, 116, 12, 76, 44, 108, 28, 92, 60, 124,
		8, 72, 40, 104, 24, 88, 56, 120, 16, 80, 48, 112, 32, 96, 64, 128 };
	return reverse[(c + (1 << SIZE_PREDCOEF)) & 127];
}

/* Fill an array that indicates for each bit of each channel which table number must be used */

void decoder_t::fillTable4Bit(segment_t& S, vector<vector<uint8_t>>& Table4Bit) {
	unsigned int SegNr;
	unsigned int Start;
	unsigned int End;
	int8_t Val;
	for (auto ChNr = 0u; ChNr < m_fr.NrOfChannels; ChNr++) {
		for (SegNr = 0u, Start = 0; SegNr < S.NrOfSegments[ChNr] - 1; SegNr++) {
			Val = (int8_t)S.Table4Segment[ChNr][SegNr];
			End = Start + S.Resolution * 8 * S.SegmentLength[ChNr][SegNr];
			for (auto BitNr = Start; BitNr < End; BitNr++) {
				uint8_t* p = (uint8_t*)&Table4Bit[ChNr][BitNr / 2];
				auto s = (BitNr & 1) << 2;
				*p = (uint8_t)(((uint8_t)Val << s) | (*p & (0xf0 >> s)));
			}
			Start += S.Resolution * 8 * S.SegmentLength[ChNr][SegNr];
		}
		Val = (int8_t)S.Table4Segment[ChNr][SegNr];
		for (auto BitNr = Start; BitNr < m_fr.NrOfBitsPerCh; BitNr++) {
			uint8_t* p = (uint8_t*)&Table4Bit[ChNr][BitNr / 2];
			auto s = (BitNr & 1) << 2;
			*p = (uint8_t)(((uint8_t)Val << s) | (*p & (0xf0 >> s)));
		}
	}
}

void decoder_t::LT_InitCoefTables(vector<array<array<int16_t, 256>, 16>>& ICoefI) {
	for (auto FilterNr = 0u; FilterNr < m_fr.NrOfFilters; FilterNr++) {
		auto FilterLength = m_fr.PredOrder[FilterNr];
		for (auto TableNr = 0u; TableNr < 16u; TableNr++) {
			auto k = (int)FilterLength - (int)TableNr * 8;
			if (k > 8) {
				k = 8;
			}
			else if (k < 0) {
				k = 0;
			}
			for (auto i = 0; i < 256; i++) {
				int cvalue = 0;
				for (auto j = 0; j < k; j++) {
					cvalue += (((i >> j) & 1) * 2 - 1) * m_fr.ICoefA[FilterNr][TableNr * 8 + j];
				}
				ICoefI[FilterNr][TableNr][i] = (int16_t)cvalue;
			}
		}
	}
}

void decoder_t::GC_InitCoefTables(vector<array<array<int16_t, 256>, 16>>& ICoefI) {
	for (auto FilterNr = 0u; FilterNr < m_fr.NrOfFilters; FilterNr++) {
		auto FilterLength = m_fr.PredOrder[FilterNr];
		for (auto TableNr = 0u; TableNr < 16u; TableNr++) {
			auto k = (int)FilterLength - (int)TableNr * 8;
			if (k > 8) {
				k = 8;
			}
			else if (k < 0) {
				k = 0;
			}
			int cvalue = 0;
			for (auto j = 0; j < k; j++) {
				cvalue -= m_fr.ICoefA[FilterNr][TableNr * 8 + j];
			}
			ICoefI[FilterNr][TableNr][0] = (int16_t)cvalue;
			for (auto i = 1; i < 256; i++) {
				auto i_gray = i ^ (i >> 1);
				auto j_gray = GC_ICoefIndex[i];
				if (j_gray < (unsigned int)k) {
					cvalue += GC_ICoefSign[i] * (m_fr.ICoefA[FilterNr][TableNr * 8 + j_gray] << 1);
				}
				ICoefI[FilterNr][TableNr][i_gray] = (int16_t)cvalue;
			}
		}
	}
}

void decoder_t::LT_InitStatus(vector<array<uint8_t, 16>>& Status) {
	for (auto ChNr = 0u; ChNr < m_fr.NrOfChannels; ChNr++) {
		for (auto TableNr = 0u; TableNr < 16u; TableNr++) {
			Status[ChNr][TableNr] = 0xaa;
		}
	}
}

int16_t decoder_t::LT_RunFilter(array<array<int16_t, 256>, 16>& FilterTable, array<uint8_t, 16>& ChannelStatus) {
	int Predict;
	Predict = FilterTable[0][ChannelStatus[0]];
	Predict += FilterTable[1][ChannelStatus[1]];
	Predict += FilterTable[2][ChannelStatus[2]];
	Predict += FilterTable[3][ChannelStatus[3]];
	Predict += FilterTable[4][ChannelStatus[4]];
	Predict += FilterTable[5][ChannelStatus[5]];
	Predict += FilterTable[6][ChannelStatus[6]];
	Predict += FilterTable[7][ChannelStatus[7]];
	Predict += FilterTable[8][ChannelStatus[8]];
	Predict += FilterTable[9][ChannelStatus[9]];
	Predict += FilterTable[10][ChannelStatus[10]];
	Predict += FilterTable[11][ChannelStatus[11]];
	Predict += FilterTable[12][ChannelStatus[12]];
	Predict += FilterTable[13][ChannelStatus[13]];
	Predict += FilterTable[14][ChannelStatus[14]];
	Predict += FilterTable[15][ChannelStatus[15]];
	return (int16_t)Predict;
}

}
