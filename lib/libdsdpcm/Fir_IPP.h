/*
* SACD Decoder plugin
* Copyright (c) 2011-2015 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
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

#pragma once

#include <ipp.h>

extern bool g_ipp_initialized;

typedef void IppsFIRSpec;

class Fir_IPP {
public:
	static inline IppStatus Sum(const Ipp32f* pSrc, int len, Ipp32f* pSum) {
		return ippsSum_32f(pSrc, len, pSum, ippAlgHintNone);
	}
	static inline IppStatus Sum(const Ipp64f* pSrc, int len, Ipp64f* pSum) {
		return ippsSum_64f(pSrc, len, pSum);
	}
	static inline IppStatus FIRMRInit(const Ipp32f* pTaps, int tapsLen, int upFactor, int upPhase, int downFactor, int downPhase, IppsFIRSpec* pSpec) {
		return ippsFIRMRInit_32f(pTaps, tapsLen, upFactor, upPhase, downFactor, downPhase, reinterpret_cast<IppsFIRSpec_32f*>(pSpec));
	}
	static inline IppStatus FIRMR(const Ipp32f* pSrc, Ipp32f* pDst, int numIters, IppsFIRSpec* pSpec, const Ipp32f* pDlySrc, Ipp32f* pDlyDst, Ipp8u* pBuf) {
		return ippsFIRMR_32f(pSrc, pDst, numIters, reinterpret_cast<IppsFIRSpec_32f*>(pSpec), pDlySrc, pDlyDst, pBuf);
	}
	static inline IppStatus FIRMRInit(const Ipp64f* pTaps, int tapsLen, int upFactor, int upPhase, int downFactor, int downPhase, IppsFIRSpec* pSpec) {
		return ippsFIRMRInit_64f(pTaps, tapsLen, upFactor, upPhase, downFactor, downPhase, reinterpret_cast<IppsFIRSpec_64f*>(pSpec));
	}
	static inline IppStatus FIRMR(const Ipp64f* pSrc, Ipp64f* pDst, int numIters, IppsFIRSpec* pSpec, const Ipp64f* pDlySrc, Ipp64f* pDlyDst, Ipp8u* pBuf) {
		return ippsFIRMR_64f(pSrc, pDst, numIters, reinterpret_cast<IppsFIRSpec_64f*>(pSpec), pDlySrc, pDlyDst, pBuf);
	}
};
