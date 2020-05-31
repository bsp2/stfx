// ----
// ---- file   : fx_example.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : plugin entry point for tksampler voice plugin FX example
// ----
// ---- created: 16May2020
// ---- changed: 17May2020, 18May2020, 19May2020, 20May2020, 21May2020, 24May2020, 25May2020
// ----          26May2020, 31May2020
// ----
// ----
// ----

#include "../../../plugin.h"
#include "fx_example.h"


ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init(unsigned int _pluginIdx) {
   switch(_pluginIdx)
   {
      default:
         break;

      case  0u: return ringmod_init          ();
      case  1u: return ws_tanh_init          ();
      case  2u: return ws_tanh_asym_init     ();
      case  3u: return bitcrusher_init       ();
      case  4u: return ws_exp_init           ();
      case  5u: return ws_fm_init            ();
      case  6u: return resample_nearest_init ();
      case  7u: return resample_linear_init  ();
      case  8u: return biquad_lpf_1_init     ();
      case  9u: return biquad_lpf_2_init     ();
      case 10u: return biquad_lpf_3_init     ();
      case 11u: return biquad_lpf_4_init     ();
      case 12u: return biquad_hpf_1_init     ();
      case 13u: return biquad_hpf_2_init     ();
      case 14u: return biquad_hpf_3_init     ();
      case 15u: return biquad_hpf_4_init     ();
      case 16u: return biquad_bpf_1_init     ();
      case 17u: return biquad_bpf_2_init     ();
      case 18u: return biquad_bpf_3_init     ();
      case 19u: return biquad_bpf_4_init     ();
      case 20u: return biquad_brf_1_init     ();
      case 21u: return biquad_brf_2_init     ();
      case 22u: return biquad_brf_3_init     ();
      case 23u: return biquad_brf_4_init     ();
      case 24u: return biquad_peq_1_init     ();
      case 25u: return biquad_peq_2_init     ();
      case 26u: return biquad_peq_3_init     ();
      case 27u: return biquad_peq_4_init     ();
      case 28u: return biquad_lsh_1_init     ();
      case 29u: return biquad_lsh_2_init     ();
      case 30u: return biquad_lsh_3_init     ();
      case 31u: return biquad_lsh_4_init     ();
      case 32u: return biquad_hsh_1_init     ();
      case 33u: return biquad_hsh_2_init     ();
      case 34u: return biquad_hsh_3_init     ();
      case 35u: return biquad_hsh_4_init     ();
      case 36u: return biquad_vsf_1_init     ();
      case 37u: return biquad_vsf_2_init     ();
      case 38u: return biquad_vsf_3_init     ();
      case 39u: return biquad_vsf_4_init     ();
      case 40u: return amp_init              ();
      case 41u: return pan_init              ();
      case 42u: return ws_smoothstep_init    ();
      case 43u: return dly_1_init            ();
      case 44u: return dly_2_init            ();
      case 45u: return dly_flt_2_init        ();
      case 46u: return bitflipper_init       ();
      case 47u: return clip_init             ();
      case 48u: return ringmul_init          ();
   }

   return NULL;
}
