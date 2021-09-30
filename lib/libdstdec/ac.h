/*
* Direct Stream Transfer (DST) codec
* ISO/IEC 14496-3 Part 3 Subpart 10: Technical description of lossless coding of oversampled audio
*/

#ifndef AC_H
#define AC_H

#include <stdint.h>
#include "common.h"

namespace dst {

constexpr int PBITS = AC_BITS;       // number of bits for Probabilities
constexpr int NBITS = 4;             // number of overhead bits: must be at least 2!
                                     // maximum "variable shift length" is (NBITS-1)
constexpr int PSUM  = 1 << PBITS;
constexpr int ABITS = PBITS + NBITS; // must be at least PBITS+2
constexpr int MB    = 0;             // if (MB) print max buffer use
constexpr int ONE   = 1 << ABITS;
constexpr int HALF  = 1 << (ABITS - 1);

class ac_t {
	unsigned int Init;
	unsigned int C;
	unsigned int A;
	int cbptr;
public:

	unsigned int getPtableIndex(int PredicVal, int PtableLen) {
		int j;
		j = (PredicVal > 0 ? PredicVal : -PredicVal) >> AC_QSTEP;
		if (j >= PtableLen) {
			j = PtableLen - 1;
		}
		return (unsigned int)j;
	}

	void decodeBit(uint8_t& b, int p, uint8_t* cb, int fs, int flush) {
		unsigned int ap;
		unsigned int h;
		if (Init == 1) {
			Init = 0;
			A = ONE - 1;
			C = 0;
			for (cbptr = 1; cbptr <= ABITS; cbptr++) {
				C <<= 1;
				if (cbptr < fs) {
					C |= cb[cbptr];
				}
			}
		}
		if (flush == 0) {
			// approximate (A * p) with "partial rounding"
			ap = ((A >> PBITS) | ((A >> (PBITS - 1)) & 1))* p;
			h = A - ap;
			if (C >= h) {
				b = 0;
				C -= h;
				A = ap;
			}
			else {
				b = 1;
				A = h;
			}
			while (A < HALF) {
				A <<= 1;
				// Use new flushing technique; insert zero in LSB of C if reading past the end of the arithmetic code
				C <<= 1;
				if (cbptr < fs) {
					C |= cb[cbptr];
				}
				cbptr++;
			}
		}
		else {
			Init = 1;
			if (cbptr < fs - 7) {
				b = 0;
			}
			else {
				b = 1;
				while ((cbptr < fs) && (b == 1)) {
					if (cb[cbptr] != 0) {
						b = 1;
					}
					cbptr++;
				}
			}
		}
	}

	void decodeBit_Init(uint8_t* cb, int fs) {
		Init = 0;
		A = ONE - 1;
		C = 0;
		for (cbptr = 1; cbptr <= ABITS; cbptr++) {
			C <<= 1;
			if (cbptr < fs) {
				C |= GET_BIT(cb, cbptr);
			}
		}
	}

	void decodeBit_Decode(uint8_t* b, int p, uint8_t* cb, int fs) {
		unsigned int ap;
		unsigned int h;
		// approximate (A * p) with "partial rounding"
		ap = ((A >> PBITS) | ((A >> (PBITS - 1)) & 1))* p;
		h = A - ap;
		if (C >= h) {
			*b = 0;
			C -= h;
			A = ap;
		}
		else {
			*b = 1;
			A = h;
		}
		while (A < HALF) {
			A <<= 1;
			// Use new flushing technique; insert zero in LSB of C if reading past the end of the arithmetic code
			C <<= 1;
			if (cbptr < fs) {
				C |= GET_BIT(cb, cbptr);
			}
			cbptr++;
		}
	}

	void decodeBit_Flush(uint8_t* b, int p, uint8_t* cb, int fs) {
		(void)p;
		Init = 1;
		if (cbptr < fs - 7) {
			*b = 0;
		}
		else {
			*b = 1;
			while ((cbptr < fs) && (*b == 1)) {
				if (GET_BIT(cb, cbptr) != 0) {
					*b = 1;
				}
				cbptr++;
			}
		}
	}

};

}

#endif
