// ----
// ---- file   : schroederverb.cpp
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2019-2024 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : simple stereo reverb.
// ----           - all voices share the same instance (=> don't alloc more than 1 voice)
// ----           - uses more than 8 (shared) parameters (55)
// ----
// ---- created: 28Jun2019
// ---- changed: 07Jul2019, 08Jul2019, 10Jul2019, 12Jul2019, 20Apr2023, 21Jan2024
// ----
// ----
// ----

/// large table (2048k). comment out to skip this.
// #define USE_PRIME_LOCK  defined

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <new>

#include "../../../plugin.h"

typedef float          sF32;
typedef double         sF64;
typedef int            sBool;
typedef int            sSI;
typedef unsigned int   sUI;
#define YAC_TRUE  1
#define YAC_FALSE 0

#include "sway.h"
#include "allpass_ff_dly.h"
#include "comb.h"
#include "delay.h"

#ifdef USE_PRIME_LOCK
#include "prime.h"
#endif // USE_PRIME_LOCK

struct SchroederCh {
   AllPassFFDly ap_1;
   AllPassFFDly ap_2;
   AllPassFFDly ap_fb;
   Comb      cb_1;
   Comb      cb_2;
   Comb      cb_3;
   Delay     dly_x;

   sF32 last_in;
   sF32 last_f;
   sF32 last_out;

   void init(sF32 _sampleRate) {

      ap_1 .setSampleRate(_sampleRate);
      ap_2 .setSampleRate(_sampleRate);
      ap_fb.setSampleRate(_sampleRate);

      ap_1 .setB( 0.3235f);
      ap_1 .setC(-0.3235f);
      ap_2 .setB(0.63147f);
      ap_2 .setC(-0.63147f);
      ap_fb.setB(0.234234f);
      ap_fb.setC(-0.3234234f);

      sF32  cbFbAmt    = 0.87f;
      sF32  noteShift  = 0.0f;
      sF32  noteScl    = 0.5f;

      cb_1.setSampleRate(_sampleRate);
      cb_1.setNote( (noteShift + 12*3 + 0.0f) * noteScl );
      cb_1.setFbAmt(cbFbAmt);

      cb_2.setSampleRate(_sampleRate);
      cb_2.setNote( (noteShift + 12*2 + 3.5f) * noteScl );
      cb_2.setFbAmt(cbFbAmt);

      cb_3.setSampleRate(_sampleRate);
      cb_3.setNote( (noteShift + 12*4 + 7.35f) * noteScl );
      cb_3.setFbAmt(cbFbAmt);

      dly_x.setSampleRate(_sampleRate);
      dly_x.setNote( (noteShift + 12*2 + 0.0f) * noteScl );
      dly_x.setFbAmt(cbFbAmt);

      cb_1.handleParamsUpdated();
      cb_2.handleParamsUpdated();
      cb_3.handleParamsUpdated();
      dly_x.handleParamsUpdated();

      last_in  = 0.0f;
      last_f   = 0.0f;
      last_out = 0.0f;

      dly_x.setFbAmt(0.3f);
      dly_x.setFbDcy(0.2f);
   }

   void updateSampleRate(sF32 _rate) {
      ap_1 .setSampleRate(_rate);
      ap_2 .setSampleRate(_rate);
      ap_fb.setSampleRate(_rate);
      cb_1 .setSampleRate(_rate);
      cb_2 .setSampleRate(_rate);
      cb_3 .setSampleRate(_rate);
      dly_x.setSampleRate(_rate);

      cb_1 .handleParamsUpdated();
      cb_2 .handleParamsUpdated();
      cb_3 .handleParamsUpdated();
      dly_x.handleParamsUpdated();
   }

   void setDlyLerpSpdNorm(sF32 _spd) {
      cb_1 .setDlyLerpSpdNorm(_spd);
      cb_2 .setDlyLerpSpdNorm(_spd);
      cb_3 .setDlyLerpSpdNorm(_spd);
      dly_x.setDlyLerpSpdNorm(_spd);
      ap_1 .setDlyLerpSpdNorm(_spd);
      ap_2 .setDlyLerpSpdNorm(_spd);
      ap_fb.setDlyLerpSpdNorm(_spd);
   }

   void setPrimeTbl(const sUI *_primeTbl) {
      cb_1 .setPrimeTbl(_primeTbl);
      cb_2 .setPrimeTbl(_primeTbl);
      cb_3 .setPrimeTbl(_primeTbl);
      dly_x.setPrimeTbl(_primeTbl);
   }
};


#define PARAM_IN_AMT       0  //          input level (ap1 input scaling)
#define PARAM_IN_HPF       1  //          input high pass filter (pre ap1)
#define PARAM_WET_AMT      2  //          wet amount (0=dry 1=wet)
#define PARAM_WET_GAIN     3  //          wet gain (0..1 => 0..2)
#define PARAM_CB_MIX       4  // bipolar  comb 1..3 mix level (0..1 => -1..1)
#define PARAM_CLIP_LVL     5  //          clip output to -10..10 before wet gain and output level
#define PARAM_DLY_LSPD     6  //          lerp speed (when changing delay time)
#define PARAM_FBMOD        7  // bipolar. dly/comb feedback amount modulation
#define PARAM_FCBMOD       8  // bipolar. comb note freq mod (/256..*256)
#define PARAM_FAPMOD       9  // bipolar. allpass freq mod (/256..*256)
#define PARAM_FDLYMOD     10  // bipolar. dly_x freq/delaytime mod (/256..*256)
#define PARAM_FREQSCLL    11  //          dly/comb freq scale left (0..1)
#define PARAM_FREQSCLR    12  //          dly/comb freq scale right (0..1)
#define PARAM_CB1_NOTE    13  //          comb 1 note (0..1 => C-0..E-8)
#define PARAM_CB2_NOTE    14  //          comb 2 note (0..1 => C-0..E-8)
#define PARAM_CB3_NOTE    15  //          comb 3 note (0..1 => C-0..E-8)
#define PARAM_DLYX_NOTE   16  //          dlyx note (0..1 => C-0..E-8)
#define PARAM_CB1_FBAMT   17  //          comb 1 feedback amount (0..1)
#define PARAM_CB2_FBAMT   18  //          comb 2 feedback amount (0..1)
#define PARAM_CB3_FBAMT   19  //          comb 3 feedback amount (0..1)
#define PARAM_DLYX_FBAMT  20  //          dlyx feedback amount (0..1)
#define PARAM_AP1_B       21  // bipolar. allpass 1 "b" coefficient (0..1 => -1..1)
#define PARAM_AP1_C       22  // bipolar. allpass 1 "c" coefficient (0..1 => -1..1)
#define PARAM_AP1_NOTE    23  //          allpass 1 note freq (0..1 => C-0..E-8)
#define PARAM_AP2_B       24  // bipolar. allpass 2 "b" coefficient (0..1 => -1..1)
#define PARAM_AP2_C       25  // bipolar. allpass 2 "c" coefficient (0..1 => -1..1)
#define PARAM_AP2_NOTE    26  //          allpass 2 note freq (0..1 => C-0..E-8)
#define PARAM_APFB_B      27  // bipolar. allpass feedback "b" coefficient (0..1 => -1..1)
#define PARAM_APFB_C      28  // bipolar. allpass feedback "c" coefficient (0..1 => -1..1)
#define PARAM_APFB_NOTE   29  //          allpass feedback note freq (0..1 => C-0..E-8)
#define PARAM_CB1_FBDCY   30  // bipolar. comb 1 feedback decay (0..1 => -1..1)
#define PARAM_CB2_FBDCY   31  // bipolar. comb 2 feedback decay (0..1 => -1..1)
#define PARAM_CB3_FBDCY   32  // bipolar. comb 3 feedback decay (0..1 => -1..1)
#define PARAM_DLYX_FBDCY  33  // bipolar. dlyx feedback decay (0..1 => -1..1)
#define PARAM_X1_AMT      34  //          cross channel amount 1 (cur ap_fb input = cross last out * x1)
#define PARAM_X2_AMT      35  //          cross channel amount 2 (cur + (post ap_fb) cross dlyx * x2)
#define PARAM_X3_AMT      36  //          preserve last out amount (last_out * x3)
#define PARAM_X4_AMT      37  //          new last out amount (+ cur out * x4)
#define PARAM_SWAYCBF     38  //          comb 1..3 sway scale
#define PARAM_SWAYCBM     39  //          comb 1..3 sway min time
#define PARAM_SWAYCBX     40  //          comb 1..3 sway max time
#define PARAM_SWAYAPF     41  //          allpass 1+2 sway scale
#define PARAM_SWAYAPM     42  //          allpass 1+2 sway min time
#define PARAM_SWAYAPX     43  //          allpass 1+2 sway max time
#define PARAM_SWAYDLYF    44  //          dly_x sway scale
#define PARAM_SWAYDLYM    45  //          dly_x sway min time
#define PARAM_SWAYDLYX    46  //          dly_x sway max time
#define PARAM_SWAYFB      47  //          comb 1..3 feedback sway scale
#define PARAM_BUFSZCB     48  //          comb 1..3 delay line length (0..1 => 0..512k frames @48kHz)
#define PARAM_BUFSZDLY    49  //          dlyx delay line length (0..1 => 0..512k frames @48kHz)
#define PARAM_LINEAR      50  //          enable linear interpolation (0=off, >=0.5=on)
#define PARAM_PRIME       51  //          left ch comb 1 fb sway scale
#define PARAM_OUT_LPF     52  //          output box filter (0..1)
#define PARAM_CB_MIX_IN   53  // bipolar  pre-comb (ap out) mix out (0..1 => -1..1)
#define PARAM_OUT_LVL     54  //          output level (0..1)
#define NUM_PARAMS        55

static const char *loc_param_names[NUM_PARAMS] = {
   "In Lvl",    // PARAM_IN_AMT
   "In HPF",    // PARAM_IN_HPF
   "Dry/Wet",   // PARAM_WET_AMT
   "Wet Gain",  // PARAM_WET_GAIN
   "Cb Mix",    // PARAM_CB_MIX
   "Clip Lvl",  // PARAM_CLIP_LVL
   "Dly LSpd",  // PARAM_DLY_LSPD
   "FbMod",     // PARAM_FBMOD
   "FCbMod",    // PARAM_FCBMOD
   "FApMod",    // PARAM_FAPMOD
   "FDlyMod",   // PARAM_FDLYMOD
   "FreqSclL",  // PARAM_FREQSCLL
   "FreqSclR",  // PARAM_FREQSCLR
   "Cb1.Note",  // PARAM_CB1_NOTE
   "Cb2.Note",  // PARAM_CB2_NOTE
   "Cb3.Note",  // PARAM_CB3_NOTE
   "DlX.Note",  // PARAM_DLYX_NOTE
   "Cb1.Fb",    // PARAM_CB1_FBAMT
   "Cb2.Fb",    // PARAM_CB2_FBAMT
   "Cb3.Fb",    // PARAM_CB3_FBAMT
   "DlX.Fb",    // PARAM_DLYX_FBAMT
   "Ap1.B",     // PARAM_AP1_B
   "Ap1.C",     // PARAM_AP1_C
   "Ap1.Note",  // PARAM_AP1_NOTE
   "Ap2.B",     // PARAM_AP2_B
   "Ap2.C",     // PARAM_AP2_C
   "Ap2.Note",  // PARAM_AP2_NOTE
   "ApFB.B",    // PARAM_APFB_B
   "ApFB.C",    // PARAM_APFB_C
   "ApFB.Note", // PARAM_APFB_NOTE
   "Cb1.Dcy",   // PARAM_CB1_FBDCY
   "Cb2.Dcy",   // PARAM_CB2_FBDCY
   "Cb3.Dcy",   // PARAM_CB3_FBDCY
   "DlX.Dcy",   // PARAM_DLYX_FBDCY
   "DlX.X1",    // PARAM_X1_AMT
   "DlX.X2",    // PARAM_X2_AMT
   "DlX.X3",    // PARAM_X3_AMT
   "DlX.X4",    // PARAM_X4_AMT
   "SwayCbF",   // PARAM_SWAYCBF
   "SwayCbM",   // PARAM_SWAYCBM
   "SwayCbX",   // PARAM_SWAYCBX
   "SwayApF",   // PARAM_SWAYAPF
   "SwayApM",   // PARAM_SWAYAPM
   "SwayApX",   // PARAM_SWAYAPX
   "SwayDlyF",  // PARAM_SWAYDLYF
   "SwayDlyM",  // PARAM_SWAYDLYM
   "SwayDlyX",  // PARAM_SWAYDLYX
   "SwayFb",    // PARAM_SWAYFB
   "BufSzCb",   // PARAM_BUFSZCB
   "BufSzDly",  // PARAM_BUFSZDLY
   "Linear",    // PARAM_LINEAR
   "Prime",     // PARAM_PRIME
   "Out LPF",   // PARAM_OUT_LPF
   "Cb MixIn",  // PARAM_CB_MIX_IN
   "Out Lvl",   // PARAM_OUT_LVL
};

static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // PARAM_IN_AMT
   0.5f,  // PARAM_IN_HPF
   0.5f,  // PARAM_WET_AMT
   0.5f,  // PARAM_WET_GAIN
   0.5f,  // PARAM_CB_MIX
   0.5f,  // PARAM_CLIP_LVL
   0.5f,  // PARAM_DLY_LSPD
   0.5f,  // PARAM_FBMOD
   0.5f,  // PARAM_FCBMOD
   0.5f,  // PARAM_FAPMOD
   0.5f,  // PARAM_FDLYMOD
   0.5f,  // PARAM_FREQSCLL
   0.5f,  // PARAM_FREQSCLR
   0.5f,  // PARAM_CB1_NOTE
   0.5f,  // PARAM_CB2_NOTE
   0.5f,  // PARAM_CB3_NOTE
   0.5f,  // PARAM_DLYX_NOTE
   0.5f,  // PARAM_CB1_FBAMT
   0.5f,  // PARAM_CB2_FBAMT
   0.5f,  // PARAM_CB3_FBAMT
   0.5f,  // PARAM_DLYX_FBAMT
   0.5f,  // PARAM_AP1_B
   0.5f,  // PARAM_AP1_C
   0.5f,  // PARAM_AP1_NOTE
   0.5f,  // PARAM_AP2_B
   0.5f,  // PARAM_AP2_C
   0.5f,  // PARAM_AP2_NOTE
   0.5f,  // PARAM_APFB_B
   0.5f,  // PARAM_APFB_C
   0.5f,  // PARAM_APFB_NOTE
   0.5f,  // PARAM_CB1_FBDCY
   0.5f,  // PARAM_CB2_FBDCY
   0.5f,  // PARAM_CB3_FBDCY
   0.5f,  // PARAM_DLYX_FBDCY
   0.5f,  // PARAM_X1_AMT
   0.5f,  // PARAM_X2_AMT
   0.5f,  // PARAM_X3_AMT
   0.5f,  // PARAM_X4_AMT
   0.0f,  // PARAM_SWAYCBF
   0.5f,  // PARAM_SWAYCBM
   0.5f,  // PARAM_SWAYCBX
   0.0f,  // PARAM_SWAYAPF
   0.5f,  // PARAM_SWAYAPM
   0.5f,  // PARAM_SWAYAPX
   0.0f,  // PARAM_SWAYDLYF
   0.5f,  // PARAM_SWAYDLYM
   0.5f,  // PARAM_SWAYDLYX
   0.0f,  // PARAM_SWAYFB
   0.5f,  // PARAM_BUFSZCB
   0.5f,  // PARAM_BUFSZDLY
   0.5f,  // PARAM_LINEAR
   0.0f,  // PARAM_PRIME
   0.0f,  // PARAM_OUT_LPF
   0.5f,  // PARAM_CB_MIX_IN
   1.0f,  // PARAM_OUT_LVL
};

#define MOD_DRYWET  0
#define MOD_IN_AMT  1
#define NUM_MODS    2
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "In Lvl"
};

typedef struct schroederverb_info_s {
   st_plugin_info_t base;
} schroederverb_info_t;

typedef struct schroederverb_shared_s {
   st_plugin_shared_t base;
   // // float params[NUM_PARAMS];

   sF32 sample_rate;

   SchroederCh rev[2];

   sF32 in_amt;
   sF32 in_hpf;
   sF32 wet_amt;

   sF32 x1_amt;  // crosschannel feedback
   sF32 x2_amt;
   sF32 x3_amt;
   sF32 x4_amt;

   sF32 wet_gain;
   sF32 comb_mix;
   sF32 comb_mix_in;  // input mix
   sF32 clip_lvl;
   sF32 dly_lerp_spd;
   sF32 out_lpf;
   sF32 out_lvl;

#ifdef USE_PRIME_LOCK
   PrimeLock prime_lock;
#endif // USE_PRIME_LOCK

} schroederverb_shared_t;

typedef struct schroederverb_voice_s {
   st_plugin_voice_t base;
   float    mods[NUM_MODS];
   float    mod_drywet_cur;
   float    mod_drywet_inc;
   float    mod_in_amt_cur;
   float    mod_in_amt_inc;
} schroederverb_voice_t;


static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(schroederverb_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(schroederverb_shared_t);

   shared->sample_rate = _sampleRate;
   shared->rev[0].updateSampleRate(_sampleRate);
   shared->rev[1].updateSampleRate(_sampleRate);
}

static const char *ST_PLUGIN_API loc_get_param_name(st_plugin_info_t *_info,
                                                    unsigned int      _paramIdx
                                                    ) {
   (void)_info;
   return loc_param_names[_paramIdx];
}

static float ST_PLUGIN_API loc_get_param_reset(st_plugin_info_t *_info,
                                               unsigned int      _paramIdx
                                               ) {
   (void)_info;
   return loc_param_resets[_paramIdx];
}

static float ST_PLUGIN_API loc_get_param_value(st_plugin_shared_t *_shared,
                                               unsigned int        _paramIdx
                                               ) {
   ST_PLUGIN_SHARED_CAST(schroederverb_shared_t);
   // // return shared->params[_paramIdx];
   sF32 r = 0.0f;
   switch(_paramIdx)
   {
      default:
      case PARAM_IN_AMT:
         r = shared->in_amt;
         break;

      case PARAM_IN_HPF:
         r = shared->in_hpf;
         break;

      case PARAM_WET_AMT:
         r = shared->wet_amt;
         break;

      case PARAM_FREQSCLL:
         r = shared->rev[0].cb_1.freq_scl;
         break;

      case PARAM_FREQSCLR:
         r = shared->rev[1].cb_1.freq_scl;
         break;

      case PARAM_CB1_NOTE:
         r = shared->rev[0].cb_1.note / 100.0f;
         break;

      case PARAM_CB2_NOTE:
         r = (shared->rev[0].cb_2.note / 100.0f);
         break;

      case PARAM_CB3_NOTE:
         r = (shared->rev[0].cb_3.note / 100.0f);
         break;

      case PARAM_DLYX_NOTE:
         r = (shared->rev[0].dly_x.note / 100.0f);
         break;

      case PARAM_CB1_FBAMT:
         r = shared->rev[0].cb_1.fb_amt;
         break;

      case PARAM_CB2_FBAMT:
         r = shared->rev[0].cb_2.fb_amt;
         break;

      case PARAM_CB3_FBAMT:
         r = shared->rev[0].cb_3.fb_amt;
         break;

      case PARAM_DLYX_FBAMT:
         r = shared->rev[0].dly_x.fb_amt;
         break;

      case PARAM_AP1_B:
         r = (shared->rev[0].ap_1.b * 0.5f) + 0.5f;
         break;

      case PARAM_AP1_C:
         r = (shared->rev[0].ap_1.c * 0.5f) + 0.5f;
         break;

      case PARAM_AP1_NOTE:
         r = (shared->rev[0].ap_1.note / 100.0f);
         break;

      case PARAM_AP2_B:
         r = (shared->rev[0].ap_2.b * 0.5f) + 0.5f;
         break;

      case PARAM_AP2_C:
         r = (shared->rev[0].ap_2.c * 0.5f) + 0.5f;
         break;

      case PARAM_AP2_NOTE:
         r = (shared->rev[0].ap_2.note / 100.0f);
         break;

      case PARAM_APFB_B:
         r = (shared->rev[0].ap_fb.b * 0.5f) + 0.5f;
         break;

      case PARAM_APFB_C:
         r = (shared->rev[0].ap_fb.c * 0.5f) + 0.5f;
         break;

      case PARAM_APFB_NOTE:
         r = (shared->rev[0].ap_fb.note / 100.0f);
         break;

      case PARAM_CB1_FBDCY:
         r = (shared->rev[0].cb_1.fb_dcy * 0.5f) + 0.5f;
         break;

      case PARAM_CB2_FBDCY:
         r = (shared->rev[0].cb_2.fb_dcy * 0.5f) + 0.5f;
         break;

      case PARAM_CB3_FBDCY:
         r = (shared->rev[0].cb_3.fb_dcy * 0.5f) + 0.5f;
         break;

      case PARAM_DLYX_FBDCY:
         r = (shared->rev[0].dly_x.fb_dcy * 0.5f) + 0.5f;
         break;

      case PARAM_X1_AMT:
         r = shared->x1_amt;
         break;

      case PARAM_X2_AMT:
         r = shared->x2_amt;
         break;

      case PARAM_X3_AMT:
         r = shared->x3_amt;
         break;

      case PARAM_X4_AMT:
         r = shared->x4_amt;
         break;

      case PARAM_WET_GAIN:
         r = shared->wet_gain * 0.5f;
         break;

      case PARAM_CB_MIX:
         r = shared->comb_mix * 0.5f + 0.5f;
         break;

      case PARAM_CLIP_LVL:
         r = shared->clip_lvl / 10.0f;
         break;

      case PARAM_DLY_LSPD:
         r = shared->dly_lerp_spd;
         break;

      case PARAM_FCBMOD:
         r = shared->rev[0].cb_1.getFreqModNorm();
         break;

      case PARAM_FAPMOD:
         r = shared->rev[0].ap_1.getFreqModNorm();
         break;

      case PARAM_FDLYMOD:
         r = shared->rev[0].dly_x.getFreqModNorm();
         break;

      case PARAM_FBMOD:
         r = shared->rev[0].cb_1.getFbModNorm();
         break;

      case PARAM_BUFSZCB:
         r = shared->rev[0].cb_1.getDlyBufSizeNorm();
         break;

      case PARAM_BUFSZDLY:
         r = shared->rev[0].dly_x.getDlyBufSizeNorm();
         break;

      case PARAM_LINEAR:
         r = shared->rev[0].dly_x.b_linear ? 1.0f : 0.0f;
         break;

      case PARAM_SWAYCBF:
         r = shared->rev[0].cb_1.sway_freq.getScaleA();
         break;

      case PARAM_SWAYCBM:
         r = shared->rev[0].cb_1.sway_freq.getMinT();
         break;

      case PARAM_SWAYCBX:
         r = shared->rev[0].cb_1.sway_freq.getMaxT();
         break;

      case PARAM_SWAYAPF:
         r = shared->rev[0].ap_1.sway_freq.getScaleA();
         break;

      case PARAM_SWAYAPM:
         r = shared->rev[0].ap_1.sway_freq.getMinT();
         break;

      case PARAM_SWAYAPX:
         r = shared->rev[0].ap_1.sway_freq.getMaxT();
         break;

      case PARAM_SWAYDLYF:
         r = shared->rev[0].dly_x.sway_freq.getScaleA();
         break;

      case PARAM_SWAYDLYM:
         r = shared->rev[0].dly_x.sway_freq.getMinT();
         break;

      case PARAM_SWAYDLYX:
         r = shared->rev[0].dly_x.sway_freq.getMaxT();
         break;

      case PARAM_SWAYFB:
         r = shared->rev[0].cb_1.sway_fb.getScaleA();
         break;

      case PARAM_PRIME:
         r = (NULL != shared->rev[0].cb_1.prime_tbl) ? 1.0f : 0.0f;
         break;

      case PARAM_OUT_LPF:
         r = shared->out_lpf;
         break;

      case PARAM_CB_MIX_IN:
         r = shared->comb_mix_in * 0.5f + 0.5f;
         break;

      case PARAM_OUT_LVL:
         r = shared->out_lvl * 0.5f;
         break;
   }
   return r;
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(schroederverb_shared_t);
   // // shared->params[_paramIdx] = _value;

   switch(_paramIdx)
   {
      default:
      case PARAM_IN_AMT:
         shared->in_amt = _value;
         break;

      case PARAM_IN_HPF:
         shared->in_hpf = _value;
         break;

      case PARAM_WET_AMT:
         shared->wet_amt = _value;
         break;

      case PARAM_FREQSCLL:
         shared->rev[0].cb_1.setFreqScl(_value);
         shared->rev[0].cb_2.setFreqScl(_value);
         shared->rev[0].cb_3.setFreqScl(_value);
         shared->rev[0].dly_x.setFreqScl(_value);

         shared->rev[0].cb_1.handleParamsUpdated();
         shared->rev[0].cb_2.handleParamsUpdated();
         shared->rev[0].cb_3.handleParamsUpdated();
         shared->rev[0].dly_x.handleParamsUpdated();
         break;

      case PARAM_FREQSCLR:
         shared->rev[1].cb_1.setFreqScl(_value);
         shared->rev[1].cb_2.setFreqScl(_value);
         shared->rev[1].cb_3.setFreqScl(_value);
         shared->rev[1].dly_x.setFreqScl(_value);

         shared->rev[1].cb_1.handleParamsUpdated();
         shared->rev[1].cb_2.handleParamsUpdated();
         shared->rev[1].cb_3.handleParamsUpdated();
         shared->rev[1].dly_x.handleParamsUpdated();
         break;

      case PARAM_CB1_NOTE:
         shared->rev[0].cb_1.setNote(_value * 100.0f);
         shared->rev[1].cb_1.setNote(shared->rev[0].cb_1.note);
         shared->rev[0].cb_1.handleParamsUpdated();
         shared->rev[1].cb_1.handleParamsUpdated();
         break;

      case PARAM_CB2_NOTE:
         shared->rev[0].cb_2.setNote(_value * 100.0f);
         shared->rev[1].cb_2.setNote(shared->rev[0].cb_2.note);
         shared->rev[0].cb_2.handleParamsUpdated();
         shared->rev[1].cb_2.handleParamsUpdated();
         break;

      case PARAM_CB3_NOTE:
         shared->rev[0].cb_3.setNote(_value * 100.0f);
         shared->rev[1].cb_3.setNote(shared->rev[0].cb_3.note);
         shared->rev[0].cb_3.handleParamsUpdated();
         shared->rev[1].cb_3.handleParamsUpdated();
         break;

      case PARAM_DLYX_NOTE:
         shared->rev[0].dly_x.setNote(_value * 100.0f);
         shared->rev[1].dly_x.setNote(shared->rev[0].dly_x.note);
         shared->rev[0].dly_x.handleParamsUpdated();
         shared->rev[1].dly_x.handleParamsUpdated();
         break;

      case PARAM_CB1_FBAMT:
         shared->rev[0].cb_1.setFbAmt(_value);
         shared->rev[1].cb_1.setFbAmt(shared->rev[0].cb_1.fb_amt);
         break;

      case PARAM_CB2_FBAMT:
         shared->rev[0].cb_2.setFbAmt(_value);
         shared->rev[1].cb_2.setFbAmt(shared->rev[0].cb_2.fb_amt);
         break;

      case PARAM_CB3_FBAMT:
         shared->rev[0].cb_3.setFbAmt(_value);
         shared->rev[1].cb_3.setFbAmt(shared->rev[0].cb_3.fb_amt);
         break;

      case PARAM_DLYX_FBAMT:
         shared->rev[0].dly_x.setFbAmt(_value);
         shared->rev[1].dly_x.setFbAmt(shared->rev[0].dly_x.fb_amt);
         break;

      case PARAM_AP1_B:
         shared->rev[0].ap_1.setB((_value - 0.5f) * 2.0f);
         shared->rev[1].ap_1.setB(shared->rev[0].ap_1.b);
         break;

      case PARAM_AP1_C:
         shared->rev[0].ap_1.setC((_value - 0.5f) * 2.0f);
         shared->rev[1].ap_1.setC(shared->rev[0].ap_1.c);
         break;

      case PARAM_AP1_NOTE:
         shared->rev[0].ap_1.setNote(_value * 100.0f);
         shared->rev[1].ap_1.setNote(shared->rev[0].ap_1.note);
         shared->rev[0].ap_1.handleParamsUpdated();
         shared->rev[1].ap_1.handleParamsUpdated();
         break;

      case PARAM_AP2_B:
         shared->rev[0].ap_2.setB((_value - 0.5f) * 2.0f);
         shared->rev[1].ap_2.setB(shared->rev[0].ap_2.b);
         break;

      case PARAM_AP2_C:
         shared->rev[0].ap_2.setC((_value - 0.5f) * 2.0f);
         shared->rev[1].ap_2.setC(shared->rev[0].ap_2.c);
         break;

      case PARAM_AP2_NOTE:
         shared->rev[0].ap_2.setNote(_value * 100.0f);
         shared->rev[1].ap_2.setNote(shared->rev[0].ap_2.note);
         shared->rev[0].ap_2.handleParamsUpdated();
         shared->rev[1].ap_2.handleParamsUpdated();
         break;

      case PARAM_APFB_B:
         shared->rev[0].ap_fb.setB((_value - 0.5f) * 2.0f);
         shared->rev[1].ap_fb.setB(shared->rev[0].ap_fb.b);
         break;

      case PARAM_APFB_C:
         shared->rev[0].ap_fb.setC((_value - 0.5f) * 2.0f);
         shared->rev[1].ap_fb.setC(shared->rev[0].ap_fb.c);
         break;

      case PARAM_APFB_NOTE:
         shared->rev[0].ap_fb.setNote(_value * 100.0f);
         shared->rev[1].ap_fb.setNote(shared->rev[0].ap_fb.note);
         shared->rev[0].ap_fb.handleParamsUpdated();
         shared->rev[1].ap_fb.handleParamsUpdated();
         break;

      case PARAM_CB1_FBDCY:
         shared->rev[0].cb_1.setFbDcy((_value - 0.5f) * 2.0f);
         shared->rev[1].cb_1.setFbDcy(shared->rev[0].cb_1.fb_dcy);
         break;

      case PARAM_CB2_FBDCY:
         shared->rev[0].cb_2.setFbDcy((_value - 0.5f) * 2.0f);
         shared->rev[1].cb_2.setFbDcy(shared->rev[0].cb_2.fb_dcy);
         break;

      case PARAM_CB3_FBDCY:
         shared->rev[0].cb_3.setFbDcy((_value - 0.5f) * 2.0f);
         shared->rev[1].cb_3.setFbDcy(shared->rev[0].cb_3.fb_dcy);
         break;

      case PARAM_DLYX_FBDCY:
         shared->rev[0].dly_x.setFbDcy((_value - 0.5f) * 2.0f);
         shared->rev[1].dly_x.setFbDcy(shared->rev[0].dly_x.fb_dcy);
         break;

      case PARAM_X1_AMT:
         shared->x1_amt = _value;
         break;

      case PARAM_X2_AMT:
         shared->x2_amt = _value;
         break;

      case PARAM_X3_AMT:
         shared->x3_amt = _value;
         break;

      case PARAM_X4_AMT:
         shared->x4_amt = _value;
         break;

      case PARAM_WET_GAIN:
         shared->wet_gain = _value * 2.0f;
         break;

      case PARAM_CB_MIX:
         shared->comb_mix = (_value - 0.5f) * 2.0f;
         break;

      case PARAM_CLIP_LVL:
         shared->clip_lvl = _value * 10.0f;
         break;

      case PARAM_DLY_LSPD:
         shared->dly_lerp_spd = _value;
         shared->rev[0].setDlyLerpSpdNorm(shared->dly_lerp_spd);
         shared->rev[1].setDlyLerpSpdNorm(shared->dly_lerp_spd);
         break;

      case PARAM_FCBMOD:
         shared->rev[0].cb_1.setFreqModNorm(_value);
         shared->rev[0].cb_2.setFreqModNorm(_value);
         shared->rev[0].cb_3.setFreqModNorm(_value);

         shared->rev[1].cb_1.setFreqModNorm(_value);
         shared->rev[1].cb_2.setFreqModNorm(_value);
         shared->rev[1].cb_3.setFreqModNorm(_value);

         shared->rev[0].cb_1.handleParamsUpdated();
         shared->rev[0].cb_2.handleParamsUpdated();
         shared->rev[0].cb_3.handleParamsUpdated();

         shared->rev[1].cb_1.handleParamsUpdated();
         shared->rev[1].cb_2.handleParamsUpdated();
         shared->rev[1].cb_3.handleParamsUpdated();
         break;

      case PARAM_FAPMOD:
         shared->rev[0].ap_1.setFreqModNorm(_value);
         shared->rev[0].ap_2.setFreqModNorm(_value);
         shared->rev[0].ap_fb.setFreqModNorm(_value);

         shared->rev[1].ap_1.setFreqModNorm(_value);
         shared->rev[1].ap_2.setFreqModNorm(_value);
         shared->rev[1].ap_fb.setFreqModNorm(_value);

         shared->rev[0].ap_1.handleParamsUpdated();
         shared->rev[0].ap_2.handleParamsUpdated();
         shared->rev[0].ap_fb.handleParamsUpdated();

         shared->rev[1].ap_1.handleParamsUpdated();
         shared->rev[1].ap_2.handleParamsUpdated();
         shared->rev[1].ap_fb.handleParamsUpdated();
         break;

      case PARAM_FDLYMOD:
         shared->rev[0].dly_x.setFreqModNorm(_value);
         shared->rev[1].dly_x.setFreqModNorm(_value);

         shared->rev[0].dly_x.handleParamsUpdated();
         shared->rev[1].dly_x.handleParamsUpdated();
         break;

      case PARAM_FBMOD:
         shared->rev[0].dly_x.setFbModNorm(_value);
         shared->rev[0].cb_1.setFbModNorm(_value);
         shared->rev[0].cb_2.setFbModNorm(_value);
         shared->rev[0].cb_3.setFbModNorm(_value);

         shared->rev[1].dly_x.setFbModNorm(_value);
         shared->rev[1].cb_1.setFbModNorm(_value);
         shared->rev[1].cb_2.setFbModNorm(_value);
         shared->rev[1].cb_3.setFbModNorm(_value);
         break;

      case PARAM_BUFSZCB:
         shared->rev[0].cb_1.setDlyBufSizeNorm(_value);
         shared->rev[0].cb_2.setDlyBufSizeNorm(_value);
         shared->rev[0].cb_3.setDlyBufSizeNorm(_value);

         shared->rev[1].cb_1.setDlyBufSizeNorm(_value);
         shared->rev[1].cb_2.setDlyBufSizeNorm(_value);
         shared->rev[1].cb_3.setDlyBufSizeNorm(_value);
         break;

      case PARAM_BUFSZDLY:
         shared->rev[0].dly_x.setDlyBufSizeNorm(_value);
         shared->rev[1].dly_x.setDlyBufSizeNorm(_value);
         break;

      case PARAM_LINEAR:
         shared->rev[0].cb_1.b_linear  = (_value >= 0.5f);
         shared->rev[0].cb_2.b_linear  = (_value >= 0.5f);
         shared->rev[0].cb_3.b_linear  = (_value >= 0.5f);
         shared->rev[0].dly_x.b_linear = (_value >= 0.5f);

         shared->rev[1].cb_1.b_linear  = (_value >= 0.5f);
         shared->rev[1].cb_2.b_linear  = (_value >= 0.5f);
         shared->rev[1].cb_3.b_linear  = (_value >= 0.5f);
         shared->rev[1].dly_x.b_linear = (_value >= 0.5f);
         break;

      case PARAM_SWAYCBF:
         shared->rev[0].cb_1.sway_freq.setScaleA(_value);
         shared->rev[0].cb_2.sway_freq.setScaleA(_value);
         shared->rev[0].cb_3.sway_freq.setScaleA(_value);

         shared->rev[1].cb_1.sway_freq.setScaleA(_value);
         shared->rev[1].cb_2.sway_freq.setScaleA(_value);
         shared->rev[1].cb_3.sway_freq.setScaleA(_value);
         break;

      case PARAM_SWAYCBM:
         shared->rev[0].cb_1.sway_freq.setMinMaxT(_value, shared->rev[0].cb_1.sway_freq.getMaxT());
         shared->rev[0].cb_2.sway_freq.setMinMaxT(_value, shared->rev[0].cb_2.sway_freq.getMaxT());
         shared->rev[0].cb_3.sway_freq.setMinMaxT(_value, shared->rev[0].cb_3.sway_freq.getMaxT());

         shared->rev[1].cb_1.sway_freq.setMinMaxT(_value, shared->rev[1].cb_1.sway_freq.getMaxT());
         shared->rev[1].cb_2.sway_freq.setMinMaxT(_value, shared->rev[1].cb_2.sway_freq.getMaxT());
         shared->rev[1].cb_3.sway_freq.setMinMaxT(_value, shared->rev[1].cb_3.sway_freq.getMaxT());
         break;

      case PARAM_SWAYCBX:
         shared->rev[0].cb_1.sway_freq.setMinMaxT(shared->rev[0].cb_1.sway_freq.getMinT(), _value);
         shared->rev[0].cb_2.sway_freq.setMinMaxT(shared->rev[0].cb_2.sway_freq.getMaxT(), _value);
         shared->rev[0].cb_3.sway_freq.setMinMaxT(shared->rev[0].cb_3.sway_freq.getMaxT(), _value);
               
         shared->rev[1].cb_1.sway_freq.setMinMaxT(shared->rev[1].cb_1.sway_freq.getMaxT(), _value);
         shared->rev[1].cb_2.sway_freq.setMinMaxT(shared->rev[1].cb_2.sway_freq.getMaxT(), _value);
         shared->rev[1].cb_3.sway_freq.setMinMaxT(shared->rev[1].cb_3.sway_freq.getMaxT(), _value);
         break;

      case PARAM_SWAYAPF:
         shared->rev[0].ap_1 .sway_freq.setScaleA(_value);
         shared->rev[0].ap_2 .sway_freq.setScaleA(_value);
         shared->rev[0].ap_fb.sway_freq.setScaleA(_value);

         shared->rev[1].ap_1 .sway_freq.setScaleA(_value);
         shared->rev[1].ap_2 .sway_freq.setScaleA(_value);
         shared->rev[1].ap_fb.sway_freq.setScaleA(_value);
         break;

      case PARAM_SWAYAPM:
         shared->rev[0].ap_1 .sway_freq.setMinMaxT(_value, shared->rev[0].ap_1.sway_freq.getMaxT());
         shared->rev[0].ap_2 .sway_freq.setMinMaxT(_value, shared->rev[0].ap_2.sway_freq.getMaxT());
         shared->rev[0].ap_fb.sway_freq.setMinMaxT(_value, shared->rev[0].ap_fb.sway_freq.getMaxT());

         shared->rev[1].ap_1 .sway_freq.setMinMaxT(_value, shared->rev[1].ap_1.sway_freq.getMaxT());
         shared->rev[1].ap_2 .sway_freq.setMinMaxT(_value, shared->rev[1].ap_2.sway_freq.getMaxT());
         shared->rev[1].ap_fb.sway_freq.setMinMaxT(_value, shared->rev[1].ap_fb.sway_freq.getMaxT());
         break;

      case PARAM_SWAYAPX:
         shared->rev[0].ap_1 .sway_freq.setMinMaxT(shared->rev[0].ap_1.sway_freq.getMinT(), _value);
         shared->rev[0].ap_2 .sway_freq.setMinMaxT(shared->rev[0].ap_2.sway_freq.getMaxT(), _value);
         shared->rev[0].ap_fb.sway_freq.setMinMaxT(shared->rev[0].ap_fb.sway_freq.getMaxT(), _value);
               
         shared->rev[1].ap_1 .sway_freq.setMinMaxT(shared->rev[1].ap_1.sway_freq.getMaxT(), _value);
         shared->rev[1].ap_2 .sway_freq.setMinMaxT(shared->rev[1].ap_2.sway_freq.getMaxT(), _value);
         shared->rev[1].ap_fb.sway_freq.setMinMaxT(shared->rev[1].ap_fb.sway_freq.getMaxT(), _value);
         break;

      case PARAM_SWAYDLYF:
         shared->rev[0].dly_x.sway_freq.setScaleA(_value);
         shared->rev[1].dly_x.sway_freq.setScaleA(_value);
         break;

      case PARAM_SWAYDLYM:
         shared->rev[0].dly_x.sway_freq.setMinMaxT(_value, shared->rev[0].dly_x.sway_freq.getMaxT());
         shared->rev[1].dly_x.sway_freq.setMinMaxT(_value, shared->rev[1].dly_x.sway_freq.getMaxT());
         break;

      case PARAM_SWAYDLYX:
         shared->rev[0].dly_x.sway_freq.setMinMaxT(shared->rev[0].dly_x.sway_freq.getMinT(), _value);
         shared->rev[1].dly_x.sway_freq.setMinMaxT(shared->rev[1].dly_x.sway_freq.getMinT(), _value);
         break;

      case PARAM_SWAYFB:
         shared->rev[0].cb_1.sway_fb.setScaleA(_value);
         shared->rev[0].cb_2.sway_fb.setScaleA(_value);
         shared->rev[0].cb_3.sway_fb.setScaleA(_value);

         shared->rev[1].cb_1.sway_fb.setScaleA(_value);
         shared->rev[1].cb_2.sway_fb.setScaleA(_value);
         shared->rev[1].cb_3.sway_fb.setScaleA(_value);
         break;

      case PARAM_PRIME:
#ifdef USE_PRIME_LOCK
         shared->rev[0].setPrimeTbl( (_value >= 0.5f) ? shared->prime_lock.prime_tbl : NULL );
         shared->rev[1].setPrimeTbl( (_value >= 0.5f) ? shared->prime_lock.prime_tbl : NULL );
#endif // USE_PRIME_LOCK
         break;

      case PARAM_OUT_LPF:
         shared->out_lpf = _value;
         break;

      case PARAM_CB_MIX_IN:
         shared->comb_mix_in = (_value - 0.5f) * 2.0f;
         break;

      case PARAM_OUT_LVL:
         shared->out_lvl = _value * 2.0f;
         break;
   }
}

static const char *ST_PLUGIN_API loc_get_mod_name(st_plugin_info_t *_info,
                                                  unsigned int      _modIdx
                                                  ) {
   (void)_info;
   return loc_mod_names[_modIdx];
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(schroederverb_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(schroederverb_shared_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(schroederverb_voice_t);
   (void)_frameOffset;
   voice->mods[_modIdx] = _value;
}

static void ST_PLUGIN_API loc_prepare_block(st_plugin_voice_t *_voice,
                                            unsigned int       _numFrames,
                                            float              _freqHz,
                                            float              _note,
                                            float              _vol,
                                            float              _pan
                                            ) {
   ST_PLUGIN_VOICE_CAST(schroederverb_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(schroederverb_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->wet_amt + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modInAmt = shared->in_amt + voice->mods[MOD_IN_AMT];
   modInAmt = Dstplugin_clamp(modInAmt, 0.0f, 1.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc = (modDryWet - voice->mod_drywet_cur) * recBlockSize;
      voice->mod_in_amt_inc = (modInAmt  - voice->mod_in_amt_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur = modDryWet;
      voice->mod_drywet_inc = 0.0f;
      voice->mod_in_amt_cur = modInAmt;
      voice->mod_in_amt_inc = 0.0f;
   }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(schroederverb_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(schroederverb_shared_t);
   // return; // xxxxxxxxxx

   unsigned int kIn = 0u;
   unsigned int kOut = 0u;

   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float inSmpRaw = _samplesIn[kIn];

      for(sUI chIdx = 0u; chIdx < 2u; chIdx++)
      {
         SchroederCh *ch = &shared->rev[chIdx];
         SchroederCh *chO = &shared->rev[chIdx ^ 1u];

         // printf("xxx  process frame[%u] ch[%u]  numFrames=%u\n", i, chIdx, _numFrames);

         sF32 f;
         sF32 inSmp;

         // printf("xxx inSmpRaw[%u] kIn=%u numFrames=%u = %f\n", chIdx, kIn, _numFrames, inSmpRaw);

         inSmp = inSmpRaw - ((ch->last_in + inSmpRaw) * shared->in_hpf * 0.5f);
         ch->last_in = inSmpRaw;

         sF32 fO = ch->ap_fb.process(chO->last_out * shared->x1_amt);
         fO = ch->dly_x.process(fO);

         // printf("xxx inSmp[%u] = %f\n", chIdx, inSmp);
         // printf("xxx f = %f\n", f);

         // // f = ch->ap_1.process(inSmp * in_amt + fO * x2_amt);  // v4
         f = ch->ap_1.process(inSmp * voice->mod_in_amt_cur); // shared->in_amt
         f = ch->ap_2.process(f);

         sF32 fCb1 = ch->cb_1.process(f);
         sF32 fCb2 = ch->cb_2.process(f);
         sF32 fCb3 = ch->cb_3.process(f);

         // // f = fCb1 + fCb2 + fCb3;
         // // f *= comb_mix; //0.333f in v4

         f = (fCb1 + fCb2 + fCb3) * shared->comb_mix + f * shared->comb_mix_in;

         f += fO * shared->x2_amt;  // v5

         if(f > shared->clip_lvl)
            f = shared->clip_lvl;
         else if(f < -shared->clip_lvl)
            f = -shared->clip_lvl;

         f = f + 10.0f;
         f = f - 10.0f;

         ch->last_out = ch->last_out * shared->x3_amt + f * shared->x4_amt;
         f *= shared->wet_gain;
         sF32 lpf = ((ch->last_f + f) * 0.5f);
         ch->last_f = f;
         f = f + (lpf - f) * shared->out_lpf;
         f  = inSmpRaw + ((f - inSmpRaw) * voice->mod_drywet_cur) * voice->mod_drywet_cur;  // wet_amt
         
         _samplesOut[kOut++] = f * shared->out_lvl;

         if(!_bMonoIn)
            inSmpRaw = _samplesIn[++kIn];

      } // loop ch

      voice->mod_drywet_cur += voice->mod_drywet_inc;
      voice->mod_in_amt_cur += voice->mod_in_amt_inc;
   } // loop frames

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   schroederverb_shared_t *ret = (schroederverb_shared_t *)malloc(sizeof(schroederverb_shared_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info  = _info;
      // // memcpy((void*)ret->params, (void*)loc_param_resets, NUM_PARAMS * sizeof(float));

      // Construct channels
      new((&ret->rev[0]))SchroederCh();
      new((&ret->rev[1]))SchroederCh();

#ifdef USE_PRIME_LOCK
      ret->prime_lock.init(512u*1024u); // max dly len, see delay.h / comb.h
#endif // USE_PRIME_LOCK

      ret->sample_rate = 48000.0f;  // updated in set_sample_rate()
      
      ret->rev[0].init(ret->sample_rate);
      ret->rev[1].init(ret->sample_rate);

      ret->in_amt       = 0.3f;
      ret->in_hpf       = 0.92f;
      ret->wet_amt      = 0.29f;
      ret->x1_amt       = 1.0f;
      ret->x2_amt       = 0.4f;
      ret->x3_amt       = 0.0f;
      ret->x4_amt       = 1.0f;
      ret->wet_gain     = 1.0f;
      ret->comb_mix     = 0.333f;
      ret->comb_mix_in  = 0.0f;
      ret->clip_lvl     = 1.0f;
      ret->dly_lerp_spd = 0.05f;

   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_shared_delete(st_plugin_shared_t *_shared) {
   free((void*)_shared);
}

static st_plugin_voice_t *ST_PLUGIN_API loc_voice_new(st_plugin_info_t *_info, unsigned int _voiceIdx) {
   (void)_voiceIdx;
   schroederverb_voice_t *ret = (schroederverb_voice_t *)malloc(sizeof(schroederverb_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info = _info;
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   free((void*)_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free((void*)_info);
}

extern "C" {
st_plugin_info_t *schroederverb_init(void) {
   schroederverb_info_t *ret = NULL;

   ret = (schroederverb_info_t *)malloc(sizeof(schroederverb_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp schroederverb";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "schroederverb";
      ret->base.short_name  = "schroederverb";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_REVERB;
      ret->base.num_params  = NUM_PARAMS;
      ret->base.num_mods    = NUM_MODS;

      ret->base.shared_new         = &loc_shared_new;
      ret->base.shared_delete      = &loc_shared_delete;
      ret->base.voice_new          = &loc_voice_new;
      ret->base.voice_delete       = &loc_voice_delete;
      ret->base.get_param_name     = &loc_get_param_name;
      ret->base.get_param_reset    = &loc_get_param_reset;
      ret->base.get_param_value    = &loc_get_param_value;
      ret->base.set_param_value    = &loc_set_param_value;
      ret->base.get_mod_name       = &loc_get_mod_name;
      ret->base.set_sample_rate    = &loc_set_sample_rate;
      ret->base.note_on            = &loc_note_on;
      ret->base.set_mod_value      = &loc_set_mod_value;
      ret->base.prepare_block      = &loc_prepare_block;
      ret->base.process_replace    = &loc_process_replace;
      ret->base.plugin_exit        = &loc_plugin_exit;
   }

   return &ret->base;
}

} // extern "C"
