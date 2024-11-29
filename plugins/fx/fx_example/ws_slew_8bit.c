// ----
// ---- file   : ws_slew_8bit.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2021-2024 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : waveshaper with configurable rise/fall slew rates and log..lin..exp curve
// ----
// ---- created: 14Apr2021
// ---- changed: 16Aug2021, 21Jan2024, 14Oct2024
// ----
// ----
// ----

// (note) total flash table sz = 4352 + 256 + (4096*2) = 12800 bytes

// total tbl size = (NUM_CURVE_STEPS_H*2+1)*256
//  8 => 4352 bytes
// 12 => 6400 bytes
#define NUM_CURVE_STEPS_H 8

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#define PARAM_DRYWET   0
#define PARAM_AMP_IN   1
#define PARAM_RISE     2
#define PARAM_FALL     3
#define PARAM_CURVE    4
#define PARAM_AMP_OUT  5
#define NUM_PARAMS     6
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "Amp In",
   "Rise",
   "Fall",
   "Curve",
   "Amp Out",
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.0f,  // AMP_IN
   0.3f,  // RISE
   0.3f,  // FALL
   0.5f,  // CURVE
   0.0f,  // AMP_OUT
};

#define MOD_DRYWET   0
#define MOD_AMP_IN   1
#define MOD_RISE     2
#define MOD_FALL     3
#define MOD_CURVE    4
#define MOD_AMP_OUT  5
#define NUM_MODS     6
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Amp In",
   "Rise",
   "Fall",
   "Curve",
   "Amp Out",
};

typedef struct ws_slew_8bit_info_s {
   st_plugin_info_t base;
   unsigned char curve_lut[NUM_CURVE_STEPS_H*2+1][256]; // 17*256= 4325 bytes
   unsigned char rate_lut[256];     // 256 bytes
   unsigned short amp_lut[16][256];  // 16*256*2 = 8192 bytes (x1 .. x4)
} ws_slew_8bit_info_t;

typedef struct ws_slew_8bit_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} ws_slew_8bit_shared_t;

typedef struct ws_slew_8bit_voice_s {
   st_plugin_voice_t base;
   float mods[NUM_MODS];
   float mod_drywet_cur;
   float mod_drywet_inc;
   float mod_amp_in_cur;
   float mod_amp_in_inc;
   float mod_rise_cur;
   float mod_rise_inc;
   float mod_fall_cur;
   float mod_fall_inc;
   float mod_curve_cur;
   float mod_curve_inc;
   float mod_amp_out_cur;
   float mod_amp_out_inc;
   stplugin_us16_t last_l;
   stplugin_us16_t last_r;
   float dly_l;
   float dly_r;
} ws_slew_8bit_voice_t;


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
   ST_PLUGIN_SHARED_CAST(ws_slew_8bit_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(ws_slew_8bit_shared_t);
   shared->params[_paramIdx] = _value;
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
   ST_PLUGIN_VOICE_CAST(ws_slew_8bit_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
      voice->last_l.s = 0;
      voice->last_r.s = 0;
      voice->dly_l = 0.0f;
      voice->dly_r = 0.0f;
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(ws_slew_8bit_voice_t);
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
   ST_PLUGIN_VOICE_CAST(ws_slew_8bit_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_slew_8bit_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modRise = shared->params[PARAM_RISE] + voice->mods[MOD_RISE];
   if(modRise > 1.0f)
      modRise = 1.0f;
   else if(modRise < 0.0f)
      modRise = 0.0f;

   float modFall = shared->params[PARAM_FALL] + voice->mods[MOD_FALL];
   if(modFall > 1.0f)
      modFall = 1.0f;
   else if(modFall < 0.0f)
      modFall = 0.0f;

   float modCurve = (shared->params[PARAM_CURVE]-0.5f)*2.0f + voice->mods[MOD_CURVE];
   modCurve = Dstplugin_clamp(modCurve, -1.0f, 1.0f);

   float modAmpIn = shared->params[PARAM_AMP_IN] + (voice->mods[MOD_AMP_IN]*2.0f);
   if(modAmpIn > 1.0f)
      modAmpIn = 1.0f;
   else if(modAmpIn < 0.0f)
      modAmpIn = 0.0f;

   float modAmpOut = shared->params[PARAM_AMP_OUT] + (voice->mods[MOD_AMP_OUT]*2.0f);
   if(modAmpOut > 1.0f)
      modAmpOut = 1.0f;
   else if(modAmpOut < 0.0f)
      modAmpOut = 0.0f;

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc  = (modDryWet - voice->mod_drywet_cur)  * recBlockSize;
      voice->mod_amp_in_inc  = (modAmpIn  - voice->mod_amp_in_cur)  * recBlockSize;
      voice->mod_rise_inc    = (modRise   - voice->mod_rise_cur)    * recBlockSize;
      voice->mod_fall_inc    = (modFall   - voice->mod_fall_cur)    * recBlockSize;
      voice->mod_curve_inc   = (modCurve  - voice->mod_curve_cur)   * recBlockSize;
      voice->mod_amp_out_inc = (modAmpOut - voice->mod_amp_out_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur  = modDryWet;
      voice->mod_drywet_inc  = 0.0f;
      voice->mod_amp_in_cur  = modAmpIn;
      voice->mod_amp_in_inc  = 0.0f;
      voice->mod_rise_cur    = modRise;
      voice->mod_rise_inc    = 0.0f;
      voice->mod_fall_cur    = modFall;
      voice->mod_fall_inc    = 0.0f;
      voice->mod_curve_cur   = modCurve;
      voice->mod_curve_inc   = 0.0f;
      voice->mod_amp_out_cur = modAmpOut;
      voice->mod_amp_out_inc = 0.0f;
   }
}

static float loc_mathLogLinExpf(float _f, float _c) {
   // c: <0: log
   //     0: lin
   //    >0: exp
   stplugin_fi_t uSign;
   uSign.f = _f;
   stplugin_fi_t u;
   u.f = _f;
   u.u &= 0x7fffFFFFu;
   u.f = powf(u.f, powf(2.0f, _c));
   u.u |= uSign.u & 0x80000000u;
   return u.f;
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(ws_slew_8bit_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(ws_slew_8bit_shared_t);
   ST_PLUGIN_VOICE_INFO_CAST(ws_slew_8bit_info_t);

   unsigned int k = 0u;

   unsigned char modCurveIdx;
   if(voice->mod_curve_cur >= 0.0f)
      modCurveIdx = NUM_CURVE_STEPS_H + (unsigned char)(voice->mod_curve_cur * (NUM_CURVE_STEPS_H-1));
   else
      modCurveIdx = NUM_CURVE_STEPS_H + (unsigned char)(voice->mod_curve_cur * NUM_CURVE_STEPS_H);

#if 0
   float fRise = voice->mod_rise_cur;
   fRise = 1.0f - fRise;
   fRise *= fRise;
   // fRise *= fRise;

   float fFall = voice->mod_fall_cur;
   fFall = 1.0f - fFall;
   fFall *= fFall;
   // fFall *= fFall;

   unsigned short riseCur = (unsigned short)(fRise * 256.0f);
   unsigned short fallCur = (unsigned short)(fFall * 256.0f);
   // unsigned short riseCur = (unsigned short)(fRise * 512.0f);
   // unsigned short fallCur = (unsigned short)(fFall * 512.0f);

#else
   unsigned short riseCur = ((unsigned short)info->rate_lut[ (unsigned char) (Dstplugin_clamp(voice->mod_rise_cur, 0.0f, 1.0f) * 255.0f) ]) + 1u;
   unsigned short fallCur = ((unsigned short)info->rate_lut[ (unsigned char) (Dstplugin_clamp(voice->mod_fall_cur, 0.0f, 1.0f) * 255.0f) ]) + 1u;
#endif

   unsigned char ampInIdx  = (unsigned char) (voice->mod_amp_in_cur * 15);
   unsigned char ampOutIdx = (unsigned char) (voice->mod_amp_out_cur * 15);

#if 0
   static int xxx = 0;
   if(0 == (++xxx & 16383))
   {
       // printf("xxx modCurveIdx=%u\n", modCurveIdx);
      printf("xxx rise=%d fall=%d modCurveIdx=%d ampInIdx=%d ampOutIdx=%d\n", riseCur, fallCur, modCurveIdx, ampInIdx, ampOutIdx);
      fflush(stdout);
   }
#endif
   
   // Stereo input, stereo output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float l = _samplesIn[k];
      float r = _samplesIn[k + 1u];

      // signed short li16 = (signed short) (l * 16383.0f);
      // signed short ri16 = (signed short) (r * 16383.0f);
      // // signed short li16 = (signed short) (l * 8191.0f);
      // // signed short ri16 = (signed short) (r * 8191.0f);
      stplugin_us16_t li16;
      li16.s = (char)(l * 127.0f);
      li16.u = info->amp_lut[ampInIdx][li16.u&255u];

      stplugin_us16_t ri16;
      ri16.s = (char)(r * 127.0f);
      ri16.u = info->amp_lut[ampInIdx][ri16.u&255u];

      // Left
      if(li16.s >= voice->last_l.s)
      {
         voice->last_l.s += ( ((short)(li16.s - voice->last_l.s)) * riseCur) >> 7;
      }
      else
      {
         voice->last_l.s += ( ((short)(li16.s - voice->last_l.s)) * fallCur) >> 7;
      }
      // voice->last_l.s = li16.s; // xxxxxx
      stplugin_us8_t outLi; outLi.u = info->curve_lut[modCurveIdx][voice->last_l.u >> 7];
      if(outLi.u > 255)
      {
         printf("xxx overflow outLi.u=%u\n", outLi.u); fflush(stdout);
      }
      outLi.u = info->amp_lut[ampOutIdx][outLi.u] >> 7;
      // outLi.s = l * 127.0f;  // xxxxxx
      float outL = outLi.s / 127.0f;

      if(ri16.s >= voice->last_r.s)
      {
         voice->last_r.s += ( ((short)(ri16.s - voice->last_r.s)) * riseCur) >> 7;
      }
      else
      {
         voice->last_r.s += ( ((short)(ri16.s - voice->last_r.s)) * fallCur) >> 7;
      }
      // voice->last_r.s = ri16.s; // xxxxxx
      stplugin_us8_t outRi; outRi.u = info->curve_lut[modCurveIdx][voice->last_r.u >> 7];
      outRi.u = info->amp_lut[ampOutIdx][outRi.u] >> 7;
      if(outRi.u > 255)
      {
         printf("xxx overflow outRi.u=%u\n", outRi.u); fflush(stdout);
      }
      // outRi.s = r * 127.0f;  // xxxxxx
      float outR = outRi.s / 127.0f;

      // Output
#if 1
      outL = l + (outL - l) * voice->mod_drywet_cur;
      outR = r + (outR - r) * voice->mod_drywet_cur;
#else
      // Simulate one sample output delay (analog dry/wet)
      {
         float dlyL = voice->dly_l;
         float dlyR = voice->dly_r;
         voice->dly_l = l + (outL - l) * voice->mod_drywet_cur;
         voice->dly_r = r + (outR - r) * voice->mod_drywet_cur;
         outL = dlyL;
         outR = dlyR;
      }
#endif

      outL = Dstplugin_fix_denorm_32(outL);
      outR = Dstplugin_fix_denorm_32(outR);

      _samplesOut[k]      = outL;
      _samplesOut[k + 1u] = outR;

      // Next frame
      k += 2u;
      voice->mod_drywet_cur  += voice->mod_drywet_inc;
      voice->mod_amp_in_cur  += voice->mod_amp_in_inc;
      voice->mod_rise_cur    += voice->mod_rise_inc;
      voice->mod_fall_cur    += voice->mod_fall_inc;
      voice->mod_curve_cur   += voice->mod_curve_inc;
      voice->mod_amp_out_cur += voice->mod_amp_out_inc;
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   ws_slew_8bit_shared_t *ret = malloc(sizeof(ws_slew_8bit_shared_t));
   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));
      ret->base.info  = _info;
      memcpy((void*)ret->params, (void*)loc_param_resets, NUM_PARAMS * sizeof(float));
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_shared_delete(st_plugin_shared_t *_shared) {
   free(_shared);
}

static st_plugin_voice_t *ST_PLUGIN_API loc_voice_new(st_plugin_info_t *_info, unsigned int _voiceIdx) {
   (void)_voiceIdx;
   ws_slew_8bit_voice_t *ret = malloc(sizeof(ws_slew_8bit_voice_t));
   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));
      ret->base.info   = _info;
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   free(_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free(_info);
}

st_plugin_info_t *ws_slew_8bit_init(void) {
   ws_slew_8bit_info_t *ret = NULL;

   ret = malloc(sizeof(ws_slew_8bit_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "bsp ws slew 8bit";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "slew shaper 8bit";
      ret->base.short_name  = "slew 8bit";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_WAVESHAPER;
      ret->base.num_params  = NUM_PARAMS;
      ret->base.num_mods    = NUM_MODS;

      ret->base.shared_new       = &loc_shared_new;
      ret->base.shared_delete    = &loc_shared_delete;
      ret->base.voice_new        = &loc_voice_new;
      ret->base.voice_delete     = &loc_voice_delete;
      ret->base.get_param_name   = &loc_get_param_name;
      ret->base.get_param_reset  = &loc_get_param_reset;
      ret->base.get_param_value  = &loc_get_param_value;
      ret->base.set_param_value  = &loc_set_param_value;
      ret->base.get_mod_name     = &loc_get_mod_name;
      ret->base.note_on          = &loc_note_on;
      ret->base.set_mod_value    = &loc_set_mod_value;
      ret->base.prepare_block    = &loc_prepare_block;
      ret->base.process_replace  = &loc_process_replace;
      ret->base.plugin_exit      = &loc_plugin_exit;

      // Init curve table
      {
         float c = -1.0;
         float cStep = 1.0 / NUM_CURVE_STEPS_H;
         unsigned char *d = &ret->curve_lut[0][0];
      
         for(int curveStep = 0; curveStep < NUM_CURVE_STEPS_H; curveStep++)
         {
            for(int valIdx = 0; valIdx < 256; valIdx++)
            {
               stplugin_us8_t i;
               i.u = (unsigned char)valIdx;
               float f = loc_mathLogLinExpf(i.s / 127.0f, c);
               i.s = (char)(f * 127.0f);
               *d++ = i.u;
            }
            c += cStep;
         }

         for(int valIdx = 0; valIdx < 256; valIdx++)
         {
            *d++ = (unsigned char)valIdx;
         }

         c = cStep;
         for(int curveStep = 0; curveStep < NUM_CURVE_STEPS_H; curveStep++)
         {
            for(int valIdx = 0; valIdx < 256; valIdx++)
            {
               stplugin_us8_t i;
               i.u = (unsigned char)valIdx;
               float f = loc_mathLogLinExpf(i.s / 127.0f, c);
               i.s = (char)(f * 127.0f);
               *d++ = i.u;
            }
            c += cStep;
         }   
      }

      // Init rate table
      {
         float c = 0.0f;
         float cStep = 1.0f / 255;
         unsigned char *d = ret->rate_lut;
         for(int valIdx = 0; valIdx < 256; valIdx++)
         {
            float fRise = 1.0f - c;
            fRise *= fRise;
            unsigned char iRise = (unsigned char)(fRise * 255.0f);
            if(125 == iRise)
               iRise = 127;  // @30%
            *d++ = iRise;
            c += cStep;
         }
      }

      // Amp table
      {
         float c = 1.0f;
         float cStep = 2.0f / 15;
         unsigned short *d = &ret->amp_lut[0][0];
         for(int ampStep = 0; ampStep < 16; ampStep++)
         {
            for(int valIdx = 0; valIdx < 256; valIdx++)
            {
               stplugin_us8_t i8;
               i8.u = (unsigned char)valIdx;
               // float f = tanhf((i8.s / 127.0f) * c);
               float f = ((i8.s / 127.0f) * c);
               if(f > 1.0f)
                  f = 1.0f;
               else if(f < -1.0f)
                  f = -1.0f;
               i8.s = (char)(f * 127.0f);
               // printf(" amp[%d][%d]: %d => %d\n", ampStep, valIdx, i8.s, i8.s);
               stplugin_us16_t i16;
               i16.s = (short)(f * 16383.0f);
               *d++ = i16.u;
            }
            c += cStep;
         }
      }

   }

   return &ret->base;
}
