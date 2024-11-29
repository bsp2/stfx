// ----
// ---- file   : osc_fof_v1.cpp
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
// ----           $ g++ -Wall -Wno-unused-function -Wno-unused-variable -I../../tksampler -c osc_fof_v1.cpp -o osc_fof_v1.o
// ---- created: 08Jan2024 19:59:37
// ----
// ----
// ----

#include <plugin.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>


#define OVERSAMPLE_FACTOR  4.0f

#define PARAM_FUND_LEVEL         0
#define PARAM_FUND_COLOR         1
#define PARAM_LEVEL              2
#define PARAM_COLOR              3
#define PARAM_RATIO              4
#define PARAM_DAMPEN             5
#define PARAM_WARP               6
#define NUM_PARAMS               7
static const char *loc_param_names[NUM_PARAMS] = {
   "fund_level",              // 0: FUND_LEVEL
   "fund_color",              // 1: FUND_COLOR
   "level",                   // 2: LEVEL
   "color",                   // 3: COLOR
   "ratio",                   // 4: RATIO
   "dampen",                  // 5: DAMPEN
   "warp",                    // 6: WARP

};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,                      // 0: FUND_LEVEL
   0.5f,                      // 1: FUND_COLOR
   0.5f,                      // 2: LEVEL
   0.5f,                      // 3: COLOR
   0.0f,                      // 4: RATIO
   0.5f,                      // 5: DAMPEN
   0.5f,                      // 6: WARP

};

#define MOD_FUND_LEVEL           0
#define MOD_FUND_COLOR           1
#define MOD_LEVEL                2
#define MOD_COLOR                3
#define MOD_RATIO                4
#define MOD_DAMPEN               5
#define MOD_WARP                 6
#define NUM_MODS                 7
static const char *loc_mod_names[NUM_MODS] = {
   "fund_level",           // 0: FUND_LEVEL
   "fund_color",           // 1: FUND_COLOR
   "level",                // 2: LEVEL
   "color",                // 3: COLOR
   "ratio",                // 4: RATIO
   "dampen",               // 5: DAMPEN
   "warp",                 // 6: WARP

};

typedef struct osc_fof_v1_info_s {
   st_plugin_info_t base;
} osc_fof_v1_info_t;

typedef struct osc_fof_v1_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];

} osc_fof_v1_shared_t;

typedef struct osc_fof_v1_voice_s {
   st_plugin_voice_t base;
   float             sample_rate;
   float             note_speed_fixed;
   float             note_speed_cur;
   float             note_speed_inc;
   float             note_cur;
   float             note_inc;
   float             mods[NUM_MODS];
   float mod_fund_level_cur;
   float mod_fund_level_inc;
   float mod_fund_color_cur;
   float mod_fund_color_inc;
   float mod_level_cur;
   float mod_level_inc;
   float mod_color_cur;
   float mod_color_inc;
   float mod_ratio_cur;
   float mod_ratio_inc;
   float mod_dampen_cur;
   float mod_dampen_inc;
   float mod_warp_cur;
   float mod_warp_inc;

   float tmp1;
   float tmp2_pha_phase;
   float tmp3;
   float tmp4;
   float tmp5;
   float tmp6_sin_phase;
   float tmp7_sin_phase;
   float tmp8;
   float tmp9;
   float tmp10;
   float tmp11;
   short tmp12;
   short tmp13;
   float tmp14;
   float tmp15;
   float tmp16;
   float var_x;
   float var_v_fund_phase;
   float var_v_freq;
   float var_v_dampen;
   float var_v_warp;

} osc_fof_v1_voice_t;

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
static short curve_0[32] = {
2021, 2016, 2008, 1995, 1979, 1960, 1938, 1913, 1886, 1856, 1823, 1787, 1747, 1705, 1659, 1608, 1552, 1490, 1420, 1337, 1237, 1095, 902, 729, 574, 437, 317, 214, 129, 62, 15, -11, 
};
static short curve_1[32] = {
2048, 2034, 2010, 1976, 1934, 1882, 1823, 1755, 1681, 1601, 1514, 1424, 1330, 1232, 1133, 1034, 934, 835, 739, 645, 556, 470, 390, 316, 249, 189, 136, 92, 56, 28, 9, 0, 
};

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
   ST_PLUGIN_VOICE_CAST(osc_fof_v1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_fof_v1_shared_t);

   float out = 0.0f;
   // -------- lane "prepare" modIdx=0
   
   // -- mod="$p_ratio" dstVar=out
   out = shared->params[PARAM_RATIO];
   
   // -- mod="$m_ratio" dstVar=out
   voice->tmp1/*seq*/ = voice->mod_ratio_cur;
   out += voice->tmp1/*seq*/;
   
   // -- mod="bts" dstVar=out
   out = loc_bipolar_to_scale(out, 2.0f, 64.0f);
   
   // -- mod="sto v_freq" dstVar=out
   voice->var_v_freq = out;
   
   // -- mod="$p_dampen" dstVar=out
   out = shared->params[PARAM_DAMPEN];
   
   // -- mod="2" dstVar=out
   voice->tmp1/*seq*/ = 2.0f;
   out *= voice->tmp1/*seq*/;
   
   // -- mod="1" dstVar=out
   voice->tmp1/*seq*/ = 1.0f;
   out -= voice->tmp1/*seq*/;
   
   // -- mod="$m_dampen" dstVar=out
   voice->tmp1/*seq*/ = voice->mod_dampen_cur;
   out += voice->tmp1/*seq*/;
   
   // -- mod="bts" dstVar=out
   out = loc_bipolar_to_scale(out, 16.0f, 16.0f);
   
   // -- mod="sto v_dampen" dstVar=out
   voice->var_v_dampen = out;
   
   // -- mod="$p_warp" dstVar=out
   out = shared->params[PARAM_WARP];
   
   // -- mod="2" dstVar=out
   voice->tmp1/*seq*/ = 2.0f;
   out *= voice->tmp1/*seq*/;
   
   // -- mod="1" dstVar=out
   voice->tmp1/*seq*/ = 1.0f;
   out -= voice->tmp1/*seq*/;
   
   // -- mod="$m_warp" dstVar=out
   voice->tmp1/*seq*/ = voice->mod_warp_cur;
   out += voice->tmp1/*seq*/;
   
   // -- mod="bts" dstVar=out
   out = loc_bipolar_to_scale(out, 8.0f, 8.0f);
   
   // -- mod="sto v_warp" dstVar=out
   voice->var_v_warp = out;
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
   return r;
}

static unsigned int ST_PLUGIN_API loc_get_param_group_idx(st_plugin_info_t *_info,
                                                          unsigned int      _paramIdx
                                                          ) {
   (void)_info;
   unsigned int r = ~0u;
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
   ST_PLUGIN_SHARED_CAST(osc_fof_v1_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(osc_fof_v1_shared_t);
   shared->params[_paramIdx] = _value;
}

static unsigned int ST_PLUGIN_API loc_query_dynamic_param_preset_values(st_plugin_shared_t *_shared,
                                                                        const unsigned int  _paramIdx,
                                                                        float              *_retValues,
                                                                        const unsigned int  _retValuesSize
                                                                        ) {
   ST_PLUGIN_SHARED_CAST(osc_fof_v1_shared_t);
   unsigned int r = 0u;
   return r;
}

static unsigned int ST_PLUGIN_API loc_query_dynamic_param_preset_name(st_plugin_shared_t *_shared,
                                                                      const unsigned int  _paramIdx,
                                                                      const unsigned int  _presetIdx,
                                                                      char               *_retBuf,
                                                                      const unsigned int  _retBufSize
                                                                      ) {
   ST_PLUGIN_SHARED_CAST(osc_fof_v1_shared_t);
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
   ST_PLUGIN_SHARED_CAST(osc_fof_v1_shared_t);
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
   ST_PLUGIN_SHARED_CAST(osc_fof_v1_shared_t);
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
   ST_PLUGIN_VOICE_CAST(osc_fof_v1_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(osc_fof_v1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_fof_v1_shared_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
#ifdef OVERSAMPLE_FACTOR
      voice->note_speed_fixed = (0.5f / (voice->sample_rate * OVERSAMPLE_FACTOR));
#else
      voice->note_speed_fixed = (0.5f / voice->sample_rate);
#endif // OVERSAMPLE_FACTOR


      voice->tmp2_pha_phase = 0.0f;
      voice->tmp6_sin_phase = 0.0f;
      voice->tmp7_sin_phase = 0.0f;

      voice->var_x = 0.0f;
      voice->var_v_fund_phase = 0.0f;
      voice->var_v_freq = 0.0f;
      voice->var_v_dampen = 0.0f;
      voice->var_v_warp = 0.0f;



   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(osc_fof_v1_voice_t);
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
   ST_PLUGIN_VOICE_CAST(osc_fof_v1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_fof_v1_shared_t);
   (void)_note;
   (void)_vol;
   (void)_pan;

#ifdef OVERSAMPLE_FACTOR
   float noteSpeed = _freqHz / (voice->sample_rate * OVERSAMPLE_FACTOR);
#else
   float noteSpeed = _freqHz / voice->sample_rate;
#endif // OVERSAMPLE_FACTOR

   float modfund_level   = shared->params[PARAM_FUND_LEVEL  ] * 2.0f - 1.0f         + voice->mods[MOD_FUND_LEVEL   ];
   float modfund_color   = shared->params[PARAM_FUND_COLOR  ] * 16.0f - 8.0f        + voice->mods[MOD_FUND_COLOR   ];
   float modlevel        = shared->params[PARAM_LEVEL       ] * 2.0f - 1.0f         + voice->mods[MOD_LEVEL        ];
   float modcolor        = shared->params[PARAM_COLOR       ] * 16.0f - 8.0f        + voice->mods[MOD_COLOR        ];
   float modratio        = shared->params[PARAM_RATIO       ]                       + voice->mods[MOD_RATIO        ];
   float moddampen       = shared->params[PARAM_DAMPEN      ]                       + voice->mods[MOD_DAMPEN       ];
   float modwarp         = shared->params[PARAM_WARP        ]                       + voice->mods[MOD_WARP         ];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->note_speed_inc = (noteSpeed - voice->note_speed_cur) * recBlockSize;
      voice->note_inc       = (_note - voice->note_cur) * recBlockSize;
      voice->mod_fund_level_inc   = (modfund_level      - voice->mod_fund_level_cur    ) * recBlockSize;
      voice->mod_fund_color_inc   = (modfund_color      - voice->mod_fund_color_cur    ) * recBlockSize;
      voice->mod_level_inc        = (modlevel           - voice->mod_level_cur         ) * recBlockSize;
      voice->mod_color_inc        = (modcolor           - voice->mod_color_cur         ) * recBlockSize;
      voice->mod_ratio_inc        = (modratio           - voice->mod_ratio_cur         ) * recBlockSize;
      voice->mod_dampen_inc       = (moddampen          - voice->mod_dampen_cur        ) * recBlockSize;
      voice->mod_warp_inc         = (modwarp            - voice->mod_warp_cur          ) * recBlockSize;

      loc_prepare(&voice->base);
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->note_speed_cur = noteSpeed;
      voice->note_speed_inc = 0.0f;
      voice->note_cur       = _note;
      voice->note_inc       = 0.0f;
      voice->mod_fund_level_cur   = modfund_level;
      voice->mod_fund_level_inc   = 0.0f;
      voice->mod_fund_color_cur   = modfund_color;
      voice->mod_fund_color_inc   = 0.0f;
      voice->mod_level_cur        = modlevel;
      voice->mod_level_inc        = 0.0f;
      voice->mod_color_cur        = modcolor;
      voice->mod_color_inc        = 0.0f;
      voice->mod_ratio_cur        = modratio;
      voice->mod_ratio_inc        = 0.0f;
      voice->mod_dampen_cur       = moddampen;
      voice->mod_dampen_inc       = 0.0f;
      voice->mod_warp_cur         = modwarp;
      voice->mod_warp_inc         = 0.0f;

      loc_prepare(&voice->base);
   }

   // printf("xxx note_cur=%f\n", voice->note_cur);
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut,
                                              unsigned int        _numFrames
                                              ) {
   (void)_bMonoIn;
   (void)_samplesIn;

   ST_PLUGIN_VOICE_CAST(osc_fof_v1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_fof_v1_shared_t);

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
         
         // -- mod="pha" dstVar=out
         voice->tmp5/*freq*/ = 1;
         voice->tmp3/*pha_speed*/ = voice->note_speed_cur * voice->tmp5/*freq*/;
         voice->tmp4/*pha_tmp*/ = ffrac_s(voice->tmp2_pha_phase);
         out = voice->tmp4/*pha_tmp*/;
         voice->tmp2_pha_phase = ffrac_s(voice->tmp2_pha_phase + voice->tmp3/*pha_speed*/);
         
         // -- mod="sto v_fund_phase" dstVar=out
         voice->var_v_fund_phase = out;
         
         // -- mod="sin" dstVar=out
         
         // ---- mod="sin" input "freq" seq 1/1
         
         // -- mod="0" dstVar=voice->tmp4/*freq*/
         voice->tmp4/*freq*/ = 0.0f;
         voice->tmp3/*sin_speed*/ = voice->note_speed_cur * voice->tmp4/*freq*/;
         
         // ---- mod="sin" input "phase" seq 1/1
         
         // -- mod="$v_fund_phase" dstVar=voice->tmp5/*phase*/
         voice->tmp5/*phase*/ = voice->var_v_fund_phase;
         voice->tmp4/*sin_tmp*/ = ((voice->tmp6_sin_phase + voice->tmp5/*phase*/));
         voice->tmp4/*sin_tmp*/ = ffrac_s(voice->tmp4/*sin_tmp*/);
         out = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp4/*sin_tmp*/)&16383u];
         
         // -- mod="lle" dstVar=out
         
         // ---- mod="lle" input "c" seq 1/1
         
         // -- mod="$p_fund_color" dstVar=voice->tmp3/*c*/
         voice->tmp3/*c*/ = shared->params[PARAM_FUND_COLOR] * 16.0f - 8.0f;
         
         // -- mod="$m_fund_color" dstVar=voice->tmp3/*c*/
         voice->tmp4/*seq*/ = voice->mod_fund_color_cur;
         voice->tmp3/*c*/ += voice->tmp4/*seq*/;
         out = mathLogLinExpf(out, voice->tmp3/*c*/);
         
         // -- mod="$p_fund_level" dstVar=out
         voice->tmp3/*seq*/ = shared->params[PARAM_FUND_LEVEL] * 2.0f - 1.0f;
         
         // -- mod="$m_fund_level" dstVar=voice->tmp3/*seq*/
         voice->tmp4/*seq*/ = voice->mod_fund_level_cur;
         voice->tmp3/*seq*/ += voice->tmp4/*seq*/;
         out *= voice->tmp3/*seq*/;
         
         // -- mod="sin" dstVar=out
         
         // ---- mod="sin" input "freq" seq 1/1
         
         // -- mod="0" dstVar=voice->tmp5/*freq*/
         voice->tmp5/*freq*/ = 0.0f;
         voice->tmp4/*sin_speed*/ = voice->note_speed_cur * voice->tmp5/*freq*/;
         
         // ---- mod="sin" input "phase" seq 1/1
         
         // -- mod="$v_fund_phase" dstVar=voice->tmp8/*phase*/
         voice->tmp8/*phase*/ = voice->var_v_fund_phase;
         
         // -- mod="$v_fund_phase" dstVar=voice->tmp8/*phase*/
         voice->tmp9/*seq*/ = voice->var_v_fund_phase;
         
         // -- mod="$v_freq" dstVar=voice->tmp9/*seq*/
         voice->tmp10/*seq*/ = voice->var_v_freq;
         voice->tmp9/*seq*/ *= voice->tmp10/*seq*/;
         
         // -- mod="pow" dstVar=voice->tmp9/*seq*/
         
         // ---- mod="pow" input "exp" seq 1/1
         
         // -- mod="$v_fund_phase" dstVar=voice->tmp10/*exp*/
         voice->tmp10/*exp*/ = voice->var_v_fund_phase;
         
         // -- mod="lut" dstVar=voice->tmp10/*exp*/
         voice->tmp11/*lut_f*/ = (voice->tmp10/*exp*/ * 31);
         voice->tmp12/*lut_idx_a*/ = (int)voice->tmp11/*lut_f*/;
         voice->tmp14/*lut_frac*/ = voice->tmp11/*lut_f*/ - (float)voice->tmp12/*lut_idx_a*/;
         voice->tmp15/*lut_a*/ = curve_1[( (unsigned int)voice->tmp12/*lut_idx_a*/      ) & 31] * (1.0f / 2048);
         voice->tmp16/*lut_b*/ = curve_1[(((unsigned int)voice->tmp12/*lut_idx_a*/) + 1u) & 31] * (1.0f / 2048);
         voice->tmp10/*exp*/ = voice->tmp15/*lut_a*/ + (voice->tmp16/*lut_b*/ - voice->tmp15/*lut_a*/) * voice->tmp14/*lut_frac*/;
         
         // -- mod="$v_warp" dstVar=voice->tmp10/*exp*/
         voice->tmp11/*seq*/ = voice->var_v_warp;
         voice->tmp10/*exp*/ *= voice->tmp11/*seq*/;
         voice->tmp9/*seq*/ = mathPowerf(voice->tmp9/*seq*/, voice->tmp10/*exp*/);
         voice->tmp8/*phase*/ *= voice->tmp9/*seq*/;
         voice->tmp5/*sin_tmp*/ = ((voice->tmp7_sin_phase + voice->tmp8/*phase*/));
         voice->tmp5/*sin_tmp*/ = ffrac_s(voice->tmp5/*sin_tmp*/);
         voice->tmp3/*seq*/ = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp5/*sin_tmp*/)&16383u];
         
         // -- mod="lle" dstVar=voice->tmp3/*seq*/
         
         // ---- mod="lle" input "c" seq 1/1
         
         // -- mod="$p_color" dstVar=voice->tmp4/*c*/
         voice->tmp4/*c*/ = shared->params[PARAM_COLOR] * 16.0f - 8.0f;
         
         // -- mod="$m_color" dstVar=voice->tmp4/*c*/
         voice->tmp5/*seq*/ = voice->mod_color_cur;
         voice->tmp4/*c*/ += voice->tmp5/*seq*/;
         voice->tmp3/*seq*/ = mathLogLinExpf(voice->tmp3/*seq*/, voice->tmp4/*c*/);
         
         // -- mod="$p_level" dstVar=voice->tmp3/*seq*/
         voice->tmp4/*seq*/ = shared->params[PARAM_LEVEL] * 2.0f - 1.0f;
         
         // -- mod="$m_level" dstVar=voice->tmp4/*seq*/
         voice->tmp5/*seq*/ = voice->mod_level_cur;
         voice->tmp4/*seq*/ += voice->tmp5/*seq*/;
         
         // -- mod="$v_fund_phase" dstVar=voice->tmp4/*seq*/
         voice->tmp5/*seq*/ = voice->var_v_fund_phase;
         
         // -- mod="lut" dstVar=voice->tmp5/*seq*/
         voice->tmp8/*lut_f*/ = (voice->tmp5/*seq*/ * 31);
         voice->tmp12/*lut_idx_a*/ = (int)voice->tmp8/*lut_f*/;
         voice->tmp9/*lut_frac*/ = voice->tmp8/*lut_f*/ - (float)voice->tmp12/*lut_idx_a*/;
         voice->tmp10/*lut_a*/ = curve_0[( (unsigned int)voice->tmp12/*lut_idx_a*/      ) & 31] * (1.0f / 2048);
         voice->tmp11/*lut_b*/ = curve_0[(((unsigned int)voice->tmp12/*lut_idx_a*/) + 1u) & 31] * (1.0f / 2048);
         voice->tmp5/*seq*/ = voice->tmp10/*lut_a*/ + (voice->tmp11/*lut_b*/ - voice->tmp10/*lut_a*/) * voice->tmp9/*lut_frac*/;
         
         // -- mod="pow" dstVar=voice->tmp5/*seq*/
         
         // ---- mod="pow" input "exp" seq 1/1
         
         // -- mod="$v_dampen" dstVar=voice->tmp8/*exp*/
         voice->tmp8/*exp*/ = voice->var_v_dampen;
         voice->tmp5/*seq*/ = mathPowerf(voice->tmp5/*seq*/, voice->tmp8/*exp*/);
         voice->tmp4/*seq*/ *= voice->tmp5/*seq*/;
         voice->tmp3/*seq*/ *= voice->tmp4/*seq*/;
         out += voice->tmp3/*seq*/;
         
         // -- mod="$P_AMP" dstVar=out
         voice->tmp3/*seq*/ = 0.7f;
         out *= voice->tmp3/*seq*/;
         
         // -- mod="tanh" dstVar=out
         out = tanhf(out);
   
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
      voice->mod_fund_level_cur    += voice->mod_fund_level_inc;
      voice->mod_fund_color_cur    += voice->mod_fund_color_inc;
      voice->mod_level_cur         += voice->mod_level_inc;
      voice->mod_color_cur         += voice->mod_color_inc;
      voice->mod_ratio_cur         += voice->mod_ratio_inc;
      voice->mod_dampen_cur        += voice->mod_dampen_inc;
      voice->mod_warp_cur          += voice->mod_warp_inc;
   } /* loop numFrames */
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   osc_fof_v1_shared_t *ret = (osc_fof_v1_shared_t *)malloc(sizeof(osc_fof_v1_shared_t));
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
   osc_fof_v1_voice_t *voice = (osc_fof_v1_voice_t *)malloc(sizeof(osc_fof_v1_voice_t));
   if(NULL != voice)
   {
      memset((void*)voice, 0, sizeof(*voice));
      voice->base.info = _info;
   }
   return &voice->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_fof_v1_voice_t);

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
st_plugin_info_t *osc_fof_v1_init(void) {
   osc_fof_v1_info_t *ret = (osc_fof_v1_info_t *)malloc(sizeof(osc_fof_v1_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "osc_fof_v1";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "osc_fof_v1";
      ret->base.short_name  = "osc_fof_v1";
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
         return osc_fof_v1_init();
   }
   return NULL;
}
#endif // STFX_SKIP_MAIN_INIT

} // extern "C"
