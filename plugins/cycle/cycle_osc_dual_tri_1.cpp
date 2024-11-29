// ----
// ---- file   : osc_dual_tri_1.cpp
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
// ----           $ g++ -Wall -Wno-unused-function -Wno-unused-variable -I../../tksampler -c osc_dual_tri_1.cpp -o osc_dual_tri_1.o
// ---- created: 18Dec2023 20:33:44
// ----
// ----
// ----

#include <plugin.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>


#define OVERSAMPLE_FACTOR  8.0f

#define PARAM_PHASE_RAND         0
#define PARAM_COLOR_1            1
#define PARAM_COLOR_2            2
#define PARAM_DETUNE             3
#define PARAM_DEPHASE            4
#define PARAM_LEVEL_2            5
#define PARAM_CUTOFF             6
#define PARAM_RES                7
#define NUM_PARAMS               8
static const char *loc_param_names[NUM_PARAMS] = {
   "phase_rand",              // 0: PHASE_RAND
   "color_1",                 // 1: COLOR_1
   "color_2",                 // 2: COLOR_2
   "detune",                  // 3: DETUNE
   "dephase",                 // 4: DEPHASE
   "level_2",                 // 5: LEVEL_2
   "cutoff",                  // 6: CUTOFF
   "res",                     // 7: RES

};
static float loc_param_resets[NUM_PARAMS] = {
   0.0f,                      // 0: PHASE_RAND
   0.5f,                      // 1: COLOR_1
   0.5f,                      // 2: COLOR_2
   0.5f,                      // 3: DETUNE
   0.0f,                      // 4: DEPHASE
   0.5f,                      // 5: LEVEL_2
   0.5f,                      // 6: CUTOFF
   0.3f,                      // 7: RES

};

#define MOD_VSYNC                0
#define MOD_COLOR_1              1
#define MOD_COLOR_2              2
#define MOD_DETUNE               3
#define MOD_DEPHASE              4
#define MOD_LEVEL_2              5
#define MOD_CUTOFF               6
#define MOD_RES                  7
#define NUM_MODS                 8
static const char *loc_mod_names[NUM_MODS] = {
   "vsync",                // 0: VSYNC
   "color_1",              // 1: COLOR_1
   "color_2",              // 2: COLOR_2
   "detune",               // 3: DETUNE
   "dephase",              // 4: DEPHASE
   "level_2",              // 5: LEVEL_2
   "cutoff",               // 6: CUTOFF
   "res",                  // 7: RES

};

typedef struct osc_dual_tri_1_info_s {
   st_plugin_info_t base;
} osc_dual_tri_1_info_t;

typedef struct osc_dual_tri_1_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];

} osc_dual_tri_1_shared_t;

typedef struct osc_dual_tri_1_voice_s {
   st_plugin_voice_t base;
   float             sample_rate;
   float             note_speed_fixed;
   float             note_speed_cur;
   float             note_speed_inc;
   float             note_cur;
   float             note_inc;
   float             mods[NUM_MODS];
   float mod_vsync_cur;
   float mod_vsync_inc;
   float mod_color_1_cur;
   float mod_color_1_inc;
   float mod_color_2_cur;
   float mod_color_2_inc;
   float mod_detune_cur;
   float mod_detune_inc;
   float mod_dephase_cur;
   float mod_dephase_inc;
   float mod_level_2_cur;
   float mod_level_2_inc;
   float mod_cutoff_cur;
   float mod_cutoff_inc;
   float mod_res_cur;
   float mod_res_inc;

   short tmp1_i2f;
   unsigned short tmp2_lfsr_state;
   short tmp3_lfsr_state_signed;
   float tmp4_seq;
   short tmp5_i2f;
   unsigned short tmp6_lfsr_state;
   short tmp7_lfsr_state_signed;
   float tmp8_seq;
   float tmp9_kbd;
   float tmp10_off;
   float tmp11_scl;
   float tmp12_tri_phase;
   float tmp13_tri_speed;
   float tmp14_freq;
   float tmp15_tri_tmp;
   float tmp16_phase;
   float tmp17_vsync;
   float tmp18_c;
   float tmp19_seq;
   float tmp20_tri_phase;
   float tmp21_tri_speed;
   float tmp22_freq;
   float tmp23_tri_tmp;
   float tmp24_phase;
   float tmp25_vsync;
   float tmp26_c;
   float tmp27_seq;
   float tmp28_svf_lp;
   float tmp29_svf_hp;
   float tmp30_svf_bp;
   float tmp31_res;
   float tmp32_freq;
   float tmp33_svf_lp;
   float tmp34_svf_hp;
   float tmp35_svf_bp;
   float tmp36_freq;
   float var_x;
   float var_v_phrand_1;
   float var_v_phrand_2;
   float var_v_cutoff;
   float var_v_res;
   float var_v_vsync;
   float var_v_freq_2;

} osc_dual_tri_1_voice_t;

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

static void loc_init(st_plugin_voice_t *_voice);
static void loc_prepare(st_plugin_voice_t *_voice);

void loc_init(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_dual_tri_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_dual_tri_1_shared_t);

   float out = 0.0f;
   // -------- lane "init" modIdx=0
   
   // -- mod="fsr" dstVar=out
   voice->tmp1_i2f = (int)(out * 2048);  // IntFallback: F2I
   voice->tmp2_lfsr_state ^= voice->tmp2_lfsr_state >> 7;
   voice->tmp2_lfsr_state ^= voice->tmp2_lfsr_state << 9;
   voice->tmp2_lfsr_state ^= voice->tmp2_lfsr_state >> 13;
   voice->tmp3_lfsr_state_signed = (voice->tmp2_lfsr_state & 65520);
   voice->tmp1_i2f = voice->tmp3_lfsr_state_signed >> 4;
   out = voice->tmp1_i2f / ((float)(2048));  // IntFallback: I2F
   
   // -- mod="$p_phase_rand" dstVar=out
   voice->tmp4_seq = shared->params[PARAM_PHASE_RAND];
   out *= voice->tmp4_seq;
   
   // -- mod="sto v_phrand_1" dstVar=out
   voice->var_v_phrand_1 = out;
   
   // -- mod="fsr" dstVar=out
   voice->tmp5_i2f = (int)(out * 2048);  // IntFallback: F2I
   voice->tmp6_lfsr_state ^= voice->tmp6_lfsr_state >> 7;
   voice->tmp6_lfsr_state ^= voice->tmp6_lfsr_state << 9;
   voice->tmp6_lfsr_state ^= voice->tmp6_lfsr_state >> 13;
   voice->tmp7_lfsr_state_signed = (voice->tmp6_lfsr_state & 65520);
   voice->tmp5_i2f = voice->tmp7_lfsr_state_signed >> 4;
   out = voice->tmp5_i2f / ((float)(2048));  // IntFallback: I2F
   
   // -- mod="$p_phase_rand" dstVar=out
   voice->tmp8_seq = shared->params[PARAM_PHASE_RAND];
   out *= voice->tmp8_seq;
   
   // -- mod="sto v_phrand_2" dstVar=out
   voice->var_v_phrand_2 = out;
} /* end init */

void loc_prepare(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_dual_tri_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_dual_tri_1_shared_t);

   float out = 0.0f;
   // -------- lane "prepare" modIdx=0
   
   // -- mod="$m_cutoff" dstVar=out
   out = voice->mod_cutoff_cur;
   
   // -- mod="clp" dstVar=out
   if(out > 1.0f) out = 1.0f;
   else if(out < 0.02f) out = 0.02f;
   
   // -- mod="kbd" dstVar=out
   out *= 127.0f;
   
   // ---- mod="kbd" input "kbd" seq 1/1
   
   // -- mod="0.5" dstVar=voice->tmp9_kbd
   voice->tmp9_kbd = 0.5f;
   voice->tmp10_off = 0;
   voice->tmp11_scl = 4;
   out += (voice->note_cur + (voice->tmp10_off * 12.0f) - 64.0f) * voice->tmp9_kbd;
   out = clamp(out, 0.0f, 127.0f);
   #ifdef OVERSAMPLE_FACTOR
   out = ((440.0f/32.0f)*expf( ((out-9.0f)/12.0f)*logf(2.0f) )) / (voice->sample_rate * OVERSAMPLE_FACTOR);
   #else
   out = ((440.0f/32.0f)*expf( ((out-9.0f)/12.0f)*logf(2.0f) )) / voice->sample_rate;
   #endif
   out *= voice->tmp11_scl;
   
   // -- mod="sto v_cutoff" dstVar=out
   voice->var_v_cutoff = out;
   
   // -- mod="$m_res" dstVar=out
   out = voice->mod_res_cur;
   
   // -- mod="sto v_res" dstVar=out
   voice->var_v_res = out;
   
   // -- mod="$m_vsync" dstVar=out
   out = voice->mod_vsync_cur;
   
   // -- mod="bts" dstVar=out
   out = loc_bipolar_to_scale(out, 16.0f, 16.0f);
   
   // -- mod="sto v_vsync" dstVar=out
   voice->var_v_vsync = out;
   
   // -- mod="$m_detune" dstVar=out
   out = voice->mod_detune_cur;
   
   // -- mod="bts" dstVar=out
   out = loc_bipolar_to_scale(out, 16.0f, 16.0f);
   
   // -- mod="sto v_freq_2" dstVar=out
   voice->var_v_freq_2 = out;
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
   ST_PLUGIN_SHARED_CAST(osc_dual_tri_1_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(osc_dual_tri_1_shared_t);
   shared->params[_paramIdx] = _value;
}

static unsigned int ST_PLUGIN_API loc_query_dynamic_param_preset_values(st_plugin_shared_t *_shared,
                                                                        const unsigned int  _paramIdx,
                                                                        float              *_retValues,
                                                                        const unsigned int  _retValuesSize
                                                                        ) {
   ST_PLUGIN_SHARED_CAST(osc_dual_tri_1_shared_t);
   unsigned int r = 0u;

   return r;
}

static unsigned int ST_PLUGIN_API loc_query_dynamic_param_preset_name(st_plugin_shared_t *_shared,
                                                                      const unsigned int  _paramIdx,
                                                                      const unsigned int  _presetIdx,
                                                                      char               *_retBuf,
                                                                      const unsigned int  _retBufSize
                                                                      ) {
   ST_PLUGIN_SHARED_CAST(osc_dual_tri_1_shared_t);
   unsigned int r = 0u;

   return r;
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
   ST_PLUGIN_VOICE_CAST(osc_dual_tri_1_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(osc_dual_tri_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_dual_tri_1_shared_t);
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


      voice->tmp3_lfsr_state_signed = 0;
      voice->tmp7_lfsr_state_signed = 0;
      voice->tmp12_tri_phase = 0.0f;
      voice->tmp20_tri_phase = 0.0f;
      voice->tmp28_svf_lp = 0.0f;
      voice->tmp29_svf_hp = 0.0f;
      voice->tmp30_svf_bp = 0.0f;
      voice->tmp33_svf_lp = 0.0f;
      voice->tmp34_svf_hp = 0.0f;
      voice->tmp35_svf_bp = 0.0f;

      voice->var_x = 0.0f;
      voice->var_v_phrand_1 = 0.0f;
      voice->var_v_phrand_2 = 0.0f;
      voice->var_v_cutoff = 0.0f;
      voice->var_v_res = 0.0f;
      voice->var_v_vsync = 0.0f;
      voice->var_v_freq_2 = 0.0f;

      loc_init(&voice->base);

   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(osc_dual_tri_1_voice_t);
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
   ST_PLUGIN_VOICE_CAST(osc_dual_tri_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_dual_tri_1_shared_t);
   (void)_note;
   (void)_vol;
   (void)_pan;

#ifdef OVERSAMPLE_FACTOR
   float noteSpeed = _freqHz / (voice->sample_rate * OVERSAMPLE_FACTOR);
#else
   float noteSpeed = _freqHz / voice->sample_rate;
#endif // OVERSAMPLE_FACTOR

   float modvsync        = voice->mods[MOD_VSYNC        ];
   float modcolor_1      = shared->params[PARAM_COLOR_1     ] * 8.0f - 4.0f         + voice->mods[MOD_COLOR_1      ];
   float modcolor_2      = shared->params[PARAM_COLOR_2     ] * 8.0f - 4.0f         + voice->mods[MOD_COLOR_2      ];
   float moddetune       = shared->params[PARAM_DETUNE      ] * 2.0f - 1.0f         + voice->mods[MOD_DETUNE       ];
   float moddephase      = shared->params[PARAM_DEPHASE     ]                       + voice->mods[MOD_DEPHASE      ];
   float modlevel_2      = shared->params[PARAM_LEVEL_2     ] * 2.0f - 1.0f         + voice->mods[MOD_LEVEL_2      ];
   float modcutoff       = shared->params[PARAM_CUTOFF      ]                       + voice->mods[MOD_CUTOFF       ];
   float modres          = shared->params[PARAM_RES         ]                       + voice->mods[MOD_RES          ];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->note_speed_inc = (noteSpeed - voice->note_speed_cur) * recBlockSize;
      voice->note_inc       = (_note - voice->note_cur) * recBlockSize;
      voice->mod_vsync_inc        = (modvsync           - voice->mod_vsync_cur         ) * recBlockSize;
      voice->mod_color_1_inc      = (modcolor_1         - voice->mod_color_1_cur       ) * recBlockSize;
      voice->mod_color_2_inc      = (modcolor_2         - voice->mod_color_2_cur       ) * recBlockSize;
      voice->mod_detune_inc       = (moddetune          - voice->mod_detune_cur        ) * recBlockSize;
      voice->mod_dephase_inc      = (moddephase         - voice->mod_dephase_cur       ) * recBlockSize;
      voice->mod_level_2_inc      = (modlevel_2         - voice->mod_level_2_cur       ) * recBlockSize;
      voice->mod_cutoff_inc       = (modcutoff          - voice->mod_cutoff_cur        ) * recBlockSize;
      voice->mod_res_inc          = (modres             - voice->mod_res_cur           ) * recBlockSize;

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
      voice->mod_color_1_cur      = modcolor_1;
      voice->mod_color_1_inc      = 0.0f;
      voice->mod_color_2_cur      = modcolor_2;
      voice->mod_color_2_inc      = 0.0f;
      voice->mod_detune_cur       = moddetune;
      voice->mod_detune_inc       = 0.0f;
      voice->mod_dephase_cur      = moddephase;
      voice->mod_dephase_inc      = 0.0f;
      voice->mod_level_2_cur      = modlevel_2;
      voice->mod_level_2_inc      = 0.0f;
      voice->mod_cutoff_cur       = modcutoff;
      voice->mod_cutoff_inc       = 0.0f;
      voice->mod_res_cur          = modres;
      voice->mod_res_inc          = 0.0f;

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

   ST_PLUGIN_VOICE_CAST(osc_dual_tri_1_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_dual_tri_1_shared_t);

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
         voice->tmp14_freq = 1;
         voice->tmp13_tri_speed = voice->note_speed_cur * voice->tmp14_freq;
         
         // ---- mod="tri" input "phase" seq 1/1
         
         // -- mod="$v_phrand_1" dstVar=voice->tmp16_phase
         voice->tmp16_phase = voice->var_v_phrand_1;
         
         // ---- mod="tri" input "vsync" seq 1/1
         
         // -- mod="$v_vsync" dstVar=voice->tmp17_vsync
         voice->tmp17_vsync = voice->var_v_vsync;
         voice->tmp15_tri_tmp = ((voice->tmp12_tri_phase + voice->tmp16_phase));
         voice->tmp15_tri_tmp = voice->tmp15_tri_tmp * voice->tmp17_vsync;
         voice->tmp15_tri_tmp = ffrac_s(voice->tmp15_tri_tmp);
         out = (voice->tmp15_tri_tmp < 0.5f) ? (-1.0 + voice->tmp15_tri_tmp * 4.0f) : (1.0 - (voice->tmp15_tri_tmp - 0.5f)*4);
         voice->tmp12_tri_phase = ffrac_s(voice->tmp12_tri_phase + voice->tmp13_tri_speed);
         
         // -- mod="lle" dstVar=out
         
         // ---- mod="lle" input "c" seq 1/1
         
         // -- mod="$m_color_1" dstVar=voice->tmp18_c
         voice->tmp18_c = voice->mod_color_1_cur;
         out = mathLogLinExpf(out, voice->tmp18_c);
         
         // -- mod="tri" dstVar=out
         
         // ---- mod="tri" input "freq" seq 1/1
         
         // -- mod="$v_freq_2" dstVar=voice->tmp22_freq
         voice->tmp22_freq = voice->var_v_freq_2;
         voice->tmp21_tri_speed = voice->note_speed_cur * voice->tmp22_freq;
         
         // ---- mod="tri" input "phase" seq 1/1
         
         // -- mod="$v_phrand_1" dstVar=voice->tmp24_phase
         voice->tmp24_phase = voice->var_v_phrand_1;
         
         // -- mod="$m_dephase" dstVar=voice->tmp24_phase
         voice->tmp24_phase = voice->mod_dephase_cur;
         
         // ---- mod="tri" input "vsync" seq 1/1
         
         // -- mod="$v_vsync" dstVar=voice->tmp25_vsync
         voice->tmp25_vsync = voice->var_v_vsync;
         voice->tmp23_tri_tmp = ((voice->tmp20_tri_phase + voice->tmp24_phase));
         voice->tmp23_tri_tmp = voice->tmp23_tri_tmp * voice->tmp25_vsync;
         voice->tmp23_tri_tmp = ffrac_s(voice->tmp23_tri_tmp);
         voice->tmp19_seq = (voice->tmp23_tri_tmp < 0.5f) ? (-1.0 + voice->tmp23_tri_tmp * 4.0f) : (1.0 - (voice->tmp23_tri_tmp - 0.5f)*4);
         voice->tmp20_tri_phase = ffrac_s(voice->tmp20_tri_phase + voice->tmp21_tri_speed);
         
         // -- mod="lle" dstVar=voice->tmp19_seq
         
         // ---- mod="lle" input "c" seq 1/1
         
         // -- mod="$m_color_2" dstVar=voice->tmp26_c
         voice->tmp26_c = voice->mod_color_2_cur;
         voice->tmp19_seq = mathLogLinExpf(voice->tmp19_seq, voice->tmp26_c);
         
         // -- mod="$m_level_2" dstVar=voice->tmp19_seq
         voice->tmp27_seq = voice->mod_level_2_cur;
         voice->tmp19_seq *= voice->tmp27_seq;
         out += voice->tmp19_seq;
         
         // -- mod="svf" dstVar=out
         
         // ---- mod="svf" input "res" seq 1/1
         
         // -- mod="$v_res" dstVar=voice->tmp31_res
         voice->tmp31_res = voice->var_v_res;
         
         // ---- mod="svf" input "freq" seq 1/1
         
         // -- mod="$v_cutoff" dstVar=voice->tmp32_freq
         voice->tmp32_freq = voice->var_v_cutoff;
         voice->tmp28_svf_lp = voice->tmp28_svf_lp + (voice->tmp30_svf_bp * voice->tmp32_freq);
         voice->tmp29_svf_hp = out - voice->tmp28_svf_lp - (voice->tmp30_svf_bp * voice->tmp31_res);
         voice->tmp30_svf_bp = voice->tmp30_svf_bp + (voice->tmp29_svf_hp * voice->tmp32_freq);
         out = voice->tmp28_svf_lp;
         
         // -- mod="svf" dstVar=out
         
         // ---- mod="svf" input "freq" seq 1/1
         
         // -- mod="$v_cutoff" dstVar=voice->tmp36_freq
         voice->tmp36_freq = voice->var_v_cutoff;
         voice->tmp33_svf_lp = voice->tmp33_svf_lp + (voice->tmp35_svf_bp * voice->tmp36_freq);
         voice->tmp34_svf_hp = out - voice->tmp33_svf_lp - (voice->tmp35_svf_bp * 1.0f);
         voice->tmp35_svf_bp = voice->tmp35_svf_bp + (voice->tmp34_svf_hp * voice->tmp36_freq);
         out = voice->tmp33_svf_lp;
   
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
      voice->mod_vsync_cur         += voice->mod_vsync_inc;
      voice->mod_color_1_cur       += voice->mod_color_1_inc;
      voice->mod_color_2_cur       += voice->mod_color_2_inc;
      voice->mod_detune_cur        += voice->mod_detune_inc;
      voice->mod_dephase_cur       += voice->mod_dephase_inc;
      voice->mod_level_2_cur       += voice->mod_level_2_inc;
      voice->mod_cutoff_cur        += voice->mod_cutoff_inc;
      voice->mod_res_cur           += voice->mod_res_inc;
   } /* loop numFrames */
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   osc_dual_tri_1_shared_t *ret = (osc_dual_tri_1_shared_t *)malloc(sizeof(osc_dual_tri_1_shared_t));
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
   osc_dual_tri_1_voice_t *voice = (osc_dual_tri_1_voice_t *)malloc(sizeof(osc_dual_tri_1_voice_t));
   if(NULL != voice)
   {
      memset((void*)voice, 0, sizeof(*voice));
      voice->base.info = _info;
      voice->tmp2_lfsr_state = 17545;
      voice->tmp6_lfsr_state = 56265;
   }
   return &voice->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_dual_tri_1_voice_t);

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
st_plugin_info_t *osc_dual_tri_1_init(void) {
   osc_dual_tri_1_info_t *ret = (osc_dual_tri_1_info_t *)malloc(sizeof(osc_dual_tri_1_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "osc_dual_tri_1";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "dual tri osc 1";
      ret->base.short_name  = "dual tri osc 1";
      ret->base.flags       = ST_PLUGIN_FLAG_OSC;
      ret->base.category    = ST_PLUGIN_CAT_UNKNOWN;
      ret->base.num_params  = NUM_PARAMS;
      ret->base.num_mods    = NUM_MODS;

      ret->base.shared_new                        = &loc_shared_new;
      ret->base.shared_delete                     = &loc_shared_delete;
      ret->base.voice_new                         = &loc_voice_new;
      ret->base.voice_delete                      = &loc_voice_delete;
      ret->base.get_param_name                    = &loc_get_param_name;
      ret->base.get_param_group_name              = &loc_get_param_group_name;
      ret->base.get_param_group_idx               = &loc_get_param_group_idx;
      ret->base.get_param_reset                   = &loc_get_param_reset;
      ret->base.query_dynamic_param_preset_values = &loc_query_dynamic_param_preset_values;
      ret->base.query_dynamic_param_preset_name   = &loc_query_dynamic_param_preset_name;
      ret->base.get_param_value                   = &loc_get_param_value;
      ret->base.set_param_value                   = &loc_set_param_value;
      ret->base.get_mod_name                      = &loc_get_mod_name;
      ret->base.set_sample_rate                   = &loc_set_sample_rate;
      ret->base.note_on                           = &loc_note_on;
      ret->base.set_mod_value                     = &loc_set_mod_value;
      ret->base.prepare_block                     = &loc_prepare_block;
      ret->base.process_replace                   = &loc_process_replace;
      ret->base.plugin_exit                       = &loc_plugin_exit;

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
         return osc_dual_tri_1_init();
   }
   return NULL;
}
#endif // STFX_SKIP_MAIN_INIT

} // extern "C"
