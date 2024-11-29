// ----
// ---- file   : saw_vsync_svf.cpp
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
// ----           $ g++ -Wall -Wno-unused-function -Wno-unused-variable -I../../tksampler -c saw_vsync_svf.cpp -o saw_vsync_svf.o
// ---- created: 08Aug2023 12:33:59
// ----
// ----
// ----

#include <plugin.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>


#define OVERSAMPLE_FACTOR  8.0f

#define PARAM_VSYNC              0
#define PARAM_FREQ               1
#define PARAM_RES                2
#define PARAM_DETUNE             3
#define NUM_PARAMS               4
static const char *loc_param_names[NUM_PARAMS] = {
   "vsync",                   // 0: VSYNC
   "freq",                    // 1: FREQ
   "res",                     // 2: RES
   "detune",                  // 3: DETUNE

};
static float loc_param_resets[NUM_PARAMS] = {
   0.15f,                     // 0: VSYNC
   0.8f,                      // 1: FREQ
   0.7f,                      // 2: RES
   0.5f,                      // 3: DETUNE

};

#define MOD_VSYNC                0
#define MOD_FREQ                 1
#define MOD_RES                  2
#define MOD_DETUNE               3
#define NUM_MODS                 4
static const char *loc_mod_names[NUM_MODS] = {
   "vsync",        // 0: VSYNC
   "freq",         // 1: FREQ
   "res",          // 2: RES
   "detune",       // 3: DETUNE

};

typedef struct saw_vsync_svf_info_s {
   st_plugin_info_t base;
} saw_vsync_svf_info_t;

typedef struct saw_vsync_svf_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];

} saw_vsync_svf_shared_t;

typedef struct saw_vsync_svf_voice_s {
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
   float mod_freq_cur;
   float mod_freq_inc;
   float mod_res_cur;
   float mod_res_inc;
   float mod_detune_cur;
   float mod_detune_inc;

   short tmp1_i2f;
   unsigned short tmp2_lfsr_state;
   short tmp3_lfsr_state_signed;
   short tmp4_i2f;
   unsigned short tmp5_lfsr_state;
   short tmp6_lfsr_state_signed;
   float tmp7_kbd;
   float tmp8_off;
   float tmp9_scl;
   float tmp10_saw_phase;
   float tmp11_win_phase;
   float tmp12_saw_speed;
   float tmp13_freq;
   float tmp14_saw_tmp;
   float tmp15_phase;
   float tmp16_vsync;
   float tmp17_window;
   float tmp18_seq;
   float tmp19_saw_phase;
   float tmp20_win_phase;
   float tmp21_saw_speed;
   float tmp22_freq;
   float tmp23_saw_tmp;
   float tmp24_phase;
   float tmp25_vsync;
   float tmp26_window;
   float tmp27_svf_lp;
   float tmp28_svf_hp;
   float tmp29_svf_bp;
   float tmp30_res;
   float tmp31_freq;
   float tmp32_svf_lp;
   float tmp33_svf_hp;
   float tmp34_svf_bp;
   float tmp35_res;
   float tmp36_freq;
   float var_x;
   float var_v_freq;
   float var_v_phase1;
   float var_v_phase2;
   float var_v_detune;

} saw_vsync_svf_voice_t;

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
   ST_PLUGIN_VOICE_CAST(saw_vsync_svf_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(saw_vsync_svf_shared_t);

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
   
   // -- mod="sto v_phase1" dstVar=out
   voice->var_v_phase1 = out;
   
   // -- mod="fsr" dstVar=out
   voice->tmp4_i2f = (int)(out * 2048);  // IntFallback: F2I
   voice->tmp5_lfsr_state ^= voice->tmp5_lfsr_state >> 7;
   voice->tmp5_lfsr_state ^= voice->tmp5_lfsr_state << 9;
   voice->tmp5_lfsr_state ^= voice->tmp5_lfsr_state >> 13;
   voice->tmp6_lfsr_state_signed = (voice->tmp5_lfsr_state & 65520);
   voice->tmp4_i2f = voice->tmp6_lfsr_state_signed >> 4;
   out = voice->tmp4_i2f / ((float)(2048));  // IntFallback: I2F
   
   // -- mod="sto v_phase2" dstVar=out
   voice->var_v_phase2 = out;
} /* end init */

void loc_prepare(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(saw_vsync_svf_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(saw_vsync_svf_shared_t);

   float out = 0.0f;
   // -------- lane "prepare" modIdx=0
   
   // -- mod="$m_freq" dstVar=out
   out = voice->mod_freq_cur;
   
   // -- mod="kbd" dstVar=out
   out *= 127.0f;
   
   // ---- mod="kbd" input "kbd" seq 1/1
   
   // -- mod="$KBD_AMT" dstVar=voice->tmp7_kbd
   voice->tmp7_kbd = 0.35f;
   voice->tmp8_off = 0;
   voice->tmp9_scl = 4;
   out += (voice->note_cur + (voice->tmp8_off * 12.0f) - 64.0f) * voice->tmp7_kbd;
   out = clamp(out, 0.0f, 127.0f);
   #ifdef OVERSAMPLE_FACTOR
   out = ((440.0f/32.0f)*expf( ((out-9.0f)/12.0f)*logf(2.0f) )) / (voice->sample_rate * OVERSAMPLE_FACTOR);
   #else
   out = ((440.0f/32.0f)*expf( ((out-9.0f)/12.0f)*logf(2.0f) )) / voice->sample_rate;
   #endif
   out *= voice->tmp9_scl;
   
   // -- mod="sto v_freq" dstVar=out
   voice->var_v_freq = out;
   
   // -- mod="$m_detune" dstVar=out
   out = voice->mod_detune_cur;
   
   // -- mod="fma" dstVar=out
   out = (out * 2.0f) + -1.0f;
   
   // -- mod="bts" dstVar=out
   out = loc_bipolar_to_scale(out, 2.0f, 2.0f);
   
   // -- mod="sto v_detune" dstVar=out
   voice->var_v_detune = out;
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
   ST_PLUGIN_SHARED_CAST(saw_vsync_svf_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(saw_vsync_svf_shared_t);
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
   ST_PLUGIN_VOICE_CAST(saw_vsync_svf_voice_t);
   voice->sample_rate = _sampleRate;
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(saw_vsync_svf_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(saw_vsync_svf_shared_t);
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
      voice->tmp6_lfsr_state_signed = 0;
      voice->tmp10_saw_phase = 0.0f;
      voice->tmp11_win_phase = 0.0f;
      voice->tmp19_saw_phase = 0.0f;
      voice->tmp20_win_phase = 0.0f;
      voice->tmp27_svf_lp = 0.0f;
      voice->tmp28_svf_hp = 0.0f;
      voice->tmp29_svf_bp = 0.0f;
      voice->tmp32_svf_lp = 0.0f;
      voice->tmp33_svf_hp = 0.0f;
      voice->tmp34_svf_bp = 0.0f;

      voice->var_x = 0.0f;
      voice->var_v_freq = 0.0f;
      voice->var_v_phase1 = 0.0f;
      voice->var_v_phase2 = 0.0f;
      voice->var_v_detune = 0.0f;

      loc_init(&voice->base);

   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(saw_vsync_svf_voice_t);
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
   ST_PLUGIN_VOICE_CAST(saw_vsync_svf_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(saw_vsync_svf_shared_t);
   (void)_note;
   (void)_vol;
   (void)_pan;

#ifdef OVERSAMPLE_FACTOR
   float noteSpeed = _freqHz / (voice->sample_rate * OVERSAMPLE_FACTOR);
#else
   float noteSpeed = _freqHz / voice->sample_rate;
#endif // OVERSAMPLE_FACTOR

   float modvsync        = shared->params[PARAM_VSYNC       ] * 14.0f + 1.0f        + voice->mods[MOD_VSYNC        ];
   float modfreq         = shared->params[PARAM_FREQ        ]                       + voice->mods[MOD_FREQ         ];
   float modres          = shared->params[PARAM_RES         ]                       + voice->mods[MOD_RES          ];
   float moddetune       = shared->params[PARAM_DETUNE      ]                       + voice->mods[MOD_DETUNE       ];

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->note_speed_inc = (noteSpeed - voice->note_speed_cur) * recBlockSize;
      voice->note_inc       = (_note - voice->note_cur) * recBlockSize;
      voice->mod_vsync_inc        = (modvsync           - voice->mod_vsync_cur         ) * recBlockSize;
      voice->mod_freq_inc         = (modfreq            - voice->mod_freq_cur          ) * recBlockSize;
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
      voice->mod_vsync_cur        = modvsync;
      voice->mod_vsync_inc        = 0.0f;
      voice->mod_freq_cur         = modfreq;
      voice->mod_freq_inc         = 0.0f;
      voice->mod_res_cur          = modres;
      voice->mod_res_inc          = 0.0f;
      voice->mod_detune_cur       = moddetune;
      voice->mod_detune_inc       = 0.0f;

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

   ST_PLUGIN_VOICE_CAST(saw_vsync_svf_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(saw_vsync_svf_shared_t);

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
         
         // -- mod="saw" dstVar=out
         voice->tmp13_freq = 1;
         voice->tmp12_saw_speed = voice->note_speed_cur * voice->tmp13_freq;
         
         // ---- mod="saw" input "phase" seq 1/1
         
         // -- mod="$v_phase1" dstVar=voice->tmp15_phase
         voice->tmp15_phase = voice->var_v_phase1;
         
         // ---- mod="saw" input "vsync" seq 1/1
         
         // -- mod="$m_vsync" dstVar=voice->tmp16_vsync
         voice->tmp16_vsync = voice->mod_vsync_cur;
         voice->tmp14_saw_tmp = ((voice->tmp10_saw_phase + voice->tmp15_phase));
         voice->tmp14_saw_tmp = ffrac_s(voice->tmp14_saw_tmp);
         out = 1.0 - (voice->tmp14_saw_tmp * 2.0f);
         voice->tmp17_window = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp11_win_phase)&16383u];
         voice->tmp17_window *= voice->tmp17_window;
         out *= voice->tmp17_window;
         voice->tmp10_saw_phase = ffrac_s(voice->tmp10_saw_phase + voice->tmp12_saw_speed * voice->tmp16_vsync);
         tmp_f = voice->tmp11_win_phase;
         voice->tmp11_win_phase = ffrac_s(voice->tmp11_win_phase + voice->tmp12_saw_speed);
         if(tmp_f > voice->tmp11_win_phase) voice->tmp10_saw_phase = voice->tmp11_win_phase * voice->tmp16_vsync; 
         
         // -- mod="saw" dstVar=out
         
         // ---- mod="saw" input "freq" seq 1/1
         
         // -- mod="$v_detune" dstVar=voice->tmp22_freq
         voice->tmp22_freq = voice->var_v_detune;
         voice->tmp21_saw_speed = voice->note_speed_cur * voice->tmp22_freq;
         
         // ---- mod="saw" input "phase" seq 1/1
         
         // -- mod="$v_phase2" dstVar=voice->tmp24_phase
         voice->tmp24_phase = voice->var_v_phase2;
         
         // ---- mod="saw" input "vsync" seq 1/1
         
         // -- mod="$m_vsync" dstVar=voice->tmp25_vsync
         voice->tmp25_vsync = voice->mod_vsync_cur;
         voice->tmp23_saw_tmp = ((voice->tmp19_saw_phase + voice->tmp24_phase));
         voice->tmp23_saw_tmp = ffrac_s(voice->tmp23_saw_tmp);
         voice->tmp18_seq = 1.0 - (voice->tmp23_saw_tmp * 2.0f);
         voice->tmp26_window = cycle_sine_tbl_f[(unsigned short)(16384 * voice->tmp20_win_phase)&16383u];
         voice->tmp26_window *= voice->tmp26_window;
         voice->tmp18_seq *= voice->tmp26_window;
         voice->tmp19_saw_phase = ffrac_s(voice->tmp19_saw_phase + voice->tmp21_saw_speed * voice->tmp25_vsync);
         tmp_f = voice->tmp20_win_phase;
         voice->tmp20_win_phase = ffrac_s(voice->tmp20_win_phase + voice->tmp21_saw_speed);
         if(tmp_f > voice->tmp20_win_phase) voice->tmp19_saw_phase = voice->tmp20_win_phase * voice->tmp25_vsync; 
         out += voice->tmp18_seq;
         
         // -- mod="svf" dstVar=out
         
         // ---- mod="svf" input "res" seq 1/1
         
         // -- mod="$m_res" dstVar=voice->tmp30_res
         voice->tmp30_res = voice->mod_res_cur;
         
         // ---- mod="svf" input "freq" seq 1/1
         
         // -- mod="$v_freq" dstVar=voice->tmp31_freq
         voice->tmp31_freq = voice->var_v_freq;
         voice->tmp27_svf_lp = voice->tmp27_svf_lp + (voice->tmp29_svf_bp * voice->tmp31_freq);
         voice->tmp28_svf_hp = out - voice->tmp27_svf_lp - (voice->tmp29_svf_bp * voice->tmp30_res);
         voice->tmp29_svf_bp = voice->tmp29_svf_bp + (voice->tmp28_svf_hp * voice->tmp31_freq);
         out = voice->tmp27_svf_lp;
         
         // -- mod="svf" dstVar=out
         
         // ---- mod="svf" input "res" seq 1/1
         
         // -- mod="$m_res" dstVar=voice->tmp35_res
         voice->tmp35_res = voice->mod_res_cur;
         
         // ---- mod="svf" input "freq" seq 1/1
         
         // -- mod="$v_freq" dstVar=voice->tmp36_freq
         voice->tmp36_freq = voice->var_v_freq;
         voice->tmp32_svf_lp = voice->tmp32_svf_lp + (voice->tmp34_svf_bp * voice->tmp36_freq);
         voice->tmp33_svf_hp = out - voice->tmp32_svf_lp - (voice->tmp34_svf_bp * voice->tmp35_res);
         voice->tmp34_svf_bp = voice->tmp34_svf_bp + (voice->tmp33_svf_hp * voice->tmp36_freq);
         out = voice->tmp32_svf_lp;
   
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
      voice->mod_freq_cur          += voice->mod_freq_inc;
      voice->mod_res_cur           += voice->mod_res_inc;
      voice->mod_detune_cur        += voice->mod_detune_inc;
   } /* loop numFrames */
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   saw_vsync_svf_shared_t *ret = (saw_vsync_svf_shared_t *)malloc(sizeof(saw_vsync_svf_shared_t));
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
   saw_vsync_svf_voice_t *voice = (saw_vsync_svf_voice_t *)malloc(sizeof(saw_vsync_svf_voice_t));
   if(NULL != voice)
   {
      memset((void*)voice, 0, sizeof(*voice));
      voice->base.info = _info;
      voice->tmp2_lfsr_state = 17545;
      voice->tmp5_lfsr_state = 4660;
   }
   return &voice->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(saw_vsync_svf_voice_t);

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
st_plugin_info_t *saw_vsync_svf_init(void) {
   saw_vsync_svf_info_t *ret = (saw_vsync_svf_info_t *)malloc(sizeof(saw_vsync_svf_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "saw_vsync_svf";  // unique id. don't change this in future builds.
      ret->base.author      = "bsp";
      ret->base.name        = "saw vsync svf";
      ret->base.short_name  = "saw vsync svf";
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
         return saw_vsync_svf_init();
   }
   return NULL;
}
#endif // STFX_SKIP_MAIN_INIT

} // extern "C"
