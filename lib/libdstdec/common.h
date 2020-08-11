/*
* Direct Stream Transfer (DST) codec
* ISO/IEC 14496-3 Part 3 Subpart 10: Technical description of lossless coding of oversampled audio
*/

#ifndef COMMON_H
#define COMMON_H

#include <kodi/General.h>
#include <stdint.h>

namespace
{

inline const char* kodiTranslateLogLevel(const AddonLog logLevel)
{
  switch (logLevel)
  {
    case ADDON_LOG_DEBUG:
      return "LOG_DEBUG:   ";
    case ADDON_LOG_INFO:
      return "LOG_INFO:    ";
    case ADDON_LOG_WARNING:
      return "LOG_WARNING: ";
    case ADDON_LOG_ERROR:
      return "LOG_ERROR:   ";
    case ADDON_LOG_FATAL:
      return "LOG_FATAL:   ";
    default:
      break;
  }
  return "LOG_UNKNOWN: ";
}

inline void kodiLog(const AddonLog logLevel, const char* format, ...)
{
  char buffer[16384];
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);

  kodi::Log(logLevel, buffer);
#ifdef DEBUG
  fprintf(stderr, "%s%s\n", kodiTranslateLogLevel(logLevel), buffer);
#endif
}

}

namespace dst {

const auto GET_BIT = [](auto base, auto index) {
	return (((unsigned char*)base)[index >> 3] >> (7 - (index & 7))) & 1;
};

const auto GET_NIBBLE = [](auto base, auto index) {
	return (((unsigned char*)base)[index >> 1] >> ((index & 1) << 2)) & 0x0f;
};

}

#endif
