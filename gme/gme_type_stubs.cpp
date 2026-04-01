/* Stub type definitions for unused emulator types.
 * Only NSF is used in nsf2fmsq. Other types are declared extern in gme.h
 * but never defined since we don't compile those emulators. */

#include "gme.h"

#ifndef USE_GME_AY
gme_type_t const gme_ay_type = nullptr;
#endif
#ifndef USE_GME_GBS
gme_type_t const gme_gbs_type = nullptr;
#endif
#ifndef USE_GME_GYM
gme_type_t const gme_gym_type = nullptr;
#endif
#ifndef USE_GME_HES
gme_type_t const gme_hes_type = nullptr;
#endif
#ifndef USE_GME_KSS
gme_type_t const gme_kss_type = nullptr;
#endif
#ifndef USE_GME_NSFE
gme_type_t const gme_nsfe_type = nullptr;
#endif
#ifndef USE_GME_SAP
gme_type_t const gme_sap_type = nullptr;
#endif
#ifndef USE_GME_SPC
gme_type_t const gme_spc_type = nullptr;
#endif
#ifndef USE_GME_VGM
gme_type_t const gme_vgm_type = nullptr;
gme_type_t const gme_vgz_type = nullptr;
#endif
