// ----
// ---- file   : fx_example.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020-2021 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : plugin entry point for tksampler voice plugin FX example
// ----
// ---- created: 16May2020
// ---- changed: 17May2020, 18May2020, 19May2020, 20May2020, 21May2020, 24May2020, 25May2020
// ----          26May2020, 31May2020, 01Jun2020, 02Jun2020, 06Jun2020, 07Jun2020, 08Jun2020
// ----          09Jun2020, 08Feb2021, 11Feb2021, 23Feb2021, 14Apr2021, 02May2021, 31May2021
// ----          16Aug2021, 13Oct2021, 14Oct2021
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

      case  0u: return ringmod_init                  ();
      case  1u: return ws_tanh_init                  ();
      case  2u: return ws_tanh_asym_init             ();
      case  3u: return bitcrusher_init               ();
      case  4u: return ws_exp_init                   ();
      case  5u: return ws_fm_init                    ();
      case  6u: return resample_nearest_init         ();
      case  7u: return resample_linear_init          ();
      case  8u: return resample_tuned_init           ();
      case  9u: return biquad_lpf_1_init             ();
      case 10u: return biquad_lpf_2_init             ();
      case 11u: return biquad_lpf_3_init             ();
      case 12u: return biquad_lpf_4_init             ();
      case 13u: return biquad_hpf_1_init             ();
      case 14u: return biquad_hpf_2_init             ();
      case 15u: return biquad_hpf_3_init             ();
      case 16u: return biquad_hpf_4_init             ();
      case 17u: return biquad_bpf_1_init             ();
      case 18u: return biquad_bpf_2_init             ();
      case 19u: return biquad_bpf_3_init             ();
      case 20u: return biquad_bpf_4_init             ();
      case 21u: return biquad_brf_1_init             ();
      case 22u: return biquad_brf_2_init             ();
      case 23u: return biquad_brf_3_init             ();
      case 24u: return biquad_brf_4_init             ();
      case 25u: return biquad_peq_1_init             ();
      case 26u: return biquad_peq_2_init             ();
      case 27u: return biquad_peq_3_init             ();
      case 28u: return biquad_peq_4_init             ();
      case 29u: return biquad_lsh_1_init             ();
      case 30u: return biquad_lsh_2_init             ();
      case 31u: return biquad_lsh_3_init             ();
      case 32u: return biquad_lsh_4_init             ();
      case 33u: return biquad_hsh_1_init             ();
      case 34u: return biquad_hsh_2_init             ();
      case 35u: return biquad_hsh_3_init             ();
      case 36u: return biquad_hsh_4_init             ();
      case 37u: return biquad_vsf_1_init             ();
      case 38u: return biquad_vsf_2_init             ();
      case 39u: return biquad_vsf_3_init             ();
      case 40u: return biquad_vsf_4_init             ();
      case 41u: return amp_init                      ();
      case 42u: return pan_init                      ();
      case 43u: return ws_smoothstep_init            ();
      case 44u: return dly_1_init                    ();
      case 45u: return dly_2_init                    ();
      case 46u: return dly_flt_2_init                ();
      case 47u: return bitflipper_init               ();
      case 48u: return clip_init                     ();
      case 49u: return ringmul_init                  ();
      case 50u: return tuned_fb_init                 ();
      case 51u: return ladder_lpf_init               ();
      case 52u: return ladder_lpf_no_pan_init        ();
      case 53u: return ladder_lpf_x2_init            ();
      case 54u: return ladder_lpf_x2_no_pan_init     ();
      case 55u: return ladder_lpf_x3_init            ();
      case 56u: return ladder_lpf_x3_no_pan_init     ();
      case 57u: return ladder_lpf_x4_init            ();
      case 58u: return ladder_lpf_x4_no_pan_init     ();
      case 59u: return comp_rms_init                 ();
      case 60u: return ws_quintic_init               ();
      case 61u: return x_mix_init                    ();
      case 62u: return x_mul_init                    ();
      case 63u: return x_min_init                    ();
      case 64u: return x_max_init                    ();
      case 65u: return x_biquad_lpf_4_init           ();
      case 66u: return x_ladder_lpf_x4_init          ();
      case 67u: return x_mul_abs_init                ();
      case 68u: return abs_init                      ();
      case 69u: return rms_init                      ();
      case 70u: return ws_fold_wrap_init             ();
      case 71u: return ws_fold_sine_init             ();
      case 72u: return dly_1_fade_init               ();
      case 73u: return dly_2_fade_init               ();
      case 74u: return dly_flt_2_fade_init           ();
      case 75u: return ws_flex_init                  ();
      case 76u: return ws_flex_asym_init             ();
      case 77u: return ws_slew_init                  ();
      case 78u: return ws_slew_asym_init             ();
      case 79u: return boost_init                    ();
      case 80u: return eq3_init                      ();
      case 81u: return ws_slew_8bit_init             ();
      case 82u: return wave_multiplier_wrap_init     ();
      case 83u: return wave_multiplier_reflect_init  ();
      case 84u: return wave_multiplier_delay_init    ();
      case 85u: return wave_multiplier_delay4_init   ();
      case 86u: return wave_multiplier_delay8_init   ();
      case 87u: return wave_multiplier_delay16_init  ();
      case 88u: return wave_multiplier_allpass4_init ();
      case 89u: return wave_multiplier_allpass8_init ();
      case 90u: return wave_multiplier_apdly4_init   ();
      case 91u: return wave_multiplier_apdly8_init   ();
   }

   return NULL;
}
