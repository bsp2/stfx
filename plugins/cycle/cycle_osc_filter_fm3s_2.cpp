// ----
// ---- file   : osc_filter_fm3s_2.cpp
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
// ----           $ g++ -Wall -Wno-unused-function -Wno-unused-variable -I../../tksampler -c osc_filter_fm3s_2.cpp -o osc_filter_fm3s_2.o
// ---- created: 11Nov2023 19:16:39
// ----
// ----
// ----

#include <plugin.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>


#define OVERSAMPLE_FACTOR  16.0f

#define PARAM_RATIO2             0
#define PARAM_LEVEL2             1
#define PARAM_RATIO3             2
#define PARAM_LEVEL3             3
#define PARAM_CUTOFF             4
#define PARAM_RES                5
#define NUM_PARAMS               6
static const char *loc_param_names[NUM_PARAMS] = {
   "ratio2",                  // 0: RATIO2
   "level2",                  // 1: LEVEL2
   "ratio3",                  // 2: RATIO3
   "level3",                  // 3: LEVEL3
   "cutoff",                  // 4: CUTOFF
   "res",                     // 5: RES

};
static float loc_param_resets[NUM_PARAMS] = {
   0.5f,                      // 0: RATIO2
   0.0f,                      // 1: LEVEL2
   0.5f,                      // 2: RATIO3
   0.0f,                      // 3: LEVEL3
   0.0f,                      // 4: CUTOFF
   0.0f,                      // 5: RES

};

#define MOD_RATIO2               0
#define MOD_LEVEL2               1
#define MOD_RATIO3               2
#define MOD_LEVEL3               3
#define MOD_CUTOFF               4
#define MOD_RES                  5
#define NUM_MODS                 6
static const char *loc_mod_names[NUM_MODS] = {
   "ratio2",       // 0: RATIO2
   "level2",       // 1: LEVEL2
   "ratio3",       // 2: RATIO3
   "level3",       // 3: LEVEL3
   "cutoff",       // 4: CUTOFF
   "res",          // 5: RES

};

typedef struct osc_filter_fm3s_2_info_s {
   st_plugin_info_t base;
} osc_filter_fm3s_2_info_t;

typedef struct osc_filter_fm3s_2_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];

   float lut_logsinexp[256];
   int b_lut_logsinexp;

   float lut_logsin[256];
   int b_lut_logsin;

   float lut_exp[2048];
   int b_lut_exp;

} osc_filter_fm3s_2_shared_t;

typedef struct osc_filter_fm3s_2_voice_s {
   st_plugin_voice_t base;
   float             sample_rate;
   float             note_speed_fixed;
   float             note_speed_cur;
   float             note_speed_inc;
   float             note_cur;
   float             note_inc;
   float             mods[NUM_MODS];
   float mod_ratio2_cur;
   float mod_ratio2_inc;
   float mod_level2_cur;
   float mod_level2_inc;
   float mod_ratio3_cur;
   float mod_ratio3_inc;
   float mod_level3_cur;
   float mod_level3_inc;
   float mod_cutoff_cur;
   float mod_cutoff_inc;
   float mod_res_cur;
   float mod_res_inc;

   float tmp1_exp;
   float tmp2_sin_phase;
   float tmp3_sin_speed;
   float tmp4_freq;
   float tmp5_sin_tmp;
   float tmp6_phase;
   float tmp7_seq;
   float tmp8_seq;
   float tmp9_sin_phase;
   float tmp10_sin_speed;
   float tmp11_freq;
   float tmp12_sin_tmp;
   float tmp13_phase;
   float tmp14_seq;
   float tmp15_exp;
   float tmp16_seq;
   float tmp17_seq;
   float tmp18_seq;
   float tmp19_seq;
   float tmp20_pha_phase;
   float tmp21_pha_speed;
   float tmp22_pha_tmp;
   float tmp23_freq;
   float tmp24_phase;
   float tmp25_pha_phase;
   float tmp26_pha_speed;
   float tmp27_pha_tmp;
   float tmp28_freq;
   float tmp29_seq;
   float tmp30_seq;
   float tmp31_seq;
   float tmp32_seq;
   float tmp33_svf_lp;
   float tmp34_svf_hp;
   float tmp35_svf_bp;
   float tmp36_res;
   float tmp37_freq;
   float tmp38_seq;
   float tmp39_pha_phase;
   float tmp40_pha_speed;
   float tmp41_pha_tmp;
   float tmp42_freq;
   float tmp43_seq;
   float tmp44_seq;
   float tmp45_seq;
   float tmp46_seq;
   float var_x;
   float var_v_ratio2;
   float var_v_level2;
   float var_v_loglevel2;
   float var_v_ratio3;
   float var_v_level3;
   float var_v_loglevel3;

} osc_filter_fm3s_2_voice_t;

#define loop(X)  for(unsigned int i = 0u; i < (X); i++)
#define clamp(a,b,c) (((a)<(b))?(b):(((a)>(c))?(c):(a)))

static inline float mathLerpf(float _a, float _b, float _t) { return _a + (_b - _a) * _t; }
static inline float mathClampf(float a, float b, float c) { return (((a)<(b))?(b):(((a)>(c))?(c):(a))); }
static inline float mathMinf(float a, float b) { return (a<b)?a:b; }
static inline float mathMaxf(float a, float b) { return (a>b)?a:b; }
static inline float mathAbsMaxf(float _x, float _y) { return ( ( (_x<0.0f)?-_x:_x)>((_y<0.0f)?-_y:_y)?_x:_y ); }
static inline float mathAbsMinf(float _x, float _y) { return ( ((_x<0.0f)?-_x:_x)<((_y<0.0f)?-_y:_y)?_x:_y ); }

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

static void loc_calc_lut_logsinexp(st_plugin_voice_t *_voice, float *_d);
static void loc_calc_lut_logsin(st_plugin_voice_t *_voice, float *_d);
static void loc_calc_lut_exp(st_plugin_voice_t *_voice, float *_d);
static void loc_prepare(st_plugin_voice_t *_voice);

void loc_calc_lut_logsinexp(st_plugin_voice_t *_voice, float *_d) {
   ST_PLUGIN_VOICE_CAST(osc_filter_fm3s_2_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_filter_fm3s_2_shared_t);
   float x = 0.0f;
   loop(256)
   {
      float out = x;
      // -------- lane "lut_logsinexp" modIdx=0
      
      // -- mod="2.71828" dstVar=out
      out = 2.71828f;
      
      // -- mod="pow" dstVar=out
      
      // ---- mod="pow" input "exp" seq 1/1
      
      // -- mod="sin" dstVar=voice->tmp1_exp
      
      // ---- mod="sin" input "freq" seq 1/1
      
      // -- mod="0" dstVar=voice->tmp4_freq
      voice->tmp4_freq = 0.0f;
      voice->tmp3_sin_speed = voice->note_speed_cur * voice->tmp4_freq;
      
      // ---- mod="sin" input "phase" seq 1/1
      
      // -- mod="$x" dstVar=voice->tmp6_phase
      voice->tmp6_phase = x;
      voice->tmp5_sin_tmp = ((voice->tmp2_sin_phase + voice->tmp6_phase));
      voice->tmp5_sin_tmp = ffrac_s(voice->tmp5_sin_tmp);
      voice->tmp1_exp = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp5_sin_tmp)&16383u];
      
      // -- mod="1" dstVar=voice->tmp1_exp
      voice->tmp7_seq = 1.0f;
      voice->tmp1_exp += voice->tmp7_seq;
      
      // -- mod="clp" dstVar=voice->tmp1_exp
      if(voice->tmp1_exp > 2.0f) voice->tmp1_exp = 2.0f;
      else if(voice->tmp1_exp < 1e-07f) voice->tmp1_exp = 1e-07f;
      
      // -- mod="log" dstVar=voice->tmp1_exp
      voice->tmp1_exp = mathLogf(voice->tmp1_exp);
      out = mathPowerf(out, voice->tmp1_exp);
      
      // -- mod="1" dstVar=out
      voice->tmp8_seq = 1.0f;
      out -= voice->tmp8_seq;
      _d[i] = out;

      // Next x
      x += 0.00392157f;
   } // loop 256
} /* end loc_calc_lut_logsinexp() */

void loc_calc_lut_logsin(st_plugin_voice_t *_voice, float *_d) {
   ST_PLUGIN_VOICE_CAST(osc_filter_fm3s_2_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_filter_fm3s_2_shared_t);
   float x = 0.0f;
   loop(256)
   {
      float out = x;
      // -------- lane "lut_logsin" modIdx=0
      
      // -- mod="sin" dstVar=out
      
      // ---- mod="sin" input "freq" seq 1/1
      
      // -- mod="0" dstVar=voice->tmp11_freq
      voice->tmp11_freq = 0.0f;
      voice->tmp10_sin_speed = voice->note_speed_cur * voice->tmp11_freq;
      
      // ---- mod="sin" input "phase" seq 1/1
      
      // -- mod="$x" dstVar=voice->tmp13_phase
      voice->tmp13_phase = x;
      voice->tmp12_sin_tmp = ((voice->tmp9_sin_phase + voice->tmp13_phase));
      voice->tmp12_sin_tmp = ffrac_s(voice->tmp12_sin_tmp);
      out = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp12_sin_tmp)&16383u];
      
      // -- mod="1" dstVar=out
      voice->tmp14_seq = 1.0f;
      out += voice->tmp14_seq;
      
      // -- mod="clp" dstVar=out
      if(out > 2.0f) out = 2.0f;
      else if(out < 1e-07f) out = 1e-07f;
      
      // -- mod="log" dstVar=out
      out = mathLogf(out);
      _d[i] = out;

      // Next x
      x += 0.00392157f;
   } // loop 256
} /* end loc_calc_lut_logsin() */

void loc_calc_lut_exp(st_plugin_voice_t *_voice, float *_d) {
   ST_PLUGIN_VOICE_CAST(osc_filter_fm3s_2_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_filter_fm3s_2_shared_t);
   float x = 0.0f;
   loop(2048)
   {
      float out = x;
      // -------- lane "lut_exp" modIdx=0
      
      // -- mod="2.71828" dstVar=out
      out = 2.71828f;
      
      // -- mod="pow" dstVar=out
      
      // ---- mod="pow" input "exp" seq 1/1
      
      // -- mod="$x" dstVar=voice->tmp15_exp
      voice->tmp15_exp = x;
      
      // -- mod="20.9701" dstVar=voice->tmp15_exp
      voice->tmp16_seq = 20.9701f;
      voice->tmp15_exp *= voice->tmp16_seq;
      
      // -- mod="16.1181" dstVar=voice->tmp15_exp
      voice->tmp17_seq = 16.1181f;
      voice->tmp15_exp -= voice->tmp17_seq;
      out = mathPowerf(out, voice->tmp15_exp);
      _d[i] = out;

      // Next x
      x += 0.00048852f;
   } // loop 2048
} /* end loc_calc_lut_exp() */

void loc_prepare(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_filter_fm3s_2_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_filter_fm3s_2_shared_t);

   float out = 0.0f;
   // -------- lane "prepare" modIdx=0
   
   // -- mod="$m_ratio2" dstVar=out
   out = voice->mod_ratio2_cur;
   
   // -- mod="fma" dstVar=out
   out = (out * 2.0f) + -1.0f;
   
   // -- mod="bts" dstVar=out
   out = loc_bipolar_to_scale(out, 4.0f, 64.0f);
   
   // -- mod="qua" dstVar=out
   out = ((int)(out * 32.0f) / 32.0f);
   
   // -- mod="sto v_ratio2" dstVar=out
   voice->var_v_ratio2 = out;
   
   // -- mod="$m_level2" dstVar=out
   out = voice->mod_level2_cur;
   
   // -- mod="64" dstVar=out
   voice->tmp18_seq = 64.0f;
   out *= voice->tmp18_seq;
   
   // -- mod="clp" dstVar=out
   if(out > 64.0f) out = 64.0f;
   else if(out < 1e-07f) out = 1e-07f;
   
   // -- mod="sto v_level2" dstVar=out
   voice->var_v_level2 = out;
   
   // -- mod="log" dstVar=out
   out = mathLogf(out);
   
   // -- mod="sto v_loglevel2" dstVar=out
   voice->var_v_loglevel2 = out;
   
   // -- mod="$m_ratio3" dstVar=out
   out = voice->mod_ratio3_cur;
   
   // -- mod="fma" dstVar=out
   out = (out * 2.0f) + -1.0f;
   
   // -- mod="bts" dstVar=out
   out = loc_bipolar_to_scale(out, 4.0f, 64.0f);
   
   // -- mod="qua" dstVar=out
   out = ((int)(out * 100.0f) / 100.0f);
   
   // -- mod="sto v_ratio3" dstVar=out
   voice->var_v_ratio3 = out;
   
   // -- mod="$m_level3" dstVar=out
   out = voice->mod_level3_cur;
   
   // -- mod="4" dstVar=out
   voice->tmp19_seq = 4.0f;
   out *= voice->tmp19_seq;
   
   // -- mod="clp" dstVar=out
   if(out > 64.0f) out = 64.0f;
   else if(out < 1e-07f) out = 1e-07f;
   
   // -- mod="sto v_level3" dstVar=out
   voice->var_v_level3 = out;
   
   // -- mod="log" dstVar=out
   out = mathLogf(out);
   
   // -- mod="sto v_loglevel3" dstVar=out
   voice->var_v_loglevel3 = out;
} /* end prepare */

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
   ST_PLUGIN_SHARED_CAST(osc_filter_fm3s_2_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(osc_filter_fm3s_2_shared_t);
   shared->params[_paramIdx] = _value;
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
   ST_PLUGIN_VOICE_CAST(osc_filter_fm3s_2_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(osc_filter_fm3s_2_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_filter_fm3s_2_shared_t);
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


      voice->tmp2_sin_phase = 0.0f;
      voice->tmp9_sin_phase = 0.0f;
      voice->tmp20_pha_phase = 0.0f;
      voice->tmp25_pha_phase = 0.0f;
      voice->tmp33_svf_lp = 0.0f;
      voice->tmp34_svf_hp = 0.0f;
      voice->tmp35_svf_bp = 0.0f;
      voice->tmp39_pha_phase = 0.0f;

      voice->var_x = 0.0f;
      voice->var_v_ratio2 = 0.0f;
      voice->var_v_level2 = 0.0f;
      voice->var_v_loglevel2 = 0.0f;
      voice->var_v_ratio3 = 0.0f;
      voice->var_v_level3 = 0.0f;
      voice->var_v_loglevel3 = 0.0f;


      if(!shared->b_lut_logsinexp) { loc_calc_lut_logsinexp(&voice->base, shared->lut_logsinexp); shared->b_lut_logsinexp = 1; }
      if(!shared->b_lut_logsin) { loc_calc_lut_logsin(&voice->base, shared->lut_logsin); shared->b_lut_logsin = 1; }
      if(!shared->b_lut_exp) { loc_calc_lut_exp(&voice->base, shared->lut_exp); shared->b_lut_exp = 1; }
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(osc_filter_fm3s_2_voice_t);
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
   ST_PLUGIN_VOICE_CAST(osc_filter_fm3s_2_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_filter_fm3s_2_shared_t);
   (void)_note;
   (void)_vol;
   (void)_pan;

#ifdef OVERSAMPLE_FACTOR
   float noteSpeed = _freqHz / (voice->sample_rate * OVERSAMPLE_FACTOR);
#else
   float noteSpeed = _freqHz / voice->sample_rate;
#endif // OVERSAMPLE_FACTOR

   float modratio2       = shared->params[PARAM_RATIO2      ]                       + voice->mods[MOD_RATIO2       ];
   float modlevel2       = shared->params[PARAM_LEVEL2      ]                       + voice->mods[MOD_LEVEL2       ];
   float modratio3       = shared->params[PARAM_RATIO3      ]                       + voice->mods[MOD_RATIO3       ];
   float modlevel3       = shared->params[PARAM_LEVEL3      ]                       + voice->mods[MOD_LEVEL3       ];
   float modcutoff       = shared->params[PARAM_CUTOFF      ]                       + voice->mods[MOD_CUTOFF       ];
   float modres          = shared->params[PARAM_RES         ]                       + voice->mods[MOD_RES          ];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->note_speed_inc = (noteSpeed - voice->note_speed_cur) * recBlockSize;
      voice->note_inc       = (_note - voice->note_cur) * recBlockSize;
      voice->mod_ratio2_inc       = (modratio2          - voice->mod_ratio2_cur        ) * recBlockSize;
      voice->mod_level2_inc       = (modlevel2          - voice->mod_level2_cur        ) * recBlockSize;
      voice->mod_ratio3_inc       = (modratio3          - voice->mod_ratio3_cur        ) * recBlockSize;
      voice->mod_level3_inc       = (modlevel3          - voice->mod_level3_cur        ) * recBlockSize;
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
      voice->mod_ratio2_cur       = modratio2;
      voice->mod_ratio2_inc       = 0.0f;
      voice->mod_level2_cur       = modlevel2;
      voice->mod_level2_inc       = 0.0f;
      voice->mod_ratio3_cur       = modratio3;
      voice->mod_ratio3_inc       = 0.0f;
      voice->mod_level3_cur       = modlevel3;
      voice->mod_level3_inc       = 0.0f;
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

   ST_PLUGIN_VOICE_CAST(osc_filter_fm3s_2_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(osc_filter_fm3s_2_shared_t);

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
         voice->tmp23_freq = 1;
         voice->tmp21_pha_speed = voice->note_speed_cur * voice->tmp23_freq;
         
         // ---- mod="pha" input "phase" seq 1/1
         
         // -- mod="pha" dstVar=voice->tmp24_phase
         
         // ---- mod="pha" input "freq" seq 1/1
         
         // -- mod="$v_ratio2" dstVar=voice->tmp28_freq
         voice->tmp28_freq = voice->var_v_ratio2;
         voice->tmp26_pha_speed = voice->note_speed_cur * voice->tmp28_freq;
         voice->tmp27_pha_tmp = ffrac_s(voice->tmp25_pha_phase);
         voice->tmp24_phase = voice->tmp27_pha_tmp;
         voice->tmp25_pha_phase = ffrac_s(voice->tmp25_pha_phase + voice->tmp26_pha_speed);
         
         // -- mod="lut" dstVar=voice->tmp24_phase
         voice->tmp24_phase = shared->lut_logsin[((unsigned int)(voice->tmp24_phase * 255)) & 255];
         
         // -- mod="$v_loglevel2" dstVar=voice->tmp24_phase
         voice->tmp29_seq = voice->var_v_loglevel2;
         voice->tmp24_phase += voice->tmp29_seq;
         
         // -- mod="16.1181" dstVar=voice->tmp24_phase
         voice->tmp30_seq = 16.1181f;
         voice->tmp24_phase += voice->tmp30_seq;
         
         // -- mod="0.0476869" dstVar=voice->tmp24_phase
         voice->tmp31_seq = 0.0476869f;
         voice->tmp24_phase *= voice->tmp31_seq;
         
         // -- mod="lut" dstVar=voice->tmp24_phase
         voice->tmp24_phase = shared->lut_exp[((unsigned int)(voice->tmp24_phase * 2047)) & 2047];
         
         // -- mod="$v_level2" dstVar=voice->tmp24_phase
         voice->tmp32_seq = voice->var_v_level2;
         voice->tmp24_phase -= voice->tmp32_seq;
         voice->tmp22_pha_tmp = ffrac_s((voice->tmp20_pha_phase + voice->tmp24_phase));
         out = voice->tmp22_pha_tmp;
         voice->tmp20_pha_phase = ffrac_s(voice->tmp20_pha_phase + voice->tmp21_pha_speed);
         
         // -- mod="lut" dstVar=out
         out = shared->lut_logsinexp[((unsigned int)(out * 255)) & 255];
         
         // -- mod="svf" dstVar=out
         
         // ---- mod="svf" input "res" seq 1/1
         
         // -- mod="$m_res" dstVar=voice->tmp36_res
         voice->tmp36_res = voice->mod_res_cur;
         
         // ---- mod="svf" input "freq" seq 1/1
         
         // -- mod="$m_cutoff" dstVar=voice->tmp37_freq
         voice->tmp37_freq = voice->mod_cutoff_cur;
         
         // -- mod="pha" dstVar=voice->tmp37_freq
         
         // ---- mod="pha" input "freq" seq 1/1
         
         // -- mod="$v_ratio3" dstVar=voice->tmp42_freq
         voice->tmp42_freq = voice->var_v_ratio3;
         voice->tmp40_pha_speed = voice->note_speed_cur * voice->tmp42_freq;
         voice->tmp41_pha_tmp = ffrac_s(voice->tmp39_pha_phase);
         voice->tmp38_seq = voice->tmp41_pha_tmp;
         voice->tmp39_pha_phase = ffrac_s(voice->tmp39_pha_phase + voice->tmp40_pha_speed);
         
         // -- mod="lut" dstVar=voice->tmp38_seq
         voice->tmp38_seq = shared->lut_logsin[((unsigned int)(voice->tmp38_seq * 255)) & 255];
         
         // -- mod="$v_loglevel3" dstVar=voice->tmp38_seq
         voice->tmp43_seq = voice->var_v_loglevel3;
         voice->tmp38_seq += voice->tmp43_seq;
         
         // -- mod="16.1181" dstVar=voice->tmp38_seq
         voice->tmp44_seq = 16.1181f;
         voice->tmp38_seq += voice->tmp44_seq;
         
         // -- mod="0.0476869" dstVar=voice->tmp38_seq
         voice->tmp45_seq = 0.0476869f;
         voice->tmp38_seq *= voice->tmp45_seq;
         
         // -- mod="lut" dstVar=voice->tmp38_seq
         voice->tmp38_seq = shared->lut_exp[((unsigned int)(voice->tmp38_seq * 2047)) & 2047];
         
         // -- mod="$v_level3" dstVar=voice->tmp38_seq
         voice->tmp46_seq = voice->var_v_level3;
         voice->tmp38_seq -= voice->tmp46_seq;
         voice->tmp37_freq += voice->tmp38_seq;
         
         // -- mod="clp" dstVar=voice->tmp37_freq
         if(voice->tmp37_freq > 1.0f) voice->tmp37_freq = 1.0f;
         else if(voice->tmp37_freq < 0.001f) voice->tmp37_freq = 0.001f;
         voice->tmp33_svf_lp = voice->tmp33_svf_lp + (voice->tmp35_svf_bp * voice->tmp37_freq);
         voice->tmp34_svf_hp = out - voice->tmp33_svf_lp - (voice->tmp35_svf_bp * voice->tmp36_res);
         voice->tmp35_svf_bp = voice->tmp35_svf_bp + (voice->tmp34_svf_hp * voice->tmp37_freq);
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
      voice->mod_ratio2_cur        += voice->mod_ratio2_inc;
      voice->mod_level2_cur        += voice->mod_level2_inc;
      voice->mod_ratio3_cur        += voice->mod_ratio3_inc;
      voice->mod_level3_cur        += voice->mod_level3_inc;
      voice->mod_cutoff_cur        += voice->mod_cutoff_inc;
      voice->mod_res_cur           += voice->mod_res_inc;
   } /* loop numFrames */
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   osc_filter_fm3s_2_shared_t *ret = (osc_filter_fm3s_2_shared_t *)malloc(sizeof(osc_filter_fm3s_2_shared_t));
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
   osc_filter_fm3s_2_voice_t *voice = (osc_filter_fm3s_2_voice_t *)malloc(sizeof(osc_filter_fm3s_2_voice_t));
   if(NULL != voice)
   {
      memset((void*)voice, 0, sizeof(*voice));
      voice->base.info = _info;
   }
   return &voice->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(osc_filter_fm3s_2_voice_t);

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
st_plugin_info_t *osc_filter_fm3s_2_init(void) {
   osc_filter_fm3s_2_info_t *ret = (osc_filter_fm3s_2_info_t *)malloc(sizeof(osc_filter_fm3s_2_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "osc_filter_fm3s_2";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "osc filter fm3s 2";
      ret->base.short_name  = "osc filter fm3s 2";
      ret->base.flags       = ST_PLUGIN_FLAG_OSC;
      ret->base.category    = ST_PLUGIN_CAT_UNKNOWN;
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
      ret->base.set_sample_rate  = &loc_set_sample_rate;
      ret->base.note_on          = &loc_note_on;
      ret->base.set_mod_value    = &loc_set_mod_value;
      ret->base.prepare_block    = &loc_prepare_block;
      ret->base.process_replace  = &loc_process_replace;
      ret->base.plugin_exit      = &loc_plugin_exit;

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
         return osc_filter_fm3s_2_init();
   }
   return NULL;
}
#endif // STFX_SKIP_MAIN_INIT

} // extern "C"
