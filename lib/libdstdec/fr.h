/*
* Direct Stream Transfer (DST) codec
* ISO/IEC 14496-3 Part 3 Subpart 10: Technical description of lossless coding of oversampled audio
*/

#ifndef FR_H
#define FR_H

#include <stdint.h>
#include "consts.h"
#include "fh.h"
#include "ct.h"
#include "stream.h"

namespace dst
{

class fr_t : public stream_t, public fh_t {
public:

	// Calculate the log2 of an integer and round the result up by using integer arithmetic
	unsigned int log2_round_up(unsigned int x) {
		unsigned int y = 0;
		while (x >= (1u << y)) {
			y++;
		}
		return y;
	}

	// Rice decoding of encoded Filters and Ptables
	int rice_decode(unsigned int m) {
		int LSBs;
		int Nr;
		int RLBit;
		int RunLength;
		int Sign;

		// Retrieve run length code
		RunLength = 0;
		do {
			RLBit = get_bit(); // Read RL_Bit (Table 10.13)
			RunLength += (1 - RLBit);
		} while (!RLBit);
		// Retrieve least significant bits
		LSBs = (int)get_uint(m); // Read LSBs (Table 10.13)
		Nr = (RunLength << m) + LSBs;
		/* Retrieve optional sign bit */
		if (Nr != 0) {
			Sign = get_bit(); // Read Sign (Table 10.13)
			if (Sign) {
				Nr = -Nr;
			}
		}
		return Nr;
	}

	// Read DSD data section from DST input stream (Table 10.4)
	void read_dsd_data(uint8_t* dsd_frame) {
		for (auto i = 0u; i < MaxFrameLen * NrOfChannels; i++) {
			dsd_frame[i] = (uint8_t)get_uint(8);
		}
	}

	// Read segmentation for Filters and Ptables (Table 10.5)
	void read_table_segmentation(unsigned int MaxNrOfSegments, unsigned int MinSegmentLength, segment_t& Segment, bool& SameSegmentForAllChannels) {
		bool ResolutionRead = false;
		unsigned int ChNr = 0;
		unsigned int DefinedBits = 0;
		unsigned int SegmentNr = 0;
		unsigned int NrOfBits;
		bool EndOfChannelSegment;
		auto MaxSegmentSize = MaxFrameLen - MinSegmentLength / 8;
		SameSegmentForAllChannels = get_bit(); // Read Same_Segm_For_All_Channels (Table 10.6)
		if (SameSegmentForAllChannels) {
			EndOfChannelSegment = get_bit(); // Read End_Of_Channel_Segm (Table 10.7)
			while (!EndOfChannelSegment) {
				if (SegmentNr >= MaxNrOfSegments) {
					log_printf("ERROR: Too many segments for this channel");
					return;
				}
				if (!ResolutionRead) {
					NrOfBits = log2_round_up(MaxFrameLen - MinSegmentLength / 8);
					Segment.Resolution = get_uint(NrOfBits); // Read Resolution (Table 10.7)
					if ((Segment.Resolution == 0) || (Segment.Resolution > MaxFrameLen - MinSegmentLength / 8)) {
						log_printf("ERROR: Invalid segment resolution");
						return;
					}
					ResolutionRead = true;
				}
				NrOfBits = log2_round_up(MaxSegmentSize / Segment.Resolution);
				Segment.SegmentLength[0][SegmentNr] = get_uint(NrOfBits); // Read Scaled_Length (Table 10.7)
				if ((Segment.Resolution * 8 * Segment.SegmentLength[0][SegmentNr] < MinSegmentLength) || (Segment.Resolution * 8 * Segment.SegmentLength[0][SegmentNr] > MaxFrameLen * 8 - DefinedBits - MinSegmentLength)) {
					log_printf("ERROR: Invalid segment length");
					return;
				}
				DefinedBits += Segment.Resolution * 8 * Segment.SegmentLength[0][SegmentNr];
				MaxSegmentSize -= Segment.Resolution * Segment.SegmentLength[0][SegmentNr];
				SegmentNr++;
				EndOfChannelSegment = get_bit(); // Read End_Of_Channel_Segm (Table 10.7)
			}
			Segment.NrOfSegments[0] = SegmentNr + 1;
			Segment.SegmentLength[0][SegmentNr] = 0;
			for (ChNr = 1; ChNr < NrOfChannels; ChNr++) {
				Segment.NrOfSegments[ChNr] = Segment.NrOfSegments[0];
				for (SegmentNr = 0; SegmentNr < Segment.NrOfSegments[0]; SegmentNr++) {
					Segment.SegmentLength[ChNr][SegmentNr] = Segment.SegmentLength[0][SegmentNr];
				}
			}
		}
		else {
			while (ChNr < NrOfChannels) {
				if (SegmentNr >= MaxNrOfSegments) {
					log_printf("ERROR: Too many segments for this channel");
					return;
				}
				EndOfChannelSegment = get_bit(); // Read End_Of_Channel_Segm (Table 10.7)
				if (!EndOfChannelSegment) {
					if (!ResolutionRead) {
						NrOfBits = log2_round_up(MaxFrameLen - MinSegmentLength / 8);
						Segment.Resolution = get_uint(NrOfBits); // Read Resolution (Table 10.7)
						if ((Segment.Resolution == 0) || (Segment.Resolution > MaxFrameLen - MinSegmentLength / 8)) {
							log_printf("ERROR: Invalid segment resolution");
							return;
						}
						ResolutionRead = true;
					}
					NrOfBits = log2_round_up(MaxSegmentSize / Segment.Resolution);
					Segment.SegmentLength[ChNr][SegmentNr] = get_uint(NrOfBits); // Read Scaled_Length (Table 10.7)
					if ((Segment.Resolution * 8 * Segment.SegmentLength[ChNr][SegmentNr] < MinSegmentLength) || (Segment.Resolution * 8 * Segment.SegmentLength[ChNr][SegmentNr] > MaxFrameLen * 8 - DefinedBits - MinSegmentLength)) {
						log_printf("ERROR: Invalid segment length");
						return;
					}
					DefinedBits += Segment.Resolution * 8 * Segment.SegmentLength[ChNr][SegmentNr];
					MaxSegmentSize -= Segment.Resolution * Segment.SegmentLength[ChNr][SegmentNr];
					SegmentNr++;
				}
				else {
					Segment.NrOfSegments[ChNr] = SegmentNr + 1;
					Segment.SegmentLength[ChNr][SegmentNr] = 0;
					SegmentNr = 0;
					DefinedBits = 0;
					MaxSegmentSize = MaxFrameLen - MinSegmentLength / 8;
					ChNr++;
				}
			}
		}
		if (!ResolutionRead) {
			Segment.Resolution = 1;
		}
	}

	// Copy segmentation for Filters and Ptables (Table 10.5)
	void copy_table_segmentation() {
		PSegment.Resolution = FSegment.Resolution;
		PSameSegAllCh = true;
		for (auto ChNr = 0u; ChNr < NrOfChannels; ChNr++) {
			PSegment.NrOfSegments[ChNr] = FSegment.NrOfSegments[ChNr];
			if (PSegment.NrOfSegments[ChNr] > MAXNROF_PSEGS) {
				log_printf("ERROR: Too many segments");
				return;
			}
			if (PSegment.NrOfSegments[ChNr] != PSegment.NrOfSegments[0]) {
				PSameSegAllCh = false;
			}
			for (auto SegmentNr = 0u; SegmentNr < FSegment.NrOfSegments[ChNr]; SegmentNr++) {
				PSegment.SegmentLength[ChNr][SegmentNr] = FSegment.SegmentLength[ChNr][SegmentNr];
				if ((PSegment.SegmentLength[ChNr][SegmentNr] != 0) && (PSegment.Resolution * 8 * PSegment.SegmentLength[ChNr][SegmentNr] < MIN_PSEG_LEN)) {
					log_printf("ERROR: Invalid segment length");
					return;
				}
				if (PSegment.SegmentLength[ChNr][SegmentNr] != PSegment.SegmentLength[0][SegmentNr]) {
					PSameSegAllCh = false;
				}
			}
		}
	}

	// Read segmentation for Filters and Ptables (Table 10.5)
	void read_segmentation() {
		PSameSegAsF = get_bit(); // Read Same_Segmentation (Table 10.5)
		read_table_segmentation(MAXNROF_FSEGS, MIN_FSEG_LEN, FSegment, FSameSegAllCh);
		if (PSameSegAsF) {
			copy_table_segmentation();
		}
		else {
			read_table_segmentation(MAXNROF_PSEGS, MIN_PSEG_LEN, PSegment, PSameSegAllCh);
		}
	}

	// Read mapping for Filters or Ptables (Table 10.8)
	void read_table_mapping(unsigned int MaxNrOfTables, segment_t& S, unsigned int& NrOfTables, bool& SameMapAllCh) {
		unsigned int CountTables = 1;
		unsigned int NrOfBits = 1;
		S.Table4Segment[0][0] = 0;
		SameMapAllCh = get_bit(); // Read Same_Maps_For_All_Channels (Table 10.9)
		if (SameMapAllCh) {
			for (auto SegmentNr = 1u; SegmentNr < S.NrOfSegments[0]; SegmentNr++) {
				NrOfBits = log2_round_up(CountTables);
				S.Table4Segment[0][SegmentNr] = get_uint(NrOfBits); // Read Element (Table 10.10)
				if (S.Table4Segment[0][SegmentNr] == CountTables) {
					CountTables++;
				}
				else if (S.Table4Segment[0][SegmentNr] > CountTables) {
					log_printf("ERROR: Invalid table number for segment");
					return;
				}
			}
			for (auto ChNr = 1u; ChNr < NrOfChannels; ChNr++) {
				if (S.NrOfSegments[ChNr] != S.NrOfSegments[0]) {
					log_printf("ERROR: Mapping can not be the same for all channels");
					return;
				}
				for (auto SegmentNr = 0u; SegmentNr < S.NrOfSegments[0]; SegmentNr++) {
					S.Table4Segment[ChNr][SegmentNr] = S.Table4Segment[0][SegmentNr];
				}
			}
		}
		else {
			for (auto ChNr = 0u; ChNr < NrOfChannels; ChNr++) {
				for (auto SegmentNr = 0u; SegmentNr < S.NrOfSegments[ChNr]; SegmentNr++) {
					if ((ChNr != 0) || (SegmentNr != 0)) {
						NrOfBits = log2_round_up(CountTables);
						S.Table4Segment[ChNr][SegmentNr] = get_uint(NrOfBits); // Read Element (Table 10.10)
						if (S.Table4Segment[ChNr][SegmentNr] == CountTables) {
							CountTables++;
						}
						else if (S.Table4Segment[ChNr][SegmentNr] > CountTables) {
							log_printf("ERROR: Invalid table number for segment");
							return;
						}
					}
				}
			}
		}
		if (CountTables > MaxNrOfTables) {
			log_printf("ERROR: Too many tables for this frame");
			return;
		}
		NrOfTables = CountTables;
	}

	// Copy mapping for Ptables from Filter
	void copy_table_mapping() {
		PSameMapAllCh = true;
		for (auto ChNr = 0u; ChNr < NrOfChannels; ChNr++) {
			if (PSegment.NrOfSegments[ChNr] == FSegment.NrOfSegments[ChNr]) {
				for (auto SegmentNr = 0u; SegmentNr < FSegment.NrOfSegments[ChNr]; SegmentNr++) {
					PSegment.Table4Segment[ChNr][SegmentNr] = FSegment.Table4Segment[ChNr][SegmentNr];
					if (PSegment.Table4Segment[ChNr][SegmentNr] != PSegment.Table4Segment[0][SegmentNr]) {
						PSameMapAllCh = false;
					}
				}
			}
			else {
				log_printf("ERROR: Not the same number of segments for Filters and Ptables");
				return;
			}
		}
		NrOfPtables = NrOfFilters;
		if (NrOfPtables > MaxNrOfPtables) {
			log_printf("ERROR: Too many tables for this frame");
			return;
		}
	}

	// Read mapping (which channel uses which Filter/Ptable)
	void read_mapping() {
		PSameMapAsF = get_bit(); // Read Same_Mapping (Table 10.8)
		read_table_mapping(MaxNrOfFilters, FSegment, NrOfFilters, FSameMapAllCh);
		if (PSameMapAsF) {
			copy_table_mapping();
		}
		else {
			read_table_mapping(MaxNrOfPtables, PSegment, NrOfPtables, PSameMapAllCh);
		}
		for (auto ChNr = 0u; ChNr < NrOfChannels; ChNr++) {
			HalfProb[ChNr] = get_bit();
		}
	}

	// Read Filter data from the DST stream (Table 10.13):
	// - which channel uses which Filter
	// - for each filter:
	//   - prediction order
	//   - coefficients
	void read_filter_coef_sets(ft_t& CF) {
		// Read the filter parameters
		for (auto FilterNr = 0u; FilterNr < NrOfFilters; FilterNr++) {
			PredOrder[FilterNr] = get_uint(SIZE_CODEDPREDORDER); // Read Coded_Pred_Order (Table 10.13)
			PredOrder[FilterNr]++;
			CF.Coded[FilterNr] = get_bit(); // Read Coded_Filter_Coef_Set (Table 10.13)
			if (!CF.Coded[FilterNr]) {
				CF.BestMethod[FilterNr] = (unsigned int)-1;
				for (auto CoefNr = 0u; CoefNr < PredOrder[FilterNr]; CoefNr++) {
					ICoefA[FilterNr][CoefNr] = (int16_t)get_sint(SIZE_PREDCOEF); // Read Coef (Table 10.13)
				}
			}
			else {
				CF.BestMethod[FilterNr] = get_uint(SIZE_RICEMETHOD); // Read CC_Method (Table 10.13)
				auto bestmethod = CF.BestMethod[FilterNr];
				if (CF.CPredOrder[bestmethod] >= PredOrder[FilterNr]) {
					log_printf("ERROR: Invalid coefficient coding method");
					return;
				}
				for (auto CoefNr = 0u; CoefNr < CF.CPredOrder[bestmethod]; CoefNr++) {
					ICoefA[FilterNr][CoefNr] = (int16_t)get_sint(SIZE_PREDCOEF); // Read Coef (Table 10.13)
				}
				CF.m[FilterNr][bestmethod] = get_uint(SIZE_RICEM); // Read CCM (Table 10.13)
				for (auto CoefNr = CF.CPredOrder[bestmethod]; CoefNr < PredOrder[FilterNr]; CoefNr++) {
					int x = 0;
					int c;
					for (auto TapNr = 0u; TapNr < CF.CPredOrder[bestmethod]; TapNr++) {
						x += CF.CPredCoef[bestmethod][TapNr] * ICoefA[FilterNr][CoefNr - TapNr - 1];
					}
					if (x >= 0) {
						c = rice_decode(CF.m[FilterNr][bestmethod]) - (x + 4) / 8;
					}
					else {
						c = rice_decode(CF.m[FilterNr][bestmethod]) + (-x + 3) / 8;
					}
					if ((c < -(1 << (SIZE_PREDCOEF - 1))) || (c >= (1 << (SIZE_PREDCOEF - 1)))) {
						log_printf("ERROR: filter coefficient out of range");
						return;
					}
					else {
						ICoefA[FilterNr][CoefNr] = (int16_t)c;
					}
				}
			}
		}
		for (auto ChNr = 0u; ChNr < NrOfChannels; ChNr++) {
			NrOfHalfBits[ChNr] = PredOrder[FSegment.Table4Segment[ChNr][0]];
		}
	}

	// Read Ptable data from the DST stream (Table 10.14):
	// - which channel uses which Ptable
	// - for each Ptable all entries
	void read_probability_tables(pt_t& CP, vector<array<unsigned int, AC_HISMAX>>& P_one) {
		// Read the data of probability tables (table entries)
		for (auto PtableNr = 0u; PtableNr < NrOfPtables; PtableNr++) {
			PtableLen[PtableNr] = get_uint(AC_HISBITS); // Read Coded_Ptable_Len (Table 10.14)
			PtableLen[PtableNr]++;
			if (PtableLen[PtableNr] > 1) {
				CP.Coded[PtableNr] = get_bit(); // Read Coded_Ptable (Table 10.14)
				if (!CP.Coded[PtableNr]) {
					CP.BestMethod[PtableNr] = (unsigned int)-1;
					for (auto EntryNr = 0u; EntryNr < PtableLen[PtableNr]; EntryNr++) {
						P_one[PtableNr][EntryNr] = get_uint(AC_BITS - 1); // Read Coded_P_one (Table 10.14)
						P_one[PtableNr][EntryNr]++;
					}
				}
				else {
					CP.BestMethod[PtableNr] = get_uint(SIZE_RICEMETHOD); // Read PC_Method (Table 10.14)
					auto bestmethod = CP.BestMethod[PtableNr];
					if (CP.CPredOrder[bestmethod] >= PtableLen[PtableNr]) {
						log_printf("ERROR: Invalid Ptable coding method");
						return;
					}
					for (auto EntryNr = 0u; EntryNr < CP.CPredOrder[bestmethod]; EntryNr++) {
						P_one[PtableNr][EntryNr] = get_uint(AC_BITS - 1); // Read Coded_P_one (Table 10.14)
						P_one[PtableNr][EntryNr]++;
					}
					CP.m[PtableNr][bestmethod] = get_uint(SIZE_RICEM); // Read PCM (Table 10.14)
					for (auto EntryNr = CP.CPredOrder[bestmethod]; EntryNr < PtableLen[PtableNr]; EntryNr++) {
						int x = 0;
						int c;
						for (auto TapNr = 0u; TapNr < CP.CPredOrder[bestmethod]; TapNr++) {
							x += CP.CPredCoef[bestmethod][TapNr] * P_one[PtableNr][EntryNr - TapNr - 1];
						}
						if (x >= 0) {
							c = rice_decode(CP.m[PtableNr][bestmethod]) - (x + 4) / 8;
						}
						else {
							c = rice_decode(CP.m[PtableNr][bestmethod]) + (-x + 3) / 8;
						}
						if ((c < 1) || (c > (1 << (AC_BITS - 1)))) {
							log_printf("ERROR: Ptable entry out of range");
							return;
						}
						else {
							P_one[PtableNr][EntryNr] = (unsigned int)c;
						}
					}
				}
			}
			else {
				P_one[PtableNr][0] = 128;
				CP.BestMethod[PtableNr] = (unsigned int)-1;
			}
		}
	}

	// Read arithmetic coded data from the DST stream (Table 10.12):
	// - length of the arithmetic code
	// - all bits of the arithmetic code
	void read_arithmetic_coded_data(uint8_t* AData) {
		auto ADataLen = CalcNrOfBits - get_offset();
		for (auto j = 0u; j < (ADataLen >> 3); j++) {
			AData[j] = (uint8_t)get_uint(8); // Read A_Data (Table 10.12)
		}
		uint8_t Val = 0;
		for (auto j = ADataLen & ~7; j < ADataLen; j++) {
			uint8_t v = get_bit(); // Read A_Data (Table 10.12)
			Val |= (uint8_t)(v << (7 - (j & 7)));
			if (j == ADataLen - 1) {
				AData[j >> 3] = Val;
				Val = 0;
			}
		}
	}

};

}

#endif


