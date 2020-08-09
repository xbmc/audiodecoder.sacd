#include "DSDPCMConverter.h"

template<typename real_t>
real_t DSDPCMConverter<real_t>::dsd_fir1_8_ctables[CTABLES(DSDFIR1_8_LENGTH)][256];

template<typename real_t>
real_t DSDPCMConverter<real_t>::dsd_fir1_16_ctables[CTABLES(DSDFIR1_16_LENGTH)][256];

template<typename real_t>
real_t DSDPCMConverter<real_t>::dsd_fir1_64_ctables[CTABLES(DSDFIR1_64_LENGTH)][256];

template<typename real_t>
real_t DSDPCMConverter<real_t>::pcm_fir2_2_coefs[PCMFIR2_2_LENGTH];

template<typename real_t>
real_t DSDPCMConverter<real_t>::pcm_fir3_2_coefs[PCMFIR3_2_LENGTH];
