// ----
// ---- file   : osc_sine_v1.cpp
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
// ----           $ g++ -Wall -Wno-unused-function -Wno-unused-variable -I../../tksampler -c osc_sine_v1.cpp -o osc_sine_v1.o
// ---- created: 15Jan2024 21:19:06
// ----
// ----
// ----

#include <plugin.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>


#define OVERSAMPLE_FACTOR  2.0f

#define PARAM_AMP                0
#define PARAM_KBD_AMT            1
#define PARAM_FINE               2
#define PARAM_COARSE             3
#define PARAM_VSYNC_FINE         4
#define PARAM_VSYNC_COARSE       5
#define PARAM_VSYNC_AMT          6
#define PARAM_VSYNC_EXP          7
#define NUM_PARAMS               8
static const char *loc_param_names[NUM_PARAMS] = {
   "amp",                     // 0: AMP
   "kbd_amt",                 // 1: KBD_AMT
   "fine",                    // 2: FINE
   "coarse",                  // 3: COARSE
   "vsync_fine",              // 4: VSYNC_FINE
   "vsync_coarse",            // 5: VSYNC_COARSE
   "vsync_amt",               // 6: VSYNC_AMT
   "vsync_exp",               // 7: VSYNC_EXP

};
static float loc_param_resets[NUM_PARAMS] = {
   0.5f,                      // 0: AMP
   0.75f,                     // 1: KBD_AMT
   0.5f,                      // 2: FINE
   0.5f,                      // 3: COARSE
   0.5f,                      // 4: VSYNC_FINE
   0.0f,                      // 5: VSYNC_COARSE
   0.0f,                      // 6: VSYNC_AMT
   0.0f,                      // 7: VSYNC_EXP

};

#define MOD_AMP                  0
#define MOD_FINE                 1
#define MOD_COARSE               2
#define MOD_VSYNC_FINE           3
#define MOD_VSYNC_COARSE         4
#define MOD_VSYNC_AMT            5
#define MOD_VSYNC_EXP            6
#define NUM_MODS                 7
static const char *loc_mod_names[NUM_MODS] = {
   "amp",                  // 0: AMP
   "fine",                 // 1: FINE
   "coarse",               // 2: COARSE
   "vsync_fine",           // 3: VSYNC_FINE
   "vsync_coarse",         // 4: VSYNC_COARSE
   "vsync_amt",            // 5: VSYNC_AMT
   "vsync_exp",            // 6: VSYNC_EXP

};

typedef struct osc_sine_v1_info_s {
   st_plugin_info_t base;
} osc_sine_v1_info_t;

typedef struct osc_sine_v1_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];

} osc_sine_v1_shared_t;

typedef struct osc_sine_v1_voice_s {
   st_plugin_voice_t base;
   float             sample_rate;
   float             note_speed_fixed;
   float             note_speed_cur;
   float             note_speed_inc;
   float             note_cur;
   float             note_inc;
   float             mods[NUM_MODS];
   float mod_amp_cur;
   float mod_amp_inc;
   float mod_fine_cur;
   float mod_fine_inc;
   float mod_coarse_cur;
   float mod_coarse_inc;
   float mod_vsync_fine_cur;
   float mod_vsync_fine_inc;
   float mod_vsync_coarse_cur;
   float mod_vsync_coarse_inc;
   float mod_vsync_amt_cur;
   float mod_vsync_amt_inc;
   float mod_vsync_exp_cur;
   float mod_vsync_exp_inc;

   float tmp1;
   float tmp2;
   float tmp3;
   float tmp4_tri_phase;
   float tmp5;
   float tmp6;
   float tmp7_sin_phase;
   float tmp8_sin_phase;
   float tmp9;
   float tmp10;
   float var_x;
   float var_v_amp;
   float var_v_freq;
   float var_v_t;
   float var_v_xfade;

} osc_sine_v1_voice_t;

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
   ST_PLUGIN_VOICE_CAST(osc_sine_v1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_sine_v1_shared_t);

   float out = 0.0f;
   // -------- lane "prepare" modIdx=0
   
   // -- mod="2" dstVar=out
   out = 2.0f;
   
   // -- mod="pow" dstVar=out
   
   // ---- mod="pow" input "exp" seq 1/1
   
   // -- mod="$m_amp" dstVar=voice->tmp1/*exp*/
   voice->tmp1/*exp*/ = voice->mod_amp_cur;
   
   // -- mod="fma" dstVar=voice->tmp1/*exp*/
   voice->tmp1/*exp*/ = (voice->tmp1/*exp*/ * 8.0f) + -4.0f;
   out = mathPowerf(out, voice->tmp1/*exp*/);
   
   // -- mod="sto v_amp" dstVar=out
   voice->var_v_amp = out;
   
   // -- mod="$m_fine" dstVar=out
   out = voice->mod_fine_cur;
   
   // -- mod="fma" dstVar=out
   out = (out * 2.0f) + -1.0f;
   
   // -- mod="bts" dstVar=out
   out = loc_bipolar_to_scale(out, 2.0f, 2.0f);
   
   // -- mod="2" dstVar=out
   voice->tmp1/*seq*/ = 2.0f;
   
   // -- mod="pow" dstVar=voice->tmp1/*seq*/
   
   // ---- mod="pow" input "exp" seq 1/1
   
   // -- mod="$m_coarse" dstVar=voice->tmp2/*exp*/
   voice->tmp2/*exp*/ = voice->mod_coarse_cur;
   
   // -- mod="qua" dstVar=voice->tmp2/*exp*/
   voice->tmp2/*exp*/ = ((int)(voice->tmp2/*exp*/ * 64.0f) / 64.0f);
   
   // -- mod="fma" dstVar=voice->tmp2/*exp*/
   voice->tmp2/*exp*/ = (voice->tmp2/*exp*/ * 8.0f) + -4.0f;
   voice->tmp1/*seq*/ = mathPowerf(voice->tmp1/*seq*/, voice->tmp2/*exp*/);
   out *= voice->tmp1/*seq*/;
   
   // -- mod="kbd" dstVar=out
   
   // ---- mod="kbd" input "kbd" seq 1/1
   
   // -- mod="$p_kbd_amt" dstVar=voice->tmp1/*kbd*/
   voice->tmp1/*kbd*/ = shared->params[PARAM_KBD_AMT];
   
   // -- mod="fma" dstVar=voice->tmp1/*kbd*/
   voice->tmp1/*kbd*/ = (voice->tmp1/*kbd*/ * 4.0f) + -2.0f;
   voice->tmp2/*off*/ = 0;
   voice->tmp3/*rate*/ = (voice->note_cur + (voice->tmp2/*off*/ * 12.0f) - 60.0f) * voice->tmp1/*kbd*/ + 60.0f;
   voice->tmp3/*rate*/ = ((440.0f/32.0f)*expf( ((voice->tmp3/*rate*/-9.0f)/12.0f)*logf(2.0f) ));
   voice->tmp3/*rate*/ *= (1.0f / 261.626f);
   out *= voice->tmp3/*rate*/;
   
   // -- mod="sto v_freq" dstVar=out
   voice->var_v_freq = out;
} /* end prepare */

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

static const char *ST_PLUGIN_API loc_get_param_name(st_plugin_info_t *_info,
                                                    unsigned int      _paramIdx
                                                    ) {
   (void)_info;
   return loc_param_names[_paramIdx];
}

static const char *ST_PLUGIN_API loc_get_param_group_name(st_plugin_info_t *_info,
                                                          unsigned int      _paramGroupIdx
                                                          ) {
   (void)_info;
   const char *r = NULL;
   static const char *groupNames[4] = { "amp", "kbd", "pitch", "vsync" };
   if(_paramGroupIdx < 4u)
      r = groupNames[_paramGroupIdx];
   return r;
}

static unsigned int ST_PLUGIN_API loc_get_param_group_idx(st_plugin_info_t *_info,
                                                          unsigned int      _paramIdx
                                                          ) {
   (void)_info;
   unsigned int r = ~0u;
   static unsigned int groupIndices[NUM_PARAMS] = { 0, 1, 2, 2, 3, 3, 3, 3 };
   r = groupIndices[_paramIdx];
   return r;
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
   ST_PLUGIN_SHARED_CAST(osc_sine_v1_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(osc_sine_v1_shared_t);
   shared->params[_paramIdx] = _value;
}

static unsigned int ST_PLUGIN_API loc_query_dynamic_param_preset_values(st_plugin_shared_t *_shared,
                                                                        const unsigned int  _paramIdx,
                                                                        float              *_retValues,
                                                                        const unsigned int  _retValuesSize
                                                                        ) {
   ST_PLUGIN_SHARED_CAST(osc_sine_v1_shared_t);
   unsigned int r = 0u;
   return r;
}

static unsigned int ST_PLUGIN_API loc_query_dynamic_param_preset_name(st_plugin_shared_t *_shared,
                                                                      const unsigned int  _paramIdx,
                                                                      const unsigned int  _presetIdx,
                                                                      char               *_retBuf,
                                                                      const unsigned int  _retBufSize
                                                                      ) {
   ST_PLUGIN_SHARED_CAST(osc_sine_v1_shared_t);
   unsigned int r = 0u;
   return r;
}

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

static float * ST_PLUGIN_API loc_get_array_param_variation_ptr(st_plugin_shared_t *_shared,
                                                               const unsigned int  _paramIdx,
                                                               const unsigned int  _variationIdx
                                                               ) {
   ST_PLUGIN_SHARED_CAST(osc_sine_v1_shared_t);
   float *r = NULL;
   switch(_paramIdx)
   {
      default:
         break;
   }
   return r;
}

static void ST_PLUGIN_API loc_set_array_param_edit_variation_idx(st_plugin_shared_t *_shared,
                                                                 const unsigned int  _paramIdx,
                                                                 const int           _variationIdx
                                                                 ) {
   ST_PLUGIN_SHARED_CAST(osc_sine_v1_shared_t);
   switch(_paramIdx)
   {
      default:
         break;
   }
}

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

static const char *ST_PLUGIN_API loc_get_mod_name(st_plugin_info_t *_info,
                                                  unsigned int      _modIdx
                                                  ) {
   (void)_info;
   return loc_mod_names[_modIdx];
}

static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(osc_sine_v1_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(osc_sine_v1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_sine_v1_shared_t);
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


      voice->tmp4_tri_phase = 0.0f;
      voice->tmp7_sin_phase = 0.0f;
      voice->tmp8_sin_phase = 0.0f;

      voice->var_x = 0.0f;
      voice->var_v_amp = 0.0f;
      voice->var_v_freq = 0.0f;
      voice->var_v_t = 0.0f;
      voice->var_v_xfade = 0.0f;



   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(osc_sine_v1_voice_t);
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
   ST_PLUGIN_VOICE_CAST(osc_sine_v1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_sine_v1_shared_t);
   (void)_note;
   (void)_vol;
   (void)_pan;

#ifdef OVERSAMPLE_FACTOR
   float noteSpeed = _freqHz / (voice->sample_rate * OVERSAMPLE_FACTOR);
#else
   float noteSpeed = _freqHz / voice->sample_rate;
#endif // OVERSAMPLE_FACTOR

   float modamp          = shared->params[PARAM_AMP         ]                       + voice->mods[MOD_AMP          ];
   float modfine         = shared->params[PARAM_FINE        ]                       + voice->mods[MOD_FINE         ];
   float modcoarse       = shared->params[PARAM_COARSE      ]                       + voice->mods[MOD_COARSE       ];
   float modvsync_fine   = shared->params[PARAM_VSYNC_FINE  ]                       + voice->mods[MOD_VSYNC_FINE   ];
   float modvsync_coarse = shared->params[PARAM_VSYNC_COARSE]                       + voice->mods[MOD_VSYNC_COARSE ];
   float modvsync_amt    = shared->params[PARAM_VSYNC_AMT   ]                       + voice->mods[MOD_VSYNC_AMT    ];
   float modvsync_exp    = shared->params[PARAM_VSYNC_EXP   ]                       + voice->mods[MOD_VSYNC_EXP    ];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->note_speed_inc = (noteSpeed - voice->note_speed_cur) * recBlockSize;
      voice->note_inc       = (_note - voice->note_cur) * recBlockSize;
      voice->mod_amp_inc          = (modamp             - voice->mod_amp_cur           ) * recBlockSize;
      voice->mod_fine_inc         = (modfine            - voice->mod_fine_cur          ) * recBlockSize;
      voice->mod_coarse_inc       = (modcoarse          - voice->mod_coarse_cur        ) * recBlockSize;
      voice->mod_vsync_fine_inc   = (modvsync_fine      - voice->mod_vsync_fine_cur    ) * recBlockSize;
      voice->mod_vsync_coarse_inc = (modvsync_coarse    - voice->mod_vsync_coarse_cur  ) * recBlockSize;
      voice->mod_vsync_amt_inc    = (modvsync_amt       - voice->mod_vsync_amt_cur     ) * recBlockSize;
      voice->mod_vsync_exp_inc    = (modvsync_exp       - voice->mod_vsync_exp_cur     ) * recBlockSize;

      loc_prepare(&voice->base);
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->note_speed_cur = noteSpeed;
      voice->note_speed_inc = 0.0f;
      voice->note_cur       = _note;
      voice->note_inc       = 0.0f;
      voice->mod_amp_cur          = modamp;
      voice->mod_amp_inc          = 0.0f;
      voice->mod_fine_cur         = modfine;
      voice->mod_fine_inc         = 0.0f;
      voice->mod_coarse_cur       = modcoarse;
      voice->mod_coarse_inc       = 0.0f;
      voice->mod_vsync_fine_cur   = modvsync_fine;
      voice->mod_vsync_fine_inc   = 0.0f;
      voice->mod_vsync_coarse_cur = modvsync_coarse;
      voice->mod_vsync_coarse_inc = 0.0f;
      voice->mod_vsync_amt_cur    = modvsync_amt;
      voice->mod_vsync_amt_inc    = 0.0f;
      voice->mod_vsync_exp_cur    = modvsync_exp;
      voice->mod_vsync_exp_inc    = 0.0f;

      loc_prepare(&voice->base);
   }

   // printf("xxx note_cur=%f\n", voice->note_cur);
   // printf("xxx prepare_block: numFrames=%u moda=%f\n", _numFrames, moda);
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut,
                                              unsigned int        _numFrames
                                              ) {
   (void)_bMonoIn;
   (void)_samplesIn;

   ST_PLUGIN_VOICE_CAST(osc_sine_v1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_sine_v1_shared_t);

   // Mono output (replicate left to right channel)
   unsigned int j = 0u;
   unsigned int jStep = _bMonoIn ? 1u : 2u;
   unsigned int k = 0u;
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float out = _samplesIn[j];
      j += jStep;
#ifdef OVERSAMPLE_FACTOR
      float outOS = 0.0f;
#endif // OVERSAMPLE_FACTOR

#ifdef OVERSAMPLE_FACTOR
      for(unsigned int osi = 0u; osi < OVERSAMPLE_FACTOR; osi++)
#endif // OVERSAMPLE_FACTOR
      {
         float tmp_f;
         float tmp2_f;
         
         // ========
         // ======== lane "out" modIdx=0
         // ========
         
         // -- mod="tri" dstVar=out
         
         // ---- mod="tri" input "freq" seq 1/1
         
         // -- mod="$v_freq" dstVar=voice->tmp6/*tri_freq*/
         voice->tmp6/*tri_freq*/ = voice->var_v_freq;
         voice->tmp5/*tri_speed*/ = voice->note_speed_fixed * voice->tmp6/*tri_freq*/;
         voice->tmp6/*tri_tmp*/ = (voice->tmp4_tri_phase);
         voice->tmp6/*tri_tmp*/ = ffrac_s(voice->tmp6/*tri_tmp*/);
         out = (voice->tmp6/*tri_tmp*/ < 0.5f) ? (-1.0 + voice->tmp6/*tri_tmp*/ * 4.0f) : (1.0 - (voice->tmp6/*tri_tmp*/ - 0.5f)*4);
         voice->tmp4_tri_phase = ffrac_s(voice->tmp4_tri_phase + voice->tmp5/*tri_speed*/);
         
         // -- mod="fma" dstVar=out
         out = (out * 0.5f) + 0.5f;
         
         // -- mod="if" dstVar=out
         
         // ---- mod="if" input "then" seq 1/1
         
         // -- mod="pow" dstVar=out
         
         // ---- mod="pow" input "exp" seq 1/1
         
         // -- mod="$m_vsync_exp" dstVar=voice->tmp5/*exp*/
         voice->tmp5/*exp*/ = voice->mod_vsync_exp_cur;
         
         // -- mod="15" dstVar=voice->tmp5/*exp*/
         voice->tmp6/*seq*/ = 15.0f;
         voice->tmp5/*exp*/ *= voice->tmp6/*seq*/;
         
         // -- mod="1" dstVar=voice->tmp5/*exp*/
         voice->tmp6/*seq*/ = 1.0f;
         voice->tmp5/*exp*/ += voice->tmp6/*seq*/;
         out = mathPowerf(out, voice->tmp5/*exp*/);
         
         // -- mod="$m_vsync_amt" dstVar=out
         voice->tmp5/*seq*/ = voice->mod_vsync_amt_cur;
         out *= voice->tmp5/*seq*/;
         
         // -- mod="sto v_xfade" dstVar=out
         voice->var_v_xfade = out;
         
         // -- mod="sin" dstVar=out
         
         // ---- mod="sin" input "freq" seq 1/1
         
         // -- mod="$v_freq" dstVar=voice->tmp6/*sin_freq*/
         voice->tmp6/*sin_freq*/ = voice->var_v_freq;
         voice->tmp5/*sin_speed*/ = voice->note_speed_fixed * voice->tmp6/*sin_freq*/;
         voice->tmp6/*sin_tmp*/ = (voice->tmp7_sin_phase);
         voice->tmp6/*sin_tmp*/ = ffrac_s(voice->tmp6/*sin_tmp*/);
         out = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp6/*sin_tmp*/)&16383u];
         voice->tmp7_sin_phase = ffrac_s(voice->tmp7_sin_phase + voice->tmp5/*sin_speed*/);
         
         // -- mod="sto v_t" dstVar=out
         voice->var_v_t = out;
         
         // -- mod="sin" dstVar=out
         
         // ---- mod="sin" input "freq" seq 1/1
         
         // -- mod="$v_freq" dstVar=voice->tmp6/*sin_freq*/
         voice->tmp6/*sin_freq*/ = voice->var_v_freq;
         voice->tmp5/*sin_speed*/ = voice->note_speed_fixed * voice->tmp6/*sin_freq*/;
         
         // ---- mod="sin" input "vsync" seq 1/1
         
         // -- mod="$m_vsync_coarse" dstVar=voice->tmp9/*vsync*/
         voice->tmp9/*vsync*/ = voice->mod_vsync_coarse_cur;
         
         // -- mod="qua" dstVar=voice->tmp9/*vsync*/
         voice->tmp9/*vsync*/ = ((int)(voice->tmp9/*vsync*/ * 64.0f) / 64.0f);
         
         // -- mod="16" dstVar=voice->tmp9/*vsync*/
         voice->tmp10/*seq*/ = 16.0f;
         voice->tmp9/*vsync*/ *= voice->tmp10/*seq*/;
         
         // -- mod="$m_vsync_fine" dstVar=voice->tmp9/*vsync*/
         voice->tmp10/*seq*/ = voice->mod_vsync_fine_cur;
         
         // -- mod="fma" dstVar=voice->tmp10/*seq*/
         voice->tmp10/*seq*/ = (voice->tmp10/*seq*/ * 4.0f) + -2.0f;
         voice->tmp9/*vsync*/ += voice->tmp10/*seq*/;
         voice->tmp6/*sin_tmp*/ = (voice->tmp8_sin_phase);
         voice->tmp6/*sin_tmp*/ = voice->tmp6/*sin_tmp*/ * voice->tmp9/*vsync*/;
         voice->tmp6/*sin_tmp*/ = ffrac_s(voice->tmp6/*sin_tmp*/);
         out = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp6/*sin_tmp*/)&16383u];
         voice->tmp8_sin_phase = ffrac_s(voice->tmp8_sin_phase + voice->tmp5/*sin_speed*/);
         
         // -- mod="ipl" dstVar=out
         
         // ---- mod="ipl" input "a" seq 1/1
         
         // -- mod="$v_t" dstVar=voice->tmp5/*a*/
         voice->tmp5/*a*/ = voice->var_v_t;
         
         // ---- mod="ipl" input "t" seq 1/1
         
         // -- mod="$v_xfade" dstVar=voice->tmp6/*t*/
         voice->tmp6/*t*/ = voice->var_v_xfade;
         out = voice->tmp5/*a*/ + (out - voice->tmp5/*a*/) * voice->tmp6/*t*/;
         
         // -- mod="$v_amp" dstVar=out
         voice->tmp5/*seq*/ = voice->var_v_amp;
         out *= voice->tmp5/*seq*/;
         
         // -- mod="clp" dstVar=out
         if(out > 0.999f) out = 0.999f;
         else if(out < -0.999f) out = -0.999f;
   
         /* end calc */

#ifdef OVERSAMPLE_FACTOR
         outOS += out;
#endif // OVERSAMPLE_FACTOR
      }
#ifdef OVERSAMPLE_FACTOR
      // Apply lowpass filter before downsampling
      //   (note) normalized Fc = F/Fs = 0.442947 / sqrt(oversample_factor� - 1)
      out = outOS * (1.0f / OVERSAMPLE_FACTOR);
#endif // OVERSAMPLE_FACTOR
      _samplesOut[k]      = out;
      _samplesOut[k + 1u] = out;

      // Next frame
      k += 2u;
      voice->note_speed_cur += voice->note_speed_inc;
      voice->note_cur       += voice->note_inc;
      voice->mod_amp_cur           += voice->mod_amp_inc;
      voice->mod_fine_cur          += voice->mod_fine_inc;
      voice->mod_coarse_cur        += voice->mod_coarse_inc;
      voice->mod_vsync_fine_cur    += voice->mod_vsync_fine_inc;
      voice->mod_vsync_coarse_cur  += voice->mod_vsync_coarse_inc;
      voice->mod_vsync_amt_cur     += voice->mod_vsync_amt_inc;
      voice->mod_vsync_exp_cur     += voice->mod_vsync_exp_inc;
   } /* loop numFrames */
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   osc_sine_v1_shared_t *ret = (osc_sine_v1_shared_t *)malloc(sizeof(osc_sine_v1_shared_t));
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
   osc_sine_v1_voice_t *voice = (osc_sine_v1_voice_t *)malloc(sizeof(osc_sine_v1_voice_t));
   if(NULL != voice)
   {
      memset((void*)voice, 0, sizeof(*voice));
      voice->base.info = _info;
   }
   return &voice->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_sine_v1_voice_t);

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

extern "C" {
st_plugin_info_t *osc_sine_v1_init(void) {
   osc_sine_v1_info_t *ret = (osc_sine_v1_info_t *)malloc(sizeof(osc_sine_v1_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "osc_sine_v1";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "osc sine v1";
      ret->base.short_name  = "osc sine v1";
      ret->base.flags       = ST_PLUGIN_FLAG_OSC;
      ret->base.category    = ST_PLUGIN_CAT_UNKNOWN;
      ret->base.num_params  = NUM_PARAMS;
      ret->base.num_mods    = NUM_MODS;

      ret->base.shared_new                          = &loc_shared_new;
      ret->base.shared_delete                       = &loc_shared_delete;
      ret->base.voice_new                           = &loc_voice_new;
      ret->base.voice_delete                        = &loc_voice_delete;
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
      ret->base.get_array_param_element_value_range = &loc_get_array_param_element_value_range;
      ret->base.get_param_value                     = &loc_get_param_value;
      ret->base.set_param_value                     = &loc_set_param_value;
      ret->base.get_mod_name                        = &loc_get_mod_name;
      ret->base.set_sample_rate                     = &loc_set_sample_rate;
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
         return osc_sine_v1_init();
   }
   return NULL;
}
#endif // STFX_SKIP_MAIN_INIT

} // extern "C"
