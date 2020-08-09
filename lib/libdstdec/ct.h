/*
* Direct Stream Transfer (DST) codec
* ISO/IEC 14496-3 Part 3 Subpart 10: Technical description of lossless coding of oversampled audio
*/

#ifndef CT_H
#define CT_H

#include <stdint.h>
#include <array>
#include <vector>
#include "consts.h"

using std::array;
using std::vector;

namespace dst
{

const int MAXFTABLESIZE = MAXPREDORDER * SIZE_PREDCOEF;
const int MAXPTABLESIZE = AC_BITS * AC_HISMAX;

template<int table_size>
class ct_t {
public:
	int StreamBits;                                 // Number of bits all filters use in the stream
	int CPredOrder[NROFFRICEMETHODS];               // Code_PredOrder[Method]
	int CPredCoef[NROFPRICEMETHODS][MAXCPREDORDER]; // Code_PredCoef[Method][CoefNr]
	vector<bool> Coded;                             // DST encode coefs/entries of Fir/PtabNr
	vector<int> BestMethod;                         // BestMethod[Fir/PtabNr]
	vector<array<int, NROFFRICEMETHODS>> m;         // m[Fir/PtabNr][Method]
	vector<int> DataLenData;                        // Fir/PtabDataLength[Fir/PtabNr]
	vector<array<int, table_size>> data;            // Fir/PtabData[FirNr][Index]
public:
	ct_t() {
		if (table_size == MAXFTABLESIZE) {
			CPredOrder[0] = 1;
			CPredCoef[0][0] = -8;
			CPredOrder[1] = 2;
			CPredCoef[1][0] = -16;
			CPredCoef[1][1] = 8;
			CPredOrder[2] = 3;
			CPredCoef[2][0] = -9;
			CPredCoef[2][1] = -5;
			CPredCoef[2][2] = 6;
#if NROFFRICEMETHODS == 4
			CPredOrder[3] = 1;
			CPredCoef[3][0] = 8;
#endif
		}
		if (table_size == MAXPTABLESIZE) {
			CPredOrder[0] = 1;
			CPredCoef[0][0] = -8;
			CPredOrder[1] = 2;
			CPredCoef[1][0] = -16;
			CPredCoef[1][1] = 8;
			CPredOrder[2] = 3;
			CPredCoef[2][0] = -24;
			CPredCoef[2][1] = 24;
			CPredCoef[2][2] = -8;
		}
	}
	void init(int tables) {
		Coded.resize(tables);
		BestMethod.resize(tables);
		m.resize(tables);
		DataLenData.resize(tables);
		data.resize(tables);
	}
};

typedef ct_t<MAXFTABLESIZE> ft_t;
typedef ct_t<MAXPTABLESIZE> pt_t;

}

#endif
