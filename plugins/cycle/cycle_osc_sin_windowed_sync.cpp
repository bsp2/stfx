// ----
// ---- file   : osc_sin_windowed_sync.cpp
// ---- author : bsp
// ---- legal  : Distributed under terms of the MIT license (https://opensource.org/licenses/MIT)
// ----
// ----          Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// ----          associated documentation files (the "Software"), to deal in the Software without restriction, including
// ----          without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// ----          copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to
// ----          the following conditions:
// ----
// ----          The above copyright notice and this permission notice shall be included in all copies or substantial
// ----          portions of the Software.
// ----
// ----          THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// ----          NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// ----          IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// ----          WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// ----          SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ----
// ---- info   : auto-generated by "Cycle"
// ----           $ g++ -Wall -Wno-unused-function -Wno-unused-variable -I../../tksampler -c osc_sin_windowed_sync.cpp -o osc_sin_windowed_sync.o
// ---- created: 02Aug2024 15:27:43
// ----
// ----
// ----

// (note) pre-define CYCLE_SKIP_UI to skip all code used only for UI/editing purposes (e.g. when compiling for standalone replay)
// #define CYCLE_SKIP_UI  defined

#include <plugin.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>
#define OVERSAMPLE_FACTOR  16.0f

#define PARAM_VSYNC              0
#define PARAM_PW                 1
#define NUM_PARAMS               2
#ifndef CYCLE_SKIP_UI
static const char *loc_param_names[NUM_PARAMS] = {
   "vsync",                   // 0: VSYNC
   "pw",                      // 1: PW

};
#endif // CYCLE_SKIP_UI
static float loc_param_resets[NUM_PARAMS] = {
   0.5f,                      // 0: VSYNC
   0.5f,                      // 1: PW

};

#define MOD_VSYNC                0
#define MOD_PW                   1
#define NUM_MODS                 2
#ifndef CYCLE_SKIP_UI
static const char *loc_mod_names[NUM_MODS] = {
   "vsync",                // 0: VSYNC
   "pw",                   // 1: PW

};
#endif // CYCLE_SKIP_UI

typedef struct osc_sin_windowed_sync_info_s {
   st_plugin_info_t base;
} osc_sin_windowed_sync_info_t;

typedef struct osc_sin_windowed_sync_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];

} osc_sin_windowed_sync_shared_t;

typedef struct osc_sin_windowed_sync_voice_s {
   st_plugin_voice_t base;
   float             sample_rate;
   float             bpm;
   float             velocity;
   float             note_speed_fixed;
   float             note_speed_cur;
   float             note_speed_inc;
   float             note_cur;
   float             note_inc;
   float             mods[NUM_MODS];
   float mod_vsync_cur;
   float mod_vsync_inc;
   float mod_pw_cur;
   float mod_pw_inc;

   float tmp1_sin_phase;
   float tmp2_win_phase;
   float tmp3;
   float tmp4;
   float tmp5;
   float tmp6;
   float var_x;
   float var_v_vsync;
   float sr_factor;

} osc_sin_windowed_sync_voice_t;

#define loop(X)  for(unsigned int i = 0u; i < (X); i++)
#define clamp(a,b,c) (((a)<(b))?(b):(((a)>(c))?(c):(a)))

static inline float mathLerpf(float _a, float _b, float _t) { return _a + (_b - _a) * _t; }
static inline float mathClampf(float a, float b, float c) { return (((a)<(b))?(b):(((a)>(c))?(c):(a))); }
static inline float mathMinf(float a, float b) { return (a<b)?a:b; }
static inline float mathMaxf(float a, float b) { return (a>b)?a:b; }
static inline float mathAbsMaxf(float _x, float _y) { return ( ( (_x<0.0f)?-_x:_x)>((_y<0.0f)?-_y:_y)?_x:_y ); }
static inline float mathAbsMinf(float _x, float _y) { return ( ((_x<0.0f)?-_x:_x)<((_y<0.0f)?-_y:_y)?_x:_y ); }
static inline float frac(float _x) { return _x - ((int)_x); }

static inline float winLinear(const float *_s, float _index) {
   int idx = (int)_index;
   float r = _index - (float)idx;
   return mathLerpf(_s[idx], _s[idx+1], r);
}

static float ffrac_s(float _f) { int i; if(_f >= 0.0f) { i = (int)_f; return _f - (float)i; } else { i = (int)-_f; return 1.0f - (-_f - (float)i); } }

#define USE_CYCLE_SINE_TBL  defined
static float cycle_sine_tbl_f[16384];

static float loc_bipolar_to_scale(const float _t, const float _div, const float _mul) {
   // t (-1..1) => /_div .. *_mul
   
   float s;

   if(_t < 0.0f)
   {
      s = (1.0f / _div);
      s = 1.0f + (s - 1.0f) * -_t;
      if(s < 0.0f)
         s = 0.0f;
   }
   else
   {
      s = _mul;
      s = 1.0f + (s - 1.0f) * _t;
   }
   
   return s;
}

static void loc_prepare(st_plugin_voice_t *_voice);

void loc_prepare(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_sin_windowed_sync_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_sin_windowed_sync_shared_t);

   float out = 0.0f;
   (void)out;
   // -------- lane "prepare" modIdx=0 class=$m_vsync
   
   // -- mod="$m_vsync" dstVar=out
   out = voice->mod_vsync_cur;
   
   // -- mod="fma" dstVar=out
   out = (out * 2.0f) + -1.0f;
   
   // -- mod="bts" dstVar=out
   out = loc_bipolar_to_scale(out, 2.0f, 48.0f);
   
   // -- mod="sto v_vsync" dstVar=out
   voice->var_v_vsync = out;
} /* end prepare */


#ifndef CYCLE_SKIP_UI
static unsigned int loc_copy_chars(char *_d, const unsigned int _dSize, const char *_s) {
   unsigned int r = 0u;
   if(NULL != _d)
   {
      // Write min(dSize,len(s)) characters to 'd'
      char *d = _d;
      if(NULL != _s)
      {
         const char *s = _s;
         for(;;)
         {
            if(r < _dSize)
            {
               char c = s[r++];
               *d++ = c;
               if(0 == c)
                  break;
            }
            else
            {
               if(r > 0u)
                  d[-1] = 0;  // force ASCIIZ
               break;
            }
         }
      }
      else
      {
         if(_dSize > 0u)
         {
            _d[0] = 0;
            r = 1u;
         }
      }
   }
   else
   {
      // Return total length of source string
      if(NULL != _s)
      {
         const char *s = _s;
         for(;;)
         {
            char c = s[r++];
            if(0 == c)
               break;
         }
      }
   }
   return r;
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static unsigned int loc_copy_floats(float              *_d,
                                    const unsigned int  _dSize,
                                    const float        *_s,
                                    const unsigned int  _sNum
                                    ) {
   unsigned int r = 0u;
   if(NULL != _d)
   {
      // Write min(dSize,sNum) characters to 'd'
      if(NULL != _s)
      {
         unsigned int num = (_sNum > _dSize) ? _dSize : _sNum;
         memcpy((void*)_d, (const void*)_s, num * sizeof(float));
         r = num;
      }
   }
   else
   {
      // Query total number of presets
      r = _sNum;
   }
   return r;
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static const char *ST_PLUGIN_API loc_get_param_name(st_plugin_info_t *_info,
                                                    unsigned int      _paramIdx
                                                    ) {
   (void)_info;
   return loc_param_names[_paramIdx];
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static const char *ST_PLUGIN_API loc_get_param_group_name(st_plugin_info_t *_info,
                                                          unsigned int      _paramGroupIdx
                                                          ) {
   (void)_info;
   const char *r = NULL;
   return r;
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static unsigned int ST_PLUGIN_API loc_get_param_group_idx(st_plugin_info_t *_info,
                                                          unsigned int      _paramIdx
                                                          ) {
   (void)_info;
   unsigned int r = ~0u;
   return r;
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static float ST_PLUGIN_API loc_get_param_reset(st_plugin_info_t *_info,
                                               unsigned int      _paramIdx
                                               ) {
   (void)_info;
   return loc_param_resets[_paramIdx];
}
#endif // CYCLE_SKIP_UI

static float ST_PLUGIN_API loc_get_param_value(st_plugin_shared_t *_shared,
                                               unsigned int        _paramIdx
                                               ) {
   ST_PLUGIN_SHARED_CAST(osc_sin_windowed_sync_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(osc_sin_windowed_sync_shared_t);
   shared->params[_paramIdx] = _value;
}

#ifndef CYCLE_SKIP_UI
static unsigned int ST_PLUGIN_API loc_query_dynamic_param_preset_values(st_plugin_shared_t *_shared,
                                                                        const unsigned int  _paramIdx,
                                                                        float              *_retValues,
                                                                        const unsigned int  _retValuesSize
                                                                        ) {
   ST_PLUGIN_SHARED_CAST(osc_sin_windowed_sync_shared_t);
   unsigned int r = 0u;
   return r;
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static unsigned int ST_PLUGIN_API loc_query_dynamic_param_preset_name(st_plugin_shared_t *_shared,
                                                                      const unsigned int  _paramIdx,
                                                                      const unsigned int  _presetIdx,
                                                                      char               *_retBuf,
                                                                      const unsigned int  _retBufSize
                                                                      ) {
   ST_PLUGIN_SHARED_CAST(osc_sin_windowed_sync_shared_t);
   unsigned int r = 0u;
   return r;
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static unsigned int ST_PLUGIN_API loc_get_array_param_size(st_plugin_info_t   *_info,
                                                           const unsigned int  _paramIdx
                                                           ) {
   unsigned int r = 0u;
   switch(_paramIdx)
   {
      default:
         break;
   }
   return r;
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static unsigned int ST_PLUGIN_API loc_get_array_param_num_variations(st_plugin_info_t   *_info,
                                                                     const unsigned int  _paramIdx
                                                                     ) {
   unsigned int r = 0u;
   switch(_paramIdx)
   {
      default:
         break;
   }
   return r;
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static float * ST_PLUGIN_API loc_get_array_param_variation_ptr(st_plugin_shared_t *_shared,
                                                               const unsigned int  _paramIdx,
                                                               const unsigned int  _variationIdx
                                                               ) {
   ST_PLUGIN_SHARED_CAST(osc_sin_windowed_sync_shared_t);
   float *r = NULL;
   switch(_paramIdx)
   {
      default:
         break;
   }
   return r;
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static void ST_PLUGIN_API loc_set_array_param_edit_variation_idx(st_plugin_shared_t *_shared,
                                                                 const unsigned int  _paramIdx,
                                                                 const int           _variationIdx
                                                                 ) {
   ST_PLUGIN_SHARED_CAST(osc_sin_windowed_sync_shared_t);
   switch(_paramIdx)
   {
      default:
         break;
   }
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static const char *ST_PLUGIN_API loc_get_array_param_element_name(st_plugin_info_t   *_info,
                                                                  const unsigned int  _paramIdx,
                                                                  const unsigned int  _elementIdx
                                                                  ) {
   ST_PLUGIN_INFO_CAST(osc_sin_windowed_sync_info_t);
   const char *r = NULL;
   switch(_paramIdx)
   {
      default:
         break;
   }
   return r;
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static int ST_PLUGIN_API loc_get_array_param_element_value_range(st_plugin_info_t   *_info,
                                                                 const unsigned int  _paramIdx,
                                                                 const unsigned int  _elementIdx,
                                                                 float              *_retStorageMin,
                                                                 float              *_retStorageMax,
                                                                 float              *_retDisplayMin,
                                                                 float              *_retDisplayMax,
                                                                 unsigned int       *_retDisplayPrecision
                                                                 ) {
   (void)_elementIdx;
   switch(_paramIdx)
   {
      default:
         break;
   }
   return 0;
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static float ST_PLUGIN_API loc_get_array_param_element_reset(st_plugin_info_t   *_info,
                                                             const unsigned int  _paramIdx,
                                                             const unsigned int  _elementIdx
                                                             ) {
   ST_PLUGIN_INFO_CAST(osc_sin_windowed_sync_info_t);
   float r = -999999.0f/*INVALID_VALUE*/;
   switch(_paramIdx)
   {
      default:
         break;
   }
   return r;
}
#endif // CYCLE_SKIP_UI

#ifndef CYCLE_SKIP_UI
static const char *ST_PLUGIN_API loc_get_mod_name(st_plugin_info_t *_info,
                                                  unsigned int      _modIdx
                                                  ) {
   (void)_info;
   return loc_mod_names[_modIdx];
}
#endif // CYCLE_SKIP_UI

static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(osc_sin_windowed_sync_voice_t);
   if(_sampleRate != voice->sample_rate)
   {
      voice->sr_factor = 3000.0f / _sampleRate;
   }
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_set_bpm(st_plugin_voice_t *_voice,
                                      float              _bpm
                                      ) {
   ST_PLUGIN_VOICE_CAST(osc_sin_windowed_sync_voice_t);
   voice->bpm = _bpm;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(osc_sin_windowed_sync_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_sin_windowed_sync_shared_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
#ifdef OVERSAMPLE_FACTOR
      voice->note_speed_fixed = (261.63f/*C-5*/ / (voice->sample_rate * OVERSAMPLE_FACTOR));
#else
      voice->note_speed_fixed = (261.63f/*C-5*/ / voice->sample_rate);
#endif // OVERSAMPLE_FACTOR
      voice->velocity = _vel;
      voice->tmp1_sin_phase = 0.0f;
      voice->tmp2_win_phase = 0.0f;
      voice->var_x = 0.0f;
      voice->var_v_vsync = 0.0f;
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(osc_sin_windowed_sync_voice_t);
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
   ST_PLUGIN_VOICE_CAST(osc_sin_windowed_sync_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_sin_windowed_sync_shared_t);
   (void)_note;
   (void)_vol;
   (void)_pan;

#ifdef OVERSAMPLE_FACTOR
   float noteSpeed = _freqHz / (voice->sample_rate * OVERSAMPLE_FACTOR);
#else
   float noteSpeed = _freqHz / voice->sample_rate;
#endif // OVERSAMPLE_FACTOR

   float modvsync        = shared->params[PARAM_VSYNC       ]                       + voice->mods[MOD_VSYNC        ];
   float modpw           = shared->params[PARAM_PW          ]                       + voice->mods[MOD_PW           ];

   if(_numFrames > 0u)
   {
      // lerp
#ifdef OVERSAMPLE_FACTOR
      float recBlockSize = (1.0f / (_numFrames * OVERSAMPLE_FACTOR));
#else
      float recBlockSize = (1.0f / _numFrames);
#endif // OVERSAMPLE_FACTOR
      voice->note_speed_inc = (noteSpeed - voice->note_speed_cur) * recBlockSize;
      voice->note_inc       = (_note - voice->note_cur) * recBlockSize;
      voice->mod_vsync_inc        = (modvsync           - voice->mod_vsync_cur         ) * recBlockSize;
      voice->mod_pw_inc           = (modpw              - voice->mod_pw_cur            ) * recBlockSize;
      loc_prepare(&voice->base);
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->note_speed_cur = noteSpeed;
      voice->note_speed_inc = 0.0f;
      voice->note_cur       = _note;
      voice->note_inc       = 0.0f;
      voice->mod_vsync_cur        = modvsync;
      voice->mod_vsync_inc        = 0.0f;
      voice->mod_pw_cur           = modpw;
      voice->mod_pw_inc           = 0.0f;
      loc_prepare(&voice->base);
   }

   // printf("xxx note_cur=%f\n", voice->note_cur);
   // printf("xxx prepare_block: numFrames=%u moda=%f\n", _numFrames, moda);
   // printf("xxx voice->voicebus_idx_0=%u voice->base.voice_bus_read_offset=%u\n", voice->voicebus_idx_0, voice->base.voice_bus_read_offset);
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut,
                                              unsigned int        _numFrames
                                              ) {
   (void)_bMonoIn;
   (void)_samplesIn;

   ST_PLUGIN_VOICE_CAST(osc_sin_windowed_sync_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_sin_windowed_sync_shared_t);

   // Mono output (replicate left to right channel)
   unsigned int j = 0u;
#ifndef STEREO
   unsigned int jStep = _bMonoIn ? 1u : 2u;
#endif // STEREO
   unsigned int k = 0u;
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      const float inL = _samplesIn[j];
#ifdef STEREO
      float outL;
      float outR;
      const float inR = _samplesIn[j + 1u];
#endif // STEREO
      float out;
#ifdef OVERSAMPLE_FACTOR
#ifdef STEREO
      float outOSL = 0.0f;
      float outOSR = 0.0f;
#else
      float outOS = 0.0f;
#endif // STEREO
#endif // OVERSAMPLE_FACTOR

#ifdef OVERSAMPLE_FACTOR
      for(unsigned int osi = 0u; osi < OVERSAMPLE_FACTOR; osi++)
#endif // OVERSAMPLE_FACTOR
      {
#ifdef STEREO
         out = outL = inL;
         outR = inR;
#else
         out = inL;
#endif // STEREO
         float tmp_f;
         float tmp2_f;
         
         // ========
         // ======== lane "out" modIdx=0 class=sin
         // ========
         
         // -- mod="sin" dstVar=out
         voice->tmp4/*sin_freq*/ = 1;
         voice->tmp3/*sin_speed*/ = voice->note_speed_cur * voice->tmp4/*sin_freq*/;
         
         // ---- mod="sin" input "vsync" seq 1/1
         
         // -- mod="$v_vsync" dstVar=voice->tmp5/*vsync*/
         voice->tmp5/*vsync*/ = voice->var_v_vsync;
         voice->tmp4/*sin_tmp*/ = (voice->tmp1_sin_phase);
         voice->tmp4/*sin_tmp*/ = ffrac_s(voice->tmp4/*sin_tmp*/);
         out = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp4/*sin_tmp*/)&16383u];
         voice->tmp6/*window*/ = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp2_win_phase)&16383u];
         voice->tmp6/*window*/ *= voice->tmp6/*window*/;
         out *= voice->tmp6/*window*/;
         voice->tmp1_sin_phase = ffrac_s(voice->tmp1_sin_phase + voice->tmp3/*sin_speed*/ * voice->tmp5/*vsync*/);
         tmp_f = voice->tmp2_win_phase;
         voice->tmp2_win_phase = ffrac_s(voice->tmp2_win_phase + voice->tmp3/*sin_speed*/);
         if(tmp_f > voice->tmp2_win_phase) voice->tmp1_sin_phase = voice->tmp2_win_phase * voice->tmp5/*vsync*/; 
   
         /* end calc */

#ifdef OVERSAMPLE_FACTOR
#ifdef STEREO
         outOSL += outL;
         outOSR += outR;
#else
         outOS += out;
#endif // STEREO
#endif // OVERSAMPLE_FACTOR
         voice->note_speed_cur += voice->note_speed_inc;
         voice->note_cur       += voice->note_inc;
         voice->mod_vsync_cur      += voice->mod_vsync_inc;
         voice->mod_pw_cur         += voice->mod_pw_inc;
      }
#ifdef OVERSAMPLE_FACTOR
      // Apply lowpass filter before downsampling
      //   (note) normalized Fc = F/Fs = 0.442947 / sqrt(oversample_factor^2 - 1)
#ifdef STEREO
      outL = outOSL * (1.0f / OVERSAMPLE_FACTOR);
      outR = outOSR * (1.0f / OVERSAMPLE_FACTOR);
#else
      out = outOS * (1.0f / OVERSAMPLE_FACTOR);
#endif // STEREO
#endif // OVERSAMPLE_FACTOR
#ifdef STEREO
      outL = Dstplugin_fix_denorm_32(outL);
      outR = Dstplugin_fix_denorm_32(outR);
#else
      out = Dstplugin_fix_denorm_32(out);
#endif // STEREO
#ifdef STEREO
      _samplesOut[k]      = outL;
      _samplesOut[k + 1u] = outR;
#else
      _samplesOut[k]      = out;
      _samplesOut[k + 1u] = out;
#endif // STEREO

      // Next frame
      k += 2u;
#ifdef STEREO
      j += 2u;
#else
      j += jStep;
#endif // STEREO
   } /* loop numFrames */
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   osc_sin_windowed_sync_shared_t *ret = (osc_sin_windowed_sync_shared_t *)malloc(sizeof(osc_sin_windowed_sync_shared_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info  = _info;
      memcpy((void*)ret->params, (void*)loc_param_resets, NUM_PARAMS * sizeof(float));
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_shared_delete(st_plugin_shared_t *_shared) {
   free((void*)_shared);
}

static st_plugin_voice_t *ST_PLUGIN_API loc_voice_new(st_plugin_info_t *_info, unsigned int _voiceIdx) {
   osc_sin_windowed_sync_voice_t *voice = (osc_sin_windowed_sync_voice_t *)malloc(sizeof(osc_sin_windowed_sync_voice_t));
   if(NULL != voice)
   {
      memset((void*)voice, 0, sizeof(*voice));
      voice->base.info = _info;
   }
   return &voice->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_sin_windowed_sync_voice_t);

   free((void*)_voice);
   }

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free((void*)_info);
}

#ifdef USE_CYCLE_SINE_TBL
static void loc_calc_sine_tbl(void) {
#define QCOS_BITS 16
#define QCOS_ONE  (1 << QCOS_BITS)
#define QCOS_MASK (QCOS_ONE - 1)
   // (note) same as in TSR "C" implementation
   float *qcos = cycle_sine_tbl_f + 4096; // quarter cos tbl

   // calc quarter cos tbl
   int k = 0;
   for(int a = 0; a < (QCOS_ONE/4); a += (QCOS_ONE/16384)/*4*/)  // 4096 elements
   {
      int x = a - ((int)(QCOS_ONE * 0.25 + 0.5)) + ((a + ((int)(QCOS_ONE * 0.25 + 0.5)))&~QCOS_MASK);
      x = ((x * ((x<0?-x:x) - ((int)(QCOS_ONE * 0.5 + 0.5)))) >> QCOS_BITS) << 4;
      x += (((((int)(QCOS_ONE * 0.225 + 0.5)) * x) >> QCOS_BITS) * ((x<0?-x:x) - QCOS_ONE)) >> QCOS_BITS;
      qcos[k++] = x / float(QCOS_ONE);
   }

   // 0�..90� (rev qtbl)
   int j = 4096;
   k = 0;
   loop(4096)
      cycle_sine_tbl_f[k++] = qcos[--j];

   // 90..180� = cos tbl
   k += 4096;

   // 180�..360� (y-flip first half of tbl)
   j = 0;
   loop(8192)
      cycle_sine_tbl_f[k++] = -cycle_sine_tbl_f[j++];

#ifdef USE_CYCLE_SINE_TBL_I
   loop(16384)
      cycle_sine_tbl_i[i] = (short)(cycle_sine_tbl_f[i] * 2048.0f/*FX_ONE*/);
#endif // USE_CYCLE_SINE_TBL_I
}
#endif // USE_CYCLE_SINE_TBL

#ifdef USE_CYCLE_HSE_TBL
#endif // USE_CYCLE_HSE_TBL

extern "C" {
st_plugin_info_t *osc_sin_windowed_sync_init(void) {
   osc_sin_windowed_sync_info_t *ret = (osc_sin_windowed_sync_info_t *)malloc(sizeof(osc_sin_windowed_sync_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "osc_sin_windowed_sync";  // unique id. don't change this in future builds.
      // (note) don't set author/name/short_name when CYCLE_SKIP_UI is defined ?
      ret->base.author      = "bsp";
      ret->base.name        = "osc sin windowed sync";
      ret->base.short_name  = "osc sin windowed sync";
      ret->base.flags       = ST_PLUGIN_FLAG_OSC;
      ret->base.category    = ST_PLUGIN_CAT_UNKNOWN;
      ret->base.num_params  = NUM_PARAMS;
      ret->base.num_mods    = NUM_MODS;

      ret->base.shared_new                          = &loc_shared_new;
      ret->base.shared_delete                       = &loc_shared_delete;
      ret->base.voice_new                           = &loc_voice_new;
      ret->base.voice_delete                        = &loc_voice_delete;
#ifndef CYCLE_SKIP_UI
      ret->base.get_param_name                      = &loc_get_param_name;
      ret->base.get_param_group_name                = &loc_get_param_group_name;
      ret->base.get_param_group_idx                 = &loc_get_param_group_idx;
      ret->base.get_param_reset                     = &loc_get_param_reset;
      ret->base.query_dynamic_param_preset_values   = &loc_query_dynamic_param_preset_values;
      ret->base.query_dynamic_param_preset_name     = &loc_query_dynamic_param_preset_name;
      ret->base.get_array_param_size                = &loc_get_array_param_size;
      ret->base.get_array_param_num_variations      = &loc_get_array_param_num_variations;
      ret->base.get_array_param_variation_ptr       = &loc_get_array_param_variation_ptr;
      ret->base.set_array_param_edit_variation_idx  = &loc_set_array_param_edit_variation_idx;
      ret->base.get_array_param_element_name        = &loc_get_array_param_element_name;
      ret->base.get_array_param_element_value_range = &loc_get_array_param_element_value_range;
      ret->base.get_array_param_element_reset       = &loc_get_array_param_element_reset;
#endif // CYCLE_SKIP_UI
      ret->base.get_param_value                     = &loc_get_param_value;
      ret->base.set_param_value                     = &loc_set_param_value;
#ifndef CYCLE_SKIP_UI
      ret->base.get_mod_name                        = &loc_get_mod_name;
#endif // CYCLE_SKIP_UI
      ret->base.set_sample_rate                     = &loc_set_sample_rate;
      ret->base.set_bpm                             = &loc_set_bpm;
      ret->base.note_on                             = &loc_note_on;
      ret->base.set_mod_value                       = &loc_set_mod_value;
      ret->base.prepare_block                       = &loc_prepare_block;
      ret->base.process_replace                     = &loc_process_replace;
      ret->base.plugin_exit                         = &loc_plugin_exit;

#ifdef USE_CYCLE_SINE_TBL
      loc_calc_sine_tbl();
#endif // USE_CYCLE_SINE_TBL
   }

   return &ret->base;
}

#ifndef STFX_SKIP_MAIN_INIT
ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init(unsigned int _pluginIdx) {
   switch(_pluginIdx)
   {
      case 0u:
         return osc_sin_windowed_sync_init();
   }
   return NULL;
}
#endif // STFX_SKIP_MAIN_INIT

} // extern "C"