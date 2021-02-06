// ----
// ---- file   : lrt_vampyr.cpp
// ---- author : Patrick Lindenberg & bsp
// ---- legal  : (c) 2017-2020 by Patrick Lindenberg
// ----
// ----
// ---- info   : dual lpf+hpf type 35 filter
// ----           (adapted for 'stfx' plugin API by bsp)
// ----
// ---- created: 07Jun2020
// ---- changed: 08Jun2020
// ----
// ----
// ----

#include <stdlib.h>
#include <math.h>

#include "../../../plugin.h"

#include "dsp/Type35Filter.h"

#define PARAM_DRYWET      0
#define PARAM_IO_DRIVE    1 
#define PARAM_FREQ        2
#define PARAM_Q           3
#define PARAM_DRIVE       4
#define PARAM_FREQ_HPF    5
#define PARAM_Q_HPF       6
#define PARAM_MODE        7  // 0=LPF, 0.2=HPF->LPF, 0.4=LPF+HPF, 0.6=LPF->HPF, 0.8=HPF
#define NUM_PARAMS        8
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry / Wet",
   "I/O Drive",
   "Freq",
   "Q",
   "Saturation",
   "Freq HPF",
   "Q HPF",
   "Mode"
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // DRYWET
   0.5f,  // IO_DRIVE
   1.0f,  // FREQ
   0.0f,  // Q
   0.0f,  // DRIVE/SATURATION
   0.0f,  // FREQ_HPF
   0.0f,  // Q_HPF
   0.2f,  // MODE
};

#define MOD_DRYWET      0
#define MOD_FREQ        1
#define MOD_Q           2
#define MOD_DRIVE       3
#define MOD_FREQ_HPF    4
#define MOD_Q_HPF       5
#define MOD_DRIVE_HPF   6
#define MOD_IO_DRIVE    7
#define NUM_MODS        8
static const char *loc_mod_names[NUM_MODS] = {
   "Dry / Wet",
   "Freq",
   "Q",
   "Saturation",
   "Freq HPF",
   "Q HPF",
   "Saturation HPF",
   "I/O Drive"
};

typedef struct lrt_vampyr_info_s {
   st_plugin_info_t base;
} lrt_vampyr_info_t;

typedef struct lrt_vampyr_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} lrt_vampyr_shared_t;

struct VampyrFilter {
   dsp::Type35Filter lpf;
   dsp::Type35Filter hpf;

   VampyrFilter(void) : lpf(44100.0f, dsp::Type35Filter::LPF), hpf(44100.0f, dsp::Type35Filter::HPF) {
   }

   ~VampyrFilter(void) {
   }

   inline void setSampleRate(float _sampleRate) {
      lpf.setSamplerate(_sampleRate);
      hpf.setSamplerate(_sampleRate);
   }

   inline void reset(void) {
      lpf.init();
      hpf.init();
   }

   inline float process(const float _inSmp,
                        const float _freq,
                        const float _q,
                        const float _drive,
                        const float _freqHPF,
                        const float _qHPF,
                        const float _driveHPF,
                        const int   _mode
                        ) {

      lpf.fc   = _freq;
      lpf.peak = _q;
      hpf.fc   = _freqHPF;
      hpf.peak = _qHPF;

      lpf.sat = _drive;
      hpf.sat = _driveHPF;

      float out = 0.0f;

      switch(_mode)
      {
         case 0:  // LPF
         default:
            lpf.in = _inSmp;
            lpf.invalidate();
            lpf.process2();
            
            out = lpf.out;
            break;

         case 1:  // HPF->LPF
            hpf.in = _inSmp;
            hpf.invalidate();
            hpf.process2();
            
            lpf.in = hpf.out;
            lpf.invalidate();
            lpf.process2();
            
            out = lpf.out;
            break;

         case 2:  // LPF+HPF
            lpf.in = _inSmp;
            lpf.invalidate();
            lpf.process2();
            
            hpf.in = _inSmp;
            hpf.invalidate();
            hpf.process2();
            
            out = hpf.out + lpf.out;
            break;

         case 3: // LPF->HPF
            lpf.in = _inSmp;
            lpf.invalidate();
            lpf.process2();
            
            hpf.in = lpf.out;
            hpf.invalidate();
            hpf.process2();

            out = hpf.out;
            break;

         case 4: // HPF
            hpf.in = _inSmp;
            hpf.invalidate();
            hpf.process2();
            
            out = hpf.out;
            break;
      }

      return out;
   }
      
};

typedef struct lrt_vampyr_voice_s {
   st_plugin_voice_t base;
   float        sample_rate;
   float        mods[NUM_MODS];
   float        mod_drywet_cur;
   float        mod_drywet_inc;
   float        mod_in_gain_cur;
   float        mod_in_gain_inc;
   float        mod_out_gain_cur;
   float        mod_out_gain_inc;
   float        mod_freq_cur;
   float        mod_freq_inc;
   float        mod_q_cur;
   float        mod_q_inc;
   float        mod_q_gain_cur;
   float        mod_q_gain_inc;
   float        mod_drive_cur;
   float        mod_drive_inc;
   float        mod_freq_hpf_cur;
   float        mod_freq_hpf_inc;
   float        mod_q_hpf_cur;
   float        mod_q_hpf_inc;
   float        mod_drive_hpf_cur;
   float        mod_drive_hpf_inc;
   int          mode;  // 0..4
   VampyrFilter flt_l;
   VampyrFilter flt_r;
} lrt_vampyr_voice_t;


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
   ST_PLUGIN_SHARED_CAST(lrt_vampyr_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(lrt_vampyr_shared_t);
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
   ST_PLUGIN_VOICE_CAST(lrt_vampyr_voice_t);
   voice->sample_rate = _sampleRate;
   voice->flt_l.setSampleRate(_sampleRate);
   voice->flt_r.setSampleRate(_sampleRate);
}

static void ST_PLUGIN_API loc_note_on(st_plugin_voice_t  *_voice,
                                      int                 _bGlide,
                                      unsigned char       _note,
                                      float               _vel
                                      ) {
   ST_PLUGIN_VOICE_CAST(lrt_vampyr_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
      voice->flt_l.reset();
      voice->flt_r.reset();
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(lrt_vampyr_voice_t);
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
   ST_PLUGIN_VOICE_CAST(lrt_vampyr_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(lrt_vampyr_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   float modDryWet = shared->params[PARAM_DRYWET]   + voice->mods[MOD_DRYWET];
   modDryWet = Dstplugin_clamp(modDryWet, 0.0f, 1.0f);

   float modFreq = shared->params[PARAM_FREQ] + voice->mods[MOD_FREQ];
   modFreq = Dstplugin_clamp(modFreq, 0.0f, 1.0f);

   float modQ = shared->params[PARAM_Q] + voice->mods[MOD_Q];
   modQ = Dstplugin_clamp(modQ, 0.0f, 1.0f);
#if 0
   float s = modQ * modQ;
   float pGain = powf(10.0f, s*2.5f);
   pGain = Dstplugin_clamp(pGain, 0.0f, 2.3f);
   float lGain = modQ*1.4f;//modQ*1.0f;
   float thres = 0.5f;
   if(modQ >= thres)
   {
      s = (modQ - thres) / (1.0f - thres);
      s = s*s * (3.0f - 2.0f * s);
      pGain = pGain + (lGain - pGain) * s;
   }
   float modQGain = 0.2f + pGain;
   modQGain *= 1.5f;
   modQ *= dsp::DiodeLadderFilter::MAX_RESONANCE;
#else
   float modQGain = 1.0f;
#endif

   float modDrive = shared->params[PARAM_DRIVE] + voice->mods[MOD_DRIVE];
   modDrive = Dstplugin_clamp(modDrive, 0.0f, 1.0f);
   modDrive = 1.0f + modDrive * 1.5f;

   float modFreqHPF = shared->params[PARAM_FREQ_HPF] + voice->mods[MOD_FREQ_HPF];
   modFreqHPF = Dstplugin_clamp(modFreqHPF, 0.0f, 1.0f);

   float modQHPF = shared->params[PARAM_Q_HPF]*0.3f + voice->mods[MOD_Q_HPF]*0.3f;
   modQHPF = Dstplugin_clamp(modQHPF, 0.0f, 1.0f);

   float modDriveHPF = shared->params[PARAM_DRIVE] + voice->mods[MOD_DRIVE_HPF];
   modDriveHPF = Dstplugin_clamp(modDriveHPF, 0.0f, 1.0f);
   modDriveHPF = 1.0f + modDrive * 1.5f;

   voice->mode = int(shared->params[PARAM_MODE] * 5.0f + 0.5f);
   if(voice->mode > 4)
      voice->mode = 4;

   float ioDriveN = shared->params[PARAM_IO_DRIVE];
   float ioDrive = (ioDriveN-0.5f)*2.0f + voice->mods[MOD_IO_DRIVE];
   float modInGain  = powf(10.0f, ioDrive *  (3.0f + sinf(ioDriveN*ST_PLUGIN_PI_F)*1.0f));
   float modOutGain = powf(10.0f, ioDrive * -2.0f);

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_drywet_inc     = (modDryWet   - voice->mod_drywet_cur)    * recBlockSize;
      voice->mod_in_gain_inc    = (modInGain   - voice->mod_in_gain_cur)   * recBlockSize;
      voice->mod_out_gain_inc   = (modOutGain  - voice->mod_out_gain_cur)  * recBlockSize;
      voice->mod_freq_inc       = (modFreq     - voice->mod_freq_cur)      * recBlockSize;
      voice->mod_q_inc          = (modQ        - voice->mod_q_cur)         * recBlockSize;
      voice->mod_q_gain_inc     = (modQGain    - voice->mod_q_gain_cur)    * recBlockSize;
      voice->mod_drive_inc      = (modDrive    - voice->mod_drive_cur)     * recBlockSize;
      voice->mod_freq_hpf_inc   = (modFreqHPF  - voice->mod_freq_hpf_cur)  * recBlockSize;
      voice->mod_q_hpf_inc      = (modQHPF     - voice->mod_q_hpf_cur)     * recBlockSize;
      voice->mod_drive_hpf_inc  = (modDriveHPF - voice->mod_drive_hpf_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_drywet_cur     = modDryWet;
      voice->mod_drywet_inc     = 0.0f;
      voice->mod_in_gain_cur    = modInGain;
      voice->mod_in_gain_inc    = 0.0f;
      voice->mod_out_gain_cur   = modOutGain;
      voice->mod_out_gain_inc   = 0.0f;
      voice->mod_freq_cur       = modFreq;
      voice->mod_freq_inc       = 0.0f;
      voice->mod_q_cur          = modQ;
      voice->mod_q_inc          = 0.0f;
      voice->mod_q_gain_cur     = modQGain;
      voice->mod_q_gain_inc     = 0.0f;
      voice->mod_drive_cur      = modDrive;
      voice->mod_drive_inc      = 0.0f;
      voice->mod_freq_hpf_cur   = modFreqHPF;
      voice->mod_freq_hpf_inc   = 0.0f;
      voice->mod_q_hpf_cur      = modQHPF;
      voice->mod_q_hpf_inc      = 0.0f;
      voice->mod_drive_hpf_cur  = modDriveHPF;
      voice->mod_drive_hpf_inc  = 0.0f;
    }
}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut, 
                                              unsigned int        _numFrames
                                              ) {
   // Ring modulate at (modulated) note frequency
   ST_PLUGIN_VOICE_CAST(lrt_vampyr_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(lrt_vampyr_shared_t);

   unsigned int k = 0u;

   if(_bMonoIn)
   {
      // Mono input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];

         float outL = voice->flt_l.process(l * voice->mod_in_gain_cur,
                                           voice->mod_freq_cur,
                                           voice->mod_q_cur,
                                           voice->mod_drive_cur,
                                           voice->mod_freq_hpf_cur,
                                           voice->mod_q_hpf_cur,
                                           voice->mod_drive_hpf_cur,
                                           voice->mode
                                           );
         outL *= voice->mod_q_gain_cur;
         outL *= voice->mod_out_gain_cur;

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outL;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur    += voice->mod_drywet_inc;
         voice->mod_in_gain_cur   += voice->mod_in_gain_inc;
         voice->mod_out_gain_cur  += voice->mod_out_gain_inc;
         voice->mod_freq_cur      += voice->mod_freq_inc;
         voice->mod_q_cur         += voice->mod_q_inc;
         voice->mod_q_gain_cur    += voice->mod_q_gain_inc;
         voice->mod_drive_cur     += voice->mod_drive_inc;
         voice->mod_freq_hpf_cur  += voice->mod_freq_hpf_inc;
         voice->mod_q_hpf_cur     += voice->mod_q_hpf_inc;
         voice->mod_drive_hpf_cur += voice->mod_drive_hpf_inc;
      }
   }
   else
   {
      // Stereo input, stereo output
      for(unsigned int i = 0u; i < _numFrames; i++)
      {
         float l = _samplesIn[k];
         float r = _samplesIn[k + 1u];

         float outL = voice->flt_l.process(l * voice->mod_in_gain_cur,
                                           voice->mod_freq_cur,
                                           voice->mod_q_cur,
                                           voice->mod_drive_cur,
                                           voice->mod_freq_hpf_cur,
                                           voice->mod_q_hpf_cur,
                                           voice->mod_drive_hpf_cur,
                                           voice->mode
                                           );
         outL *= voice->mod_q_gain_cur;
         outL *= voice->mod_out_gain_cur;

         float outR = voice->flt_r.process(r * voice->mod_in_gain_cur,
                                           voice->mod_freq_cur,
                                           voice->mod_q_cur,
                                           voice->mod_drive_cur,
                                           voice->mod_freq_hpf_cur,
                                           voice->mod_q_hpf_cur,
                                           voice->mod_drive_hpf_cur,
                                           voice->mode
                                           );
         outR *= voice->mod_q_gain_cur;
         outR *= voice->mod_out_gain_cur;

         outL = l + (outL - l) * voice->mod_drywet_cur;
         outR = r + (outR - r) * voice->mod_drywet_cur;
         outL = Dstplugin_fix_denorm_32(outL);
         outR = Dstplugin_fix_denorm_32(outR);
         _samplesOut[k]      = outL;
         _samplesOut[k + 1u] = outR;

         // Next frame
         k += 2u;
         voice->mod_drywet_cur    += voice->mod_drywet_inc;
         voice->mod_in_gain_cur   += voice->mod_in_gain_inc;
         voice->mod_out_gain_cur  += voice->mod_out_gain_inc;
         voice->mod_freq_cur      += voice->mod_freq_inc;
         voice->mod_q_cur         += voice->mod_q_inc;
         voice->mod_q_gain_cur    += voice->mod_q_gain_inc;
         voice->mod_drive_cur     += voice->mod_drive_inc;
         voice->mod_freq_hpf_cur  += voice->mod_freq_hpf_inc;
         voice->mod_q_hpf_cur     += voice->mod_q_hpf_inc;
         voice->mod_drive_hpf_cur += voice->mod_drive_hpf_inc;
      }
   }
}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   lrt_vampyr_shared_t *ret = (lrt_vampyr_shared_t *)malloc(sizeof(lrt_vampyr_shared_t));
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

static st_plugin_voice_t *ST_PLUGIN_API loc_voice_new(st_plugin_info_t *_info) {
   lrt_vampyr_voice_t *ret = (lrt_vampyr_voice_t *)malloc(sizeof(lrt_vampyr_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info   = _info;
      new(&ret->flt_l)VampyrFilter();
      new(&ret->flt_r)VampyrFilter();
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(lrt_vampyr_voice_t);
   voice->flt_l.~VampyrFilter();
   voice->flt_r.~VampyrFilter();
   free((void*)_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free((void*)_info);
}

extern "C" {
st_plugin_info_t *lrt_vampyr_init(void) {
   lrt_vampyr_info_t *ret = (lrt_vampyr_info_t *)malloc(sizeof(lrt_vampyr_info_t));

   if(NULL != ret)
   {
      memset(ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "lrt vampyr 35 dual filter";  // unique id. don't change this in future builds.
      ret->base.author      = "lrt & bsp";
      ret->base.name        = "lrt vampyr 35 lpf+hpf";
      ret->base.short_name  = "lrt vampyr 35 lpf+hpf";
      ret->base.flags       = ST_PLUGIN_FLAG_FX;
      ret->base.category    = ST_PLUGIN_CAT_FILTER;
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
   }

   return &ret->base;
}

} // extern "C"
