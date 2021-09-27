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

enum class ct_e { Filter, Ptable };

constexpr int MAXFILTERBITS = MAXPREDORDER * SIZE_PREDCOEF;
constexpr int MAXPTABLEBITS = AC_HISMAX * AC_BITS;

template<ct_e ct_type>
class ct_t {
protected:
	static constexpr int ct_size = (ct_type == ct_e::Filter) ? MAXPREDORDER : AC_HISMAX;
	static constexpr int ct_bits = (ct_type == ct_e::Filter) ? MAXFILTERBITS : MAXPTABLEBITS;
public:
	unsigned int NrOfTables;                         // Number of coded tables
	unsigned int StreamBits;                         // Number of bits all filters use in the stream
	unsigned int CPredOrder[NROFFRICEMETHODS];       // Code_PredOrder[Method]
	int CPredCoef[NROFPRICEMETHODS][MAXCPREDORDER];  // Code_PredCoef[Method][CoefNr]
	vector<bool> Coded;                              // DST encode coefs/entries of Fir/PtabNr
	vector<unsigned int> BestMethod;                 // BestMethod[Fir/PtabNr]
	vector<array<unsigned int, NROFFRICEMETHODS>> m; // m[Fir/PtabNr][Method]
	vector<unsigned int> DataLenData;                // Fir/PtabDataLength[Fir/PtabNr]
	vector<array<int, ct_size>> data;                // Fir/PtabData[FirNr][Index]
public:
	ct_t() {
		if constexpr(ct_type == ct_e::Filter) {
			CPredOrder[0] = 1;
			CPredCoef[0][0] = -8;
			CPredOrder[1] = 2;
			CPredCoef[1][0] = -16;
			CPredCoef[1][1] = 8;
			CPredOrder[2] = 3;
			CPredCoef[2][0] = -9;
			CPredCoef[2][1] = -5;
			CPredCoef[2][2] = 6;
			if constexpr (NROFFRICEMETHODS == 4) {
				CPredOrder[3] = 1;
				CPredCoef[3][0] = 8;
			}
		}
		if constexpr(ct_type == ct_e::Ptable) {
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
	void init(unsigned int tables) {
		NrOfTables = tables;
		Coded.resize(tables);
		BestMethod.resize(tables);
		m.resize(tables);
		DataLenData.resize(tables);
		data.resize(tables);
	}
};

typedef ct_t<ct_e::Filter> ft_t;
typedef ct_t<ct_e::Ptable> pt_t;

}

#endif
