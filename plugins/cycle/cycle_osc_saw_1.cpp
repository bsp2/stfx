// ----
// ---- file   : osc_saw_1.cpp
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
// ----           $ g++ -Wall -Wno-unused-function -Wno-unused-variable -I../../tksampler -c osc_saw_1.cpp -o osc_saw_1.o
// ---- created: 05Aug2024 17:28:31
// ----
// ----
// ----

// (note) pre-define CYCLE_SKIP_UI to skip all code used only for UI/editing purposes (e.g. when compiling for standalone replay)
// #define CYCLE_SKIP_UI  defined

#include <plugin.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>
#define OVERSAMPLE_FACTOR  8.0f

#define PARAM_PHASE_RAND         0
#define PARAM_COLOR              1
#define PARAM_CUTOFF             2
#define PARAM_RES                3
#define PARAM_DETUNE             4
#define NUM_PARAMS               5
#ifndef CYCLE_SKIP_UI
static const char *loc_param_names[NUM_PARAMS] = {
   "phase_rand",              // 0: PHASE_RAND
   "color",                   // 1: COLOR
   "cutoff",                  // 2: CUTOFF
   "res",                     // 3: RES
   "detune",                  // 4: DETUNE

};
#endif // CYCLE_SKIP_UI
static float loc_param_resets[NUM_PARAMS] = {
   0.0f,                      // 0: PHASE_RAND
   0.5f,                      // 1: COLOR
   0.5f,                      // 2: CUTOFF
   0.3f,                      // 3: RES
   0.5f,                      // 4: DETUNE

};

#define MOD_COLOR                0
#define MOD_CUTOFF               1
#define MOD_RES                  2
#define MOD_DETUNE               3
#define NUM_MODS                 4
#ifndef CYCLE_SKIP_UI
static const char *loc_mod_names[NUM_MODS] = {
   "color",                // 0: COLOR
   "cutoff",               // 1: CUTOFF
   "res",                  // 2: RES
   "detune",               // 3: DETUNE

};
#endif // CYCLE_SKIP_UI

typedef struct osc_saw_1_info_s {
   st_plugin_info_t base;
} osc_saw_1_info_t;

typedef struct osc_saw_1_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];

} osc_saw_1_shared_t;

typedef struct osc_saw_1_voice_s {
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
   float mod_color_cur;
   float mod_color_inc;
   float mod_cutoff_cur;
   float mod_cutoff_inc;
   float mod_res_cur;
   float mod_res_inc;
   float mod_detune_cur;
   float mod_detune_inc;

   short tmp1;
   unsigned short tmp2_lfsr_state;
   short tmp3_lfsr_state_signed;
   float tmp4;
   float tmp5_saw_phase;
   float tmp6;
   float tmp7;
   float tmp8;
   float tmp9_svf_lp;
   float tmp10_svf_hp;
   float tmp11_svf_bp;
   float tmp12_svf_lp;
   float tmp13_svf_hp;
   float tmp14_svf_bp;
   float var_x;
   float var_v_phrand_1;
   float var_v_cutoff;
   float var_v_res;
   float sr_factor;

} osc_saw_1_voice_t;

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

static float mathLogf(const float _x) {
   union {
      float f;
      unsigned int u;
   } bx;
   bx.f = _x;
   const unsigned int ex = bx.u >> 23;
   const signed int t = (signed int)ex - (signed int)127;
   const unsigned int s = (t < 0) ? (-t) : t;
   bx.u = 1065353216u | (bx.u & 8388607u);
   return
      -1.7417939f + (2.8212026f + (-1.4699568f + (0.44717955f - 0.056570851f * bx.f)*bx.f)*bx.f)*bx.f
      + 0.6931471806f * t;
}

static float mathPowerf(float _x, float _y) {
   float r;
   if(_y != 0.0f)
   {
      if(_x < 0.0f)
      {
         r = (float)( -expf(_y*mathLogf(-_x)) );
      }
      else if(_x > 0.0f)
      {
         r = (float)( expf(_y*mathLogf(_x)) );
      }
      else
      {
         r = 0.0f;
      }
   }
   else
   {
      r = 1.0f;
   }
   return Dstplugin_fix_denorm_32(r);
}

static float mathLogLinExpf(float _f, float _c) {
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

static void loc_init(st_plugin_voice_t *_voice);
static void loc_prepare(st_plugin_voice_t *_voice);

void loc_init(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_saw_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_saw_1_shared_t);

   float out = 0.0f;
   (void)out;
   // -------- lane "init" modIdx=0 class=fsr
   
   // -- mod="fsr" dstVar=out
   voice->tmp1/*i2f*/ = (int)(out * 2048);  // IntFallback: F2I
   voice->tmp2_lfsr_state ^= voice->tmp2_lfsr_state >> 7;
   voice->tmp2_lfsr_state ^= voice->tmp2_lfsr_state << 9;
   voice->tmp2_lfsr_state ^= voice->tmp2_lfsr_state >> 13;
   voice->tmp3_lfsr_state_signed = (voice->tmp2_lfsr_state & 65520);
   voice->tmp1/*i2f*/ = voice->tmp3_lfsr_state_signed >> 4;
   out = voice->tmp1/*i2f*/ / ((float)(2048));  // IntFallback: I2F
   
   // -- mod="$p_phase_rand" dstVar=out
   voice->tmp4/*seq*/ = shared->params[PARAM_PHASE_RAND];
   out *= voice->tmp4/*seq*/;
   
   // -- mod="sto v_phrand_1" dstVar=out
   voice->var_v_phrand_1 = out;
} /* end init */

void loc_prepare(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_saw_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_saw_1_shared_t);

   float out = 0.0f;
   (void)out;
   // -------- lane "prepare" modIdx=0 class=$m_cutoff
   
   // -- mod="$m_cutoff" dstVar=out
   out = voice->mod_cutoff_cur;
   
   // -- mod="clp" dstVar=out
   if(out > 1.0f) out = 1.0f;
   else if(out < 0.0f) out = 0.0f;
   
   // -- mod="pow" dstVar=out
   out = out * out;
   
   // -- mod="p2s" dstVar=out
   out  = (mathPowerf(2.0f, out * 7.0f) - 1.0f);
   out *= 0.00787402f;
   
   // -- mod="sto v_cutoff" dstVar=out
   voice->var_v_cutoff = out;
   
   // -- mod="$m_res" dstVar=out
   out = voice->mod_res_cur;
   
   // -- mod="sto v_res" dstVar=out
   voice->var_v_res = out;
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
   ST_PLUGIN_SHARED_CAST(osc_saw_1_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(osc_saw_1_shared_t);
   shared->params[_paramIdx] = _value;
}

#ifndef CYCLE_SKIP_UI
static unsigned int ST_PLUGIN_API loc_query_dynamic_param_preset_values(st_plugin_shared_t *_shared,
                                                                        const unsigned int  _paramIdx,
                                                                        float              *_retValues,
                                                                        const unsigned int  _retValuesSize
                                                                        ) {
   ST_PLUGIN_SHARED_CAST(osc_saw_1_shared_t);
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
   ST_PLUGIN_SHARED_CAST(osc_saw_1_shared_t);
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
   ST_PLUGIN_SHARED_CAST(osc_saw_1_shared_t);
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
   ST_PLUGIN_SHARED_CAST(osc_saw_1_shared_t);
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
   ST_PLUGIN_INFO_CAST(osc_saw_1_info_t);
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
   ST_PLUGIN_INFO_CAST(osc_saw_1_info_t);
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
   ST_PLUGIN_VOICE_CAST(osc_saw_1_voice_t);
   if(_sampleRate != voice->sample_rate)
   {
      voice->sr_factor = 6000.0f / _sampleRate;
   }
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_set_bpm(st_plugin_voice_t *_voice,
                                      float              _bpm
                                      ) {
   ST_PLUGIN_VOICE_CAST(osc_saw_1_voice_t);
   voice->bpm = _bpm;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(osc_saw_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_saw_1_shared_t);
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
      voice->tmp3_lfsr_state_signed = 0;
      voice->tmp5_saw_phase = 0.0f;
      voice->tmp9_svf_lp = 0.0f;
      voice->tmp10_svf_hp = 0.0f;
      voice->tmp11_svf_bp = 0.0f;
      voice->tmp12_svf_lp = 0.0f;
      voice->tmp13_svf_hp = 0.0f;
      voice->tmp14_svf_bp = 0.0f;
      voice->var_x = 0.0f;
      voice->var_v_phrand_1 = 0.0f;
      voice->var_v_cutoff = 0.0f;
      voice->var_v_res = 0.0f;
      loc_init(&voice->base);
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(osc_saw_1_voice_t);
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
   ST_PLUGIN_VOICE_CAST(osc_saw_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_saw_1_shared_t);
   (void)_note;
   (void)_vol;
   (void)_pan;

#ifdef OVERSAMPLE_FACTOR
   float noteSpeed = _freqHz / (voice->sample_rate * OVERSAMPLE_FACTOR);
#else
   float noteSpeed = _freqHz / voice->sample_rate;
#endif // OVERSAMPLE_FACTOR

   float modcolor        = shared->params[PARAM_COLOR       ] * 8.0f - 4.0f         + voice->mods[MOD_COLOR        ];
   float modcutoff       = shared->params[PARAM_CUTOFF      ]                       + voice->mods[MOD_CUTOFF       ];
   float modres          = shared->params[PARAM_RES         ]                       + voice->mods[MOD_RES          ];
   float moddetune       = shared->params[PARAM_DETUNE      ] * 2.0f - 1.0f         + voice->mods[MOD_DETUNE       ];

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
      voice->mod_color_inc        = (modcolor           - voice->mod_color_cur         ) * recBlockSize;
      voice->mod_cutoff_inc       = (modcutoff          - voice->mod_cutoff_cur        ) * recBlockSize;
      voice->mod_res_inc          = (modres             - voice->mod_res_cur           ) * recBlockSize;
      voice->mod_detune_inc       = (moddetune          - voice->mod_detune_cur        ) * recBlockSize;
      loc_prepare(&voice->base);
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->note_speed_cur = noteSpeed;
      voice->note_speed_inc = 0.0f;
      voice->note_cur       = _note;
      voice->note_inc       = 0.0f;
      voice->mod_color_cur        = modcolor;
      voice->mod_color_inc        = 0.0f;
      voice->mod_cutoff_cur       = modcutoff;
      voice->mod_cutoff_inc       = 0.0f;
      voice->mod_res_cur          = modres;
      voice->mod_res_inc          = 0.0f;
      voice->mod_detune_cur       = moddetune;
      voice->mod_detune_inc       = 0.0f;
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

   ST_PLUGIN_VOICE_CAST(osc_saw_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_saw_1_shared_t);

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
         // ======== lane "out" modIdx=0 class=saw
         // ========
         
         // -- mod="saw" dstVar=out
         
         // ---- mod="saw" input "freq" seq 1/1
         
         // -- mod="1" dstVar=voice->tmp7/*saw_freq*/
         voice->tmp7/*saw_freq*/ = 1.0f;
         
         // -- mod="$m_detune" dstVar=voice->tmp7/*saw_freq*/
         voice->tmp8/*seq*/ = voice->mod_detune_cur;
         voice->tmp7/*saw_freq*/ += voice->tmp8/*seq*/;
         voice->tmp6/*saw_speed*/ = voice->note_speed_cur * voice->tmp7/*saw_freq*/;
         
         // ---- mod="saw" input "phase" seq 1/1
         
         // -- mod="$v_phrand_1" dstVar=voice->tmp8/*phase*/
         voice->tmp8/*phase*/ = voice->var_v_phrand_1;
         voice->tmp7/*saw_tmp*/ = ((voice->tmp5_saw_phase + voice->tmp8/*phase*/));
         voice->tmp7/*saw_tmp*/ = ffrac_s(voice->tmp7/*saw_tmp*/);
         out = 1.0 - (voice->tmp7/*saw_tmp*/ * 2.0f);
         voice->tmp5_saw_phase = ffrac_s(voice->tmp5_saw_phase + voice->tmp6/*saw_speed*/);
         
         // -- mod="lle" dstVar=out
         
         // ---- mod="lle" input "c" seq 1/1
         
         // -- mod="$m_color" dstVar=voice->tmp6/*c*/
         voice->tmp6/*c*/ = voice->mod_color_cur;
         out = mathLogLinExpf(out, voice->tmp6/*c*/);
         
         // -- mod="svf" dstVar=out
         
         // ---- mod="svf" input "res" seq 1/1
         
         // -- mod="$v_res" dstVar=voice->tmp6/*res*/
         voice->tmp6/*res*/ = voice->var_v_res;
         
         // ---- mod="svf" input "freq" seq 1/1
         
         // -- mod="$v_cutoff" dstVar=voice->tmp7/*addsr_freq*/
         voice->tmp7/*addsr_freq*/ = voice->var_v_cutoff;
         voice->tmp7/*addsr_freq*/ *= voice->sr_factor;
         voice->tmp9_svf_lp = voice->tmp9_svf_lp + (voice->tmp11_svf_bp * voice->tmp7/*addsr_freq*/);
         voice->tmp10_svf_hp = out - voice->tmp9_svf_lp - (voice->tmp11_svf_bp * voice->tmp6/*res*/);
         voice->tmp11_svf_bp = voice->tmp11_svf_bp + (voice->tmp10_svf_hp * voice->tmp7/*addsr_freq*/);
         out = voice->tmp9_svf_lp;
         
         // -- mod="svf" dstVar=out
         
         // ---- mod="svf" input "res" seq 1/1
         
         // -- mod="$v_res" dstVar=voice->tmp6/*res*/
         voice->tmp6/*res*/ = voice->var_v_res;
         
         // ---- mod="svf" input "freq" seq 1/1
         
         // -- mod="$v_cutoff" dstVar=voice->tmp7/*addsr_freq*/
         voice->tmp7/*addsr_freq*/ = voice->var_v_cutoff;
         voice->tmp7/*addsr_freq*/ *= voice->sr_factor;
         voice->tmp12_svf_lp = voice->tmp12_svf_lp + (voice->tmp14_svf_bp * voice->tmp7/*addsr_freq*/);
         voice->tmp13_svf_hp = out - voice->tmp12_svf_lp - (voice->tmp14_svf_bp * voice->tmp6/*res*/);
         voice->tmp14_svf_bp = voice->tmp14_svf_bp + (voice->tmp13_svf_hp * voice->tmp7/*addsr_freq*/);
         out = voice->tmp12_svf_lp;
   
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
         voice->mod_color_cur      += voice->mod_color_inc;
         voice->mod_cutoff_cur     += voice->mod_cutoff_inc;
         voice->mod_res_cur        += voice->mod_res_inc;
         voice->mod_detune_cur     += voice->mod_detune_inc;
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
   osc_saw_1_shared_t *ret = (osc_saw_1_shared_t *)malloc(sizeof(osc_saw_1_shared_t));
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
   osc_saw_1_voice_t *voice = (osc_saw_1_voice_t *)malloc(sizeof(osc_saw_1_voice_t));
   if(NULL != voice)
   {
      memset((void*)voice, 0, sizeof(*voice));
      voice->base.info = _info;
      voice->tmp2_lfsr_state = 17545 * (_voiceIdx + 1u);
   }
   return &voice->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_saw_1_voice_t);

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
st_plugin_info_t *osc_saw_1_init(void) {
   osc_saw_1_info_t *ret = (osc_saw_1_info_t *)malloc(sizeof(osc_saw_1_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "osc_saw_1";  // unique id. don't change this in future builds.
      // (note) don't set author/name/short_name when CYCLE_SKIP_UI is defined ?
      ret->base.author      = "bsp";
      ret->base.name        = "saw osc 1";
      ret->base.short_name  = "saw osc 1";
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
         return osc_saw_1_init();
   }
   return NULL;
}
#endif // STFX_SKIP_MAIN_INIT

} // extern "C"