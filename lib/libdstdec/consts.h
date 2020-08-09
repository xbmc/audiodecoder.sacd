/*
* Direct Stream Transfer (DST) codec
* ISO/IEC 14496-3 Part 3 Subpart 10: Technical description of lossless coding of oversampled audio
*/

#ifndef CONSTS_H
#define CONSTS_H

namespace dst {

// Prediction
constexpr int SIZE_CODEDPREDORDER = 7; // Number of bits in the stream for representing	the CodedPredOrder in each frame
constexpr int MAXPREDORDER = 1 << SIZE_CODEDPREDORDER; // Maximum prediction filter order

// Probability tables
constexpr int SIZE_CODEDPTABLELEN = 6; // Number bits for p-table length
constexpr int MAXPTABLELEN = 1 << SIZE_CODEDPTABLELEN; // Maximum length of p-tables

constexpr int SIZE_PREDCOEF = 9; // Number of bits in the stream for representing each filter coefficient in each frame

// Arithmetic coding
constexpr int AC_BITS    = 8; // Number of bits and maximum level for coding the probability
constexpr int AC_PROBS   = 1 << AC_BITS;
constexpr int AC_HISBITS = 6; // Number of entries in the histogram
constexpr int AC_HISMAX  = 1 << AC_HISBITS;
constexpr int AC_QSTEP   = SIZE_PREDCOEF - AC_HISBITS; // Quantization step for histogram

// Rice coding of filter coefficients and probability tables
constexpr int NROFFRICEMETHODS = 3; // Number of different Pred. Methods for filters	used in combination with Rice coding
constexpr int NROFPRICEMETHODS = 3; // Number of different Pred. Methods for Ptables	used in combination with Rice coding
constexpr int MAXCPREDORDER    = 3; // max pred.order for prediction of filter coefs / Ptables entries
constexpr int SIZE_RICEMETHOD  = 2; // nr of bits in stream for indicating method
constexpr int SIZE_RICEM       = 3; // nr of bits in stream for indicating m
constexpr int MAX_RICE_M_F     = 6; // Max. value of m for filters
constexpr int MAX_RICE_M_P     = 4; // Max. value of m for Ptables

// Segmentation
constexpr int MAXNROF_FSEGS = 4;   // max nr of segments per channel for filters
constexpr int MAXNROF_PSEGS = 8;   // max nr of segments per channel for Ptables
constexpr int MIN_FSEG_LEN = 1024; // min segment length in bits of filters
constexpr int MIN_PSEG_LEN = 32;   // min segment length in bits of Ptables

constexpr int MAXNROF_SEGS = 8; // max nr of segments per channel for filters or Ptables

}

#endif
