// ----
// ---- file   : env_am_adsr_md1_digi_v1.cpp
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
// ----           $ g++ -Wall -Wno-unused-function -Wno-unused-variable -I../../tksampler -c env_am_adsr_md1_digi_v1.cpp -o env_am_adsr_md1_digi_v1.o
// ---- created: 15Jan2024 22:02:31
// ----
// ----
// ----

#include <plugin.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>




#define PARAM_A                  0
#define PARAM_ASHAPE             1
#define PARAM_D                  2
#define PARAM_DSHAPE             3
#define PARAM_S                  4
#define PARAM_R                  5
#define PARAM_RSHAPE             6
#define PARAM_OUT_SHAPE          7
#define NUM_PARAMS               8
static const char *loc_param_names[NUM_PARAMS] = {
   "a",                       // 0: A
   "ashape",                  // 1: ASHAPE
   "d",                       // 2: D
   "dshape",                  // 3: DSHAPE
   "s",                       // 4: S
   "r",                       // 5: R
   "rshape",                  // 6: RSHAPE
   "out_shape",               // 7: OUT_SHAPE

};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,                      // 0: A
   0.5f,                      // 1: ASHAPE
   0.3f,                      // 2: D
   0.5f,                      // 3: DSHAPE
   0.6f,                      // 4: S
   0.1f,                      // 5: R
   0.5f,                      // 6: RSHAPE
   0.5f,                      // 7: OUT_SHAPE

};

#define MOD_SLEW                 0
#define MOD_A                    1
#define MOD_ASHAPE               2
#define MOD_D                    3
#define MOD_S                    4
#define MOD_R                    5
#define MOD_DR_SHAPE             6
#define MOD_OUT_SHAPE            7
#define NUM_MODS                 8
static const char *loc_mod_names[NUM_MODS] = {
   "slew",                 // 0: SLEW
   "a",                    // 1: A
   "ashape",               // 2: ASHAPE
   "d",                    // 3: D
   "s",                    // 4: S
   "r",                    // 5: R
   "dr_shape",             // 6: DR_SHAPE
   "out_shape",            // 7: OUT_SHAPE

};

typedef struct env_am_adsr_md1_digi_v1_info_s {
   st_plugin_info_t base;
} env_am_adsr_md1_digi_v1_info_t;

typedef struct env_am_adsr_md1_digi_v1_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];

} env_am_adsr_md1_digi_v1_shared_t;

typedef struct env_am_adsr_md1_digi_v1_voice_s {
   st_plugin_voice_t base;
   float             sample_rate;
   float             note_speed_fixed;
   float             note_speed_cur;
   float             note_speed_inc;
   float             note_cur;
   float             note_inc;
   float             mods[NUM_MODS];
   float mod_slew_cur;
   float mod_slew_inc;
   float mod_a_cur;
   float mod_a_inc;
   float mod_ashape_cur;
   float mod_ashape_inc;
   float mod_d_cur;
   float mod_d_inc;
   float mod_s_cur;
   float mod_s_inc;
   float mod_r_cur;
   float mod_r_inc;
   float mod_dr_shape_cur;
   float mod_dr_shape_inc;
   float mod_out_shape_cur;
   float mod_out_shape_inc;

   float tmp1;
   float tmp2_last_gate;
   float tmp3_levelint;
   float tmp4_level;
   float tmp5_last_sus_level;
   float tmp6_last_atk_level;
   short tmp7_segidx;
   float tmp8;
   float tmp9;
   float tmp10;
   float tmp11;
   float tmp12;
   float tmp13;
   float var_x;
   float var_v_lvl_c;
   float var_v_slew;

} env_am_adsr_md1_digi_v1_voice_t;

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
   ST_PLUGIN_VOICE_CAST(env_am_adsr_md1_digi_v1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(env_am_adsr_md1_digi_v1_shared_t);

   float out = 0.0f;
   // -------- lane "init" modIdx=0
   
   // -- mod="set v_lvl_c" dstVar=out
   voice->var_v_lvl_c = 0;
} /* end init */

void loc_prepare(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(env_am_adsr_md1_digi_v1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(env_am_adsr_md1_digi_v1_shared_t);

   float out = 0.0f;
   // -------- lane "prepare" modIdx=0
   
   // -- mod="1" dstVar=out
   out = 1.0f;
   
   // -- mod="$m_slew" dstVar=out
   voice->tmp1/*seq*/ = voice->mod_slew_cur;
   out -= voice->tmp1/*seq*/;
   
   // -- mod="clp" dstVar=out
   if(out > 1.0f) out = 1.0f;
   else if(out < 0.0f) out = 0.0f;
   
   // -- mod="pow" dstVar=out
   out = out * out;
   
   // -- mod="pow" dstVar=out
   out = out * out;
   
   // -- mod="sto v_slew" dstVar=out
   voice->var_v_slew = out;
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
   static const char *groupNames[3] = { "rate", "shape", "level" };
   if(_paramGroupIdx < 3u)
      r = groupNames[_paramGroupIdx];
   return r;
}

static unsigned int ST_PLUGIN_API loc_get_param_group_idx(st_plugin_info_t *_info,
                                                          unsigned int      _paramIdx
                                                          ) {
   (void)_info;
   unsigned int r = ~0u;
   static unsigned int groupIndices[NUM_PARAMS] = { 0, 1, 0, 1, 2, 0, 1, 1 };
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
   ST_PLUGIN_SHARED_CAST(env_am_adsr_md1_digi_v1_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(env_am_adsr_md1_digi_v1_shared_t);
   shared->params[_paramIdx] = _value;
}

static unsigned int ST_PLUGIN_API loc_query_dynamic_param_preset_values(st_plugin_shared_t *_shared,
                                                                        const unsigned int  _paramIdx,
                                                                        float              *_retValues,
                                                                        const unsigned int  _retValuesSize
                                                                        ) {
   ST_PLUGIN_SHARED_CAST(env_am_adsr_md1_digi_v1_shared_t);
   unsigned int r = 0u;
   return r;
}

static unsigned int ST_PLUGIN_API loc_query_dynamic_param_preset_name(st_plugin_shared_t *_shared,
                                                                      const unsigned int  _paramIdx,
                                                                      const unsigned int  _presetIdx,
                                                                      char               *_retBuf,
                                                                      const unsigned int  _retBufSize
                                                                      ) {
   ST_PLUGIN_SHARED_CAST(env_am_adsr_md1_digi_v1_shared_t);
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
   ST_PLUGIN_SHARED_CAST(env_am_adsr_md1_digi_v1_shared_t);
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
   ST_PLUGIN_SHARED_CAST(env_am_adsr_md1_digi_v1_shared_t);
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
   ST_PLUGIN_VOICE_CAST(env_am_adsr_md1_digi_v1_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(env_am_adsr_md1_digi_v1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(env_am_adsr_md1_digi_v1_shared_t);
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


      voice->tmp2_last_gate = 0.0f;
      voice->tmp7_segidx = 0;

      voice->var_x = 0.0f;
      voice->var_v_lvl_c = 0.0f;
      voice->var_v_slew = 0.0f;

      loc_init(&voice->base);


   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(env_am_adsr_md1_digi_v1_voice_t);
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
   ST_PLUGIN_VOICE_CAST(env_am_adsr_md1_digi_v1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(env_am_adsr_md1_digi_v1_shared_t);
   (void)_note;
   (void)_vol;
   (void)_pan;

#ifdef OVERSAMPLE_FACTOR
   float noteSpeed = _freqHz / (voice->sample_rate * OVERSAMPLE_FACTOR);
#else
   float noteSpeed = _freqHz / voice->sample_rate;
#endif // OVERSAMPLE_FACTOR

   float modslew         = voice->mods[MOD_SLEW         ];
   float moda            = shared->params[PARAM_A           ]                       + voice->mods[MOD_A            ];
   float modashape       = shared->params[PARAM_ASHAPE      ]                       + voice->mods[MOD_ASHAPE       ];
   float modd            = shared->params[PARAM_D           ]                       + voice->mods[MOD_D            ];
   float mods            = shared->params[PARAM_S           ]                       + voice->mods[MOD_S            ];
   float modr            = shared->params[PARAM_R           ]                       + voice->mods[MOD_R            ];
   float moddr_shape     = voice->mods[MOD_DR_SHAPE     ];
   float modout_shape    = shared->params[PARAM_OUT_SHAPE   ]                       + voice->mods[MOD_OUT_SHAPE    ];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->note_speed_inc = (noteSpeed - voice->note_speed_cur) * recBlockSize;
      voice->note_inc       = (_note - voice->note_cur) * recBlockSize;
      voice->mod_slew_inc         = (modslew            - voice->mod_slew_cur          ) * recBlockSize;
      voice->mod_a_inc            = (moda               - voice->mod_a_cur             ) * recBlockSize;
      voice->mod_ashape_inc       = (modashape          - voice->mod_ashape_cur        ) * recBlockSize;
      voice->mod_d_inc            = (modd               - voice->mod_d_cur             ) * recBlockSize;
      voice->mod_s_inc            = (mods               - voice->mod_s_cur             ) * recBlockSize;
      voice->mod_r_inc            = (modr               - voice->mod_r_cur             ) * recBlockSize;
      voice->mod_dr_shape_inc     = (moddr_shape        - voice->mod_dr_shape_cur      ) * recBlockSize;
      voice->mod_out_shape_inc    = (modout_shape       - voice->mod_out_shape_cur     ) * recBlockSize;

      loc_prepare(&voice->base);
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->note_speed_cur = noteSpeed;
      voice->note_speed_inc = 0.0f;
      voice->note_cur       = _note;
      voice->note_inc       = 0.0f;
      voice->mod_slew_cur         = modslew;
      voice->mod_slew_inc         = 0.0f;
      voice->mod_a_cur            = moda;
      voice->mod_a_inc            = 0.0f;
      voice->mod_ashape_cur       = modashape;
      voice->mod_ashape_inc       = 0.0f;
      voice->mod_d_cur            = modd;
      voice->mod_d_inc            = 0.0f;
      voice->mod_s_cur            = mods;
      voice->mod_s_inc            = 0.0f;
      voice->mod_r_cur            = modr;
      voice->mod_r_inc            = 0.0f;
      voice->mod_dr_shape_cur     = moddr_shape;
      voice->mod_dr_shape_inc     = 0.0f;
      voice->mod_out_shape_cur    = modout_shape;
      voice->mod_out_shape_inc    = 0.0f;

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

   ST_PLUGIN_VOICE_CAST(env_am_adsr_md1_digi_v1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(env_am_adsr_md1_digi_v1_shared_t);

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
      
      // -- mod="adsr" dstVar=out
      
      // ---- mod="adsr" input "s" seq 1/1
      
      // -- mod="$m_s" dstVar=voice->tmp9/*suslvl*/
      voice->tmp9/*suslvl*/ = voice->mod_s_cur;
      if(out > 0.5f && voice->tmp2_last_gate < 0.5f)
      {
         voice->tmp3_levelint = 0.0f;
         voice->tmp4_level = 0.0f;
         voice->tmp6_last_atk_level = voice->tmp3_levelint;
         voice->tmp7_segidx = 0;
         voice->tmp2_last_gate = 1.0f;
      }
      else if(out < 0.5f && voice->tmp2_last_gate > 0.5f)
      {
         voice->tmp7_segidx = 3;
         voice->tmp2_last_gate = 0.0f;
         voice->tmp3_levelint = voice->tmp4_level;
         voice->tmp5_last_sus_level = voice->tmp4_level;
      }
      switch(voice->tmp7_segidx)
      {
         case 0: // a
            
            // ---- mod="adsr" input "a" seq 1/1
            
            // -- mod="$m_a" dstVar=voice->tmp8/*rate*/
            voice->tmp8/*rate*/ = voice->mod_a_cur;
            
            // -- mod="clp" dstVar=voice->tmp8/*rate*/
            if(voice->tmp8/*rate*/ > 1.0f) voice->tmp8/*rate*/ = 1.0f;
            else if(voice->tmp8/*rate*/ < 0.0f) voice->tmp8/*rate*/ = 0.0f;
            
            // -- mod="pow" dstVar=voice->tmp8/*rate*/
            voice->tmp8/*rate*/ = voice->tmp8/*rate*/ * voice->tmp8/*rate*/ * voice->tmp8/*rate*/;
            voice->tmp3_levelint += voice->tmp8/*rate*/ * 0.1f;
            if(voice->tmp3_levelint >= 1.0f)
            {
               voice->tmp4_level = 1.0f;
               voice->tmp3_levelint = 1.0f;
               voice->tmp7_segidx++;
            }
            else
            {
               voice->tmp10/*delta_h*/ = (voice->tmp3_levelint - voice->tmp6_last_atk_level);
               voice->tmp11/*delta_n*/ = (1.0f != voice->tmp6_last_atk_level) ? (voice->tmp10/*delta_h*/ / (1.0f - voice->tmp6_last_atk_level)) : voice->tmp10/*delta_h*/;
               
               // ---- mod="adsr" input "ashape" seq 1/1
               
               // -- mod="$m_ashape" dstVar=voice->tmp12/*ashape_c*/
               voice->tmp12/*ashape_c*/ = voice->mod_ashape_cur;
               
               // -- mod="fma" dstVar=voice->tmp12/*ashape_c*/
               voice->tmp12/*ashape_c*/ = (voice->tmp12/*ashape_c*/ * 16.0f) + -8.0f;
               voice->tmp11/*delta_n*/ = mathLogLinExpf(voice->tmp11/*delta_n*/, voice->tmp12/*ashape_c*/);
               voice->tmp4_level = voice->tmp6_last_atk_level + voice->tmp11/*delta_n*/ * voice->tmp10/*delta_h*/;
            }
            break;
      
         case 1: // d
            
            // ---- mod="adsr" input "d" seq 1/1
            
            // -- mod="$m_d" dstVar=voice->tmp8/*rate*/
            voice->tmp8/*rate*/ = voice->mod_d_cur;
            
            // -- mod="clp" dstVar=voice->tmp8/*rate*/
            if(voice->tmp8/*rate*/ > 1.0f) voice->tmp8/*rate*/ = 1.0f;
            else if(voice->tmp8/*rate*/ < 0.0f) voice->tmp8/*rate*/ = 0.0f;
            
            // -- mod="pow" dstVar=voice->tmp8/*rate*/
            voice->tmp8/*rate*/ = voice->tmp8/*rate*/ * voice->tmp8/*rate*/ * voice->tmp8/*rate*/;
            voice->tmp3_levelint -= voice->tmp8/*rate*/ * 0.1f;
            if(voice->tmp3_levelint <= voice->tmp9/*suslvl*/)
            {
               voice->tmp4_level = voice->tmp9/*suslvl*/;
               voice->tmp3_levelint = voice->tmp9/*suslvl*/;
               voice->tmp7_segidx++;
            }
            else
            {
               voice->tmp10/*delta_h*/ = (voice->tmp3_levelint - voice->tmp9/*suslvl*/);
               voice->tmp11/*delta_n*/ = (1.0f != voice->tmp9/*suslvl*/) ? (voice->tmp10/*delta_h*/ / (1.0f - voice->tmp9/*suslvl*/)) : voice->tmp10/*delta_h*/;
               
               // ---- mod="adsr" input "dshape" seq 1/1
               
               // -- mod="$p_dshape" dstVar=voice->tmp12/*dshape_c*/
               voice->tmp12/*dshape_c*/ = shared->params[PARAM_DSHAPE];
               
               // -- mod="$m_dr_shape" dstVar=voice->tmp12/*dshape_c*/
               voice->tmp13/*seq*/ = voice->mod_dr_shape_cur;
               voice->tmp12/*dshape_c*/ += voice->tmp13/*seq*/;
               
               // -- mod="fma" dstVar=voice->tmp12/*dshape_c*/
               voice->tmp12/*dshape_c*/ = (voice->tmp12/*dshape_c*/ * 16.0f) + -8.0f;
               voice->tmp11/*delta_n*/ = mathLogLinExpf(voice->tmp11/*delta_n*/, voice->tmp12/*dshape_c*/);
               voice->tmp4_level = voice->tmp9/*suslvl*/ + voice->tmp11/*delta_n*/ * voice->tmp10/*delta_h*/;
            }
            break;
      
         case 2: // s
            voice->tmp4_level = voice->tmp9/*suslvl*/;
            break;
      
         case 3: // r
            
            // ---- mod="adsr" input "r" seq 1/1
            
            // -- mod="$m_r" dstVar=voice->tmp8/*rate*/
            voice->tmp8/*rate*/ = voice->mod_r_cur;
            
            // -- mod="clp" dstVar=voice->tmp8/*rate*/
            if(voice->tmp8/*rate*/ > 1.0f) voice->tmp8/*rate*/ = 1.0f;
            else if(voice->tmp8/*rate*/ < 0.0f) voice->tmp8/*rate*/ = 0.0f;
            
            // -- mod="pow" dstVar=voice->tmp8/*rate*/
            voice->tmp8/*rate*/ = voice->tmp8/*rate*/ * voice->tmp8/*rate*/ * voice->tmp8/*rate*/;
            voice->tmp3_levelint -= voice->tmp8/*rate*/ * 0.1f;
            if(voice->tmp3_levelint <= 0.0f)
            {
               voice->tmp4_level = 0.0f;
               voice->tmp3_levelint = 0.0f;
               voice->tmp7_segidx++;
            }
               voice->tmp10/*delta_h*/ = (voice->tmp3_levelint/* - 0.0f*/);
               voice->tmp11/*delta_n*/ = (0.0f != voice->tmp5_last_sus_level) ? (voice->tmp10/*delta_h*/ / voice->tmp5_last_sus_level) : voice->tmp10/*delta_h*/;
               
               // ---- mod="adsr" input "rshape" seq 1/1
               
               // -- mod="$p_rshape" dstVar=voice->tmp12/*rshape_c*/
               voice->tmp12/*rshape_c*/ = shared->params[PARAM_RSHAPE];
               
               // -- mod="$m_dr_shape" dstVar=voice->tmp12/*rshape_c*/
               voice->tmp13/*seq*/ = voice->mod_dr_shape_cur;
               voice->tmp12/*rshape_c*/ += voice->tmp13/*seq*/;
               
               // -- mod="fma" dstVar=voice->tmp12/*rshape_c*/
               voice->tmp12/*rshape_c*/ = (voice->tmp12/*rshape_c*/ * 16.0f) + -8.0f;
               voice->tmp11/*delta_n*/ = mathLogLinExpf(voice->tmp11/*delta_n*/, voice->tmp12/*rshape_c*/);
               voice->tmp4_level = voice->tmp11/*delta_n*/ * voice->tmp10/*delta_h*/;
            break;
      
         default:
         case 4: // <end>
            break;
      }
      out = voice->tmp4_level;
      
      // -- mod="lle" dstVar=out
      
      // ---- mod="lle" input "c" seq 1/1
      
      // -- mod="$m_out_shape" dstVar=voice->tmp8/*c*/
      voice->tmp8/*c*/ = voice->mod_out_shape_cur;
      
      // -- mod="fma" dstVar=voice->tmp8/*c*/
      voice->tmp8/*c*/ = (voice->tmp8/*c*/ * 4.0f) + -2.0f;
      out = mathLogLinExpf(out, voice->tmp8/*c*/);
      
      // -- mod="ipl" dstVar=out
      
      // ---- mod="ipl" input "a" seq 1/1
      
      // -- mod="$v_lvl_c" dstVar=voice->tmp8/*a*/
      voice->tmp8/*a*/ = voice->var_v_lvl_c;
      
      // ---- mod="ipl" input "t" seq 1/1
      
      // -- mod="$v_slew" dstVar=voice->tmp9/*t*/
      voice->tmp9/*t*/ = voice->var_v_slew;
      out = voice->tmp8/*a*/ + (out - voice->tmp8/*a*/) * voice->tmp9/*t*/;
      
      // -- mod="sto v_lvl_c" dstVar=out
      voice->var_v_lvl_c = out;

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
      voice->mod_slew_cur          += voice->mod_slew_inc;
      voice->mod_a_cur             += voice->mod_a_inc;
      voice->mod_ashape_cur        += voice->mod_ashape_inc;
      voice->mod_d_cur             += voice->mod_d_inc;
      voice->mod_s_cur             += voice->mod_s_inc;
      voice->mod_r_cur             += voice->mod_r_inc;
      voice->mod_dr_shape_cur      += voice->mod_dr_shape_inc;
      voice->mod_out_shape_cur     += voice->mod_out_shape_inc;
   } /* loop numFrames */
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   env_am_adsr_md1_digi_v1_shared_t *ret = (env_am_adsr_md1_digi_v1_shared_t *)malloc(sizeof(env_am_adsr_md1_digi_v1_shared_t));
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
   env_am_adsr_md1_digi_v1_voice_t *voice = (env_am_adsr_md1_digi_v1_voice_t *)malloc(sizeof(env_am_adsr_md1_digi_v1_voice_t));
   if(NULL != voice)
   {
      memset((void*)voice, 0, sizeof(*voice));
      voice->base.info = _info;
      voice->tmp3_levelint = 0.0f;
      voice->tmp4_level = 0.0f;
      voice->tmp5_last_sus_level = 0.0f;
      voice->tmp6_last_atk_level = 0.0f;
   }
   return &voice->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(env_am_adsr_md1_digi_v1_voice_t);

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
st_plugin_info_t *env_am_adsr_md1_digi_v1_init(void) {
   env_am_adsr_md1_digi_v1_info_t *ret = (env_am_adsr_md1_digi_v1_info_t *)malloc(sizeof(env_am_adsr_md1_digi_v1_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "env_am_adsr_md1_digi_v1";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "env_am_adsr_md1_digi_v1";
      ret->base.short_name  = "env_am_adsr_md1_digi_v1";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_AMP;
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
         return env_am_adsr_md1_digi_v1_init();
   }
   return NULL;
}
#endif // STFX_SKIP_MAIN_INIT

} // extern "C"
