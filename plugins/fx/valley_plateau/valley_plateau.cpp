
// authors: Dale Johnson (original code)
//          bsp (STFX plugin port)
// license: BSD-3-Clause (see docs/LICENSE)
//
// (note) "Plateau is based on the venerable Dattorro (1997) reverb algorithm.
//          Reference: Dattorro, J. (1997). Effect design part 1: Reverberator and other filters, J. Audio
//          Eng. Soc, 45(9), 660-684."

// 19Sep2024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../plugin.h"

#ifndef M_PI
#define M_PI ST_PLUGIN_PI
#endif

#include "Plateau/Dattorro.hpp"


#define PARAM_DRY                0
#define PARAM_WET                1
#define PARAM_PRE_DELAY          2
#define PARAM_SIZE               3
#define PARAM_DIFFUSION          4
#define PARAM_DIFFUSION_AMT      5
#define PARAM_DECAY              6
#define PARAM_REVERB_HIGH_DAMP   7 
#define PARAM_REVERB_LOW_DAMP    8
#define PARAM_INPUT_HIGH_DAMP    9
#define PARAM_INPUT_LOW_DAMP    10
#define PARAM_MOD_SPEED         11
#define PARAM_MOD_SHAPE         12
#define PARAM_MOD_DEPTH         13
#define PARAM_FREEZE_SW         14  // >=0.5: freeze
#define PARAM_CLEAR_SW          15  // >=0.5: clear
#define PARAM_TUNED_SW          16  // >=0.5: enable tuned mode (size)
#define NUM_PARAMS              17
static const char *loc_param_names[NUM_PARAMS] = {
   "Dry",           //  0
   "Wet",           //  1
   "PreDelay",      //  2
   "Size",          //  3
   "Diffusion",     //  4
   "Diffusion Amt", //  5
   "Decay",         //  6
   "Rev Hi Damp",   //  7
   "Rev Lo Damp",   //  8
   "In Hi Damp",    //  9
   "In Lo Damp",    // 10
   "Mod Speed",     // 11
   "Mod Shape",     // 12
   "Mod Depth",     // 13
   "Freeze Sw",     // 14
   "Clear Sw",      // 15
   "Tuned Sw",      // 16
};
static float loc_param_resets[NUM_PARAMS] = {
   1.0f,  // PARAM_DRY
   0.25f, // PARAM_WET
   0.0f,  // PARAM_PRE_DELAY
   0.5f,  // PARAM_SIZE
   0.5f,  // PARAM_DIFFUSION
   1.0f,  // PARAM_DIFFUSION_AMT
   0.5f,  // PARAM_DECAY
   0.0f,  // PARAM_REVERB_HIGH_DAMP
   1.0f,  // PARAM_REVERB_LOW_DAMP
   0.0f,  // PARAM_INPUT_HIGH_DAMP
   1.0f,  // PARAM_INPUT_LOW_DAMP
   0.0f,  // PARAM_MOD_SPEED
   0.5f,  // PARAM_MOD_SHAPE  0.5=tri
   0.1f,  // PARAM_MOD_DEPTH
   0.0f,  // PARAM_FREEZE_SW
   0.0f,  // PARAM_CLEAR_SW
   0.0f,  // PARAM_TUNED_SW
};

#define MOD_DRY                0
#define MOD_WET                1
#define MOD_PRE_DELAY          2
#define MOD_SIZE               3
#define MOD_DIFFUSION          4
#define MOD_DIFFUSION_AMT      5
#define MOD_DECAY              6
#define MOD_REVERB_HIGH_DAMP   7
#define MOD_REVERB_LOW_DAMP    8
#define MOD_INPUT_HIGH_DAMP    9
#define MOD_INPUT_LOW_DAMP    10
#define MOD_MOD_SPEED         11
#define MOD_MOD_SHAPE         12
#define MOD_MOD_DEPTH         13
#define MOD_FREEZE_SW         14
#define MOD_CLEAR_SW          15
#define MOD_TUNED_SW          16
#define NUM_MODS              17
static const char *loc_mod_names[NUM_MODS] = {
   "Dry",           //  0
   "Wet",           //  1
   "PreDelay",      //  2
   "Size",          //  3
   "Diffusion",     //  4
   "Diffusion Amt", //  5
   "Decay",         //  6
   "Rev Hi Damp",   //  7
   "Rev Lo Damp",   //  8
   "In Hi Damp",    //  9
   "In Lo Damp",    // 10
   "Mod Speed",     // 11
   "Mod Shape",     // 12
   "Mod Depth",     // 13
   "Freeze Sw",     // 14
   "Clear Sw",      // 15
   "Tuned Sw",      // 16
};

typedef struct valley_plateau_info_s {
   st_plugin_info_t base;
} valley_plateau_info_t;

typedef struct valley_plateau_shared_s {
   st_plugin_shared_t base;
   float params[NUM_PARAMS];
} valley_plateau_shared_t;

typedef struct valley_plateau_voice_s {
   st_plugin_voice_t base;
   float    mods[NUM_MODS];
   float    mod_dry_cur;
   float    mod_dry_inc;
   float    mod_wet_cur;
   float    mod_wet_inc;
   bool     frozen;
   bool     cleared;
   Dattorro reverb;
} valley_plateau_voice_t;


static void ST_PLUGIN_API loc_set_sample_rate(st_plugin_voice_t *_voice,
                                              float              _sampleRate
                                              ) {
   ST_PLUGIN_VOICE_CAST(valley_plateau_voice_t);
   voice->reverb.setSampleRate(_sampleRate);
}

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
   ST_PLUGIN_SHARED_CAST(valley_plateau_shared_t);
   return shared->params[_paramIdx];
}

static void ST_PLUGIN_API loc_set_param_value(st_plugin_shared_t *_shared,
                                              unsigned int        _paramIdx,
                                              float               _value
                                              ) {
   ST_PLUGIN_SHARED_CAST(valley_plateau_shared_t);
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
   ST_PLUGIN_VOICE_CAST(valley_plateau_voice_t);
   (void)_bGlide;
   (void)_note;
   (void)_vel;
   if(!_bGlide)
   {
      memset((void*)voice->mods, 0, sizeof(voice->mods));
      voice->reverb.clear();
   }
}

static void ST_PLUGIN_API loc_set_mod_value(st_plugin_voice_t *_voice,
                                            unsigned int       _modIdx,
                                            float              _value,
                                            unsigned           _frameOffset
                                            ) {
   ST_PLUGIN_VOICE_CAST(valley_plateau_voice_t);
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
   ST_PLUGIN_VOICE_CAST(valley_plateau_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(valley_plateau_shared_t);
   (void)_freqHz;
   (void)_note;
   (void)_vol;
   (void)_pan;

   // CV scaling
   // const float preDelayNormSens = 0.1f;
   // const float preDelayLowSens = 0.05f;
   const float sizeMin = 0.0025f;
   const float sizeMax = 4.0f;
   const float decayMin = 0.1f;
   const float decayMax = 0.9999f;
   const float modDepthMax = 16.f;
   const float modShapeMin = 0.001f;
   const float modShapeMax = 0.999f;

   float modDry = shared->params[PARAM_DRY] + voice->mods[MOD_DRY];
   modDry = Dstplugin_clamp(modDry, 0.0f, 1.0f);

   float modWet = shared->params[PARAM_WET] + voice->mods[MOD_WET];
   modWet = Dstplugin_clamp(modWet, 0.0f, 1.0f);

   float modFreeze = shared->params[PARAM_FREEZE_SW] + voice->mods[MOD_FREEZE_SW];
   bool freeze = (modFreeze >= 0.5f);
   if(freeze && !voice->frozen)
   {
      voice->frozen = true;
      voice->reverb.freeze();
   }
   else if(!freeze && voice->frozen)
   {
      voice->frozen = false;
      voice->reverb.unFreeze();
   }

   float modClear = shared->params[PARAM_CLEAR_SW] + voice->mods[MOD_CLEAR_SW];
   bool clear = (modClear >= 0.5f);
   if(clear && !voice->cleared)
   {
      voice->cleared = true;
      voice->reverb.clear();
   }
   else if(!clear && voice->cleared)
   {
      voice->cleared = false;
   }

   float modTuned = shared->params[PARAM_TUNED_SW] + voice->mods[MOD_TUNED_SW];
   bool tuned = (modTuned >= 0.5f);

   float modPreDelay = shared->params[PARAM_PRE_DELAY] + voice->mods[MOD_PRE_DELAY];
   modPreDelay = Dstplugin_clamp(modPreDelay, 0.0f, 1.0f);
   float preDelay;
   preDelay = modPreDelay;
   voice->reverb.setPreDelay(Dstplugin_clamp(preDelay, 0.f, 1.f));

   float modSize = shared->params[PARAM_SIZE] + voice->mods[MOD_SIZE];
   modSize = Dstplugin_clamp(modSize, 0.0f, 1.0f);
   float size = modSize;
   if(tuned)
   {
      size = sizeMin * powf(2.f, size * 5.f);
      size = Dstplugin_clamp(size, sizeMin, 2.5f);
   }
   else
   {
      size *= size;
      size = Dstplugin_scale(size, 0.01f, sizeMax);
      size = Dstplugin_clamp(size, 0.01f, sizeMax);
   }
   voice->reverb.setTimeScale(size);

   float modDiffusion = shared->params[PARAM_DIFFUSION] + voice->mods[MOD_DIFFUSION];
   modDiffusion = Dstplugin_clamp(modDiffusion, 0.0f, 1.0f);
   float diffusion = modDiffusion;
   voice->reverb.plateDiffusion1 = diffusion * 0.7f;
   voice->reverb.plateDiffusion2 = diffusion * 0.5f;

   // (note) Plateau originally used a switch (0/1) for this
   float modDiffuse = shared->params[PARAM_DIFFUSION_AMT] + voice->mods[MOD_DIFFUSION_AMT];
   // // bool diffuseInput = (modDiffuse >= 0.5f);
   voice->reverb.diffuseInput = (double)modDiffuse;

   float modDecay = shared->params[PARAM_DECAY] + voice->mods[MOD_DECAY];
   modDecay = Dstplugin_clamp(modDecay, 0.0f, 1.0f);
   float decay = Dstplugin_scale(modDecay, decayMin, decayMax);
   decay = 1.f - decay;
   decay = 1.f - decay * decay;
   voice->reverb.decay = decay;

   float modReverbHighDamp = shared->params[PARAM_REVERB_HIGH_DAMP] + voice->mods[MOD_REVERB_HIGH_DAMP];
   modReverbHighDamp = Dstplugin_clamp(modReverbHighDamp, 0.0f, 1.0f);
   float reverbDampHigh = 10.0f - modReverbHighDamp * 10.0f;
   voice->reverb.reverbHighCut = 440.f * powf(2.f, reverbDampHigh - 5.f);

   float modReverbLowDamp = shared->params[PARAM_REVERB_LOW_DAMP] + voice->mods[MOD_REVERB_LOW_DAMP];
   modReverbLowDamp = Dstplugin_clamp(modReverbLowDamp, 0.0f, 1.0f);
   float reverbDampLow = 10.0f - modReverbLowDamp * 10.0f;
   voice->reverb.reverbLowCut = 440.f * powf(2.f, reverbDampLow - 5.f);

   float modInputLowDamp = shared->params[PARAM_INPUT_LOW_DAMP] + voice->mods[MOD_INPUT_LOW_DAMP];
   modInputLowDamp = Dstplugin_clamp(modInputLowDamp, 0.0f, 1.0f);
   float inputDampLow = 10.0f - modInputLowDamp * 10.0f;
   voice->reverb.inputLowCut = 440.f * powf(2.f, inputDampLow - 5.f);

   float modInputHighDamp = shared->params[PARAM_INPUT_HIGH_DAMP] + voice->mods[MOD_INPUT_HIGH_DAMP];
   modInputHighDamp = Dstplugin_clamp(modInputHighDamp, 0.0f, 1.0f);
   float inputDampHigh = 10.0f - modInputHighDamp * 10.0f;
   voice->reverb.inputHighCut = 440.f * powf(2.f, inputDampHigh - 5.f);

   float modModSpeed = shared->params[PARAM_MOD_SPEED] + voice->mods[MOD_MOD_SPEED];
   modModSpeed = Dstplugin_clamp(modModSpeed, 0.0f, 1.0f);
   float modSpeed = modModSpeed * modModSpeed;
   modSpeed = modSpeed * 99.f + 1.f;
   voice->reverb.modSpeed = modSpeed;

   float modModShape = shared->params[PARAM_MOD_SHAPE] + voice->mods[MOD_MOD_SHAPE];
   modModShape = Dstplugin_clamp(modModShape, 0.0f, 1.0f);
   float modShape = modModShape * modModShape;
   modShape = Dstplugin_scale(modShape, modShapeMin, modShapeMax);
   voice->reverb.setModShape(modShape);

   float modModDepth = shared->params[PARAM_MOD_DEPTH] + voice->mods[MOD_MOD_DEPTH];
   modModDepth = Dstplugin_clamp(modModDepth, 0.0f, 1.0f);
   float modDepth = modModDepth * modDepthMax;
   voice->reverb.modDepth = modDepth;

   if(_numFrames > 0u)
   {
      // lerp
      float recBlockSize = (1.0f / _numFrames);
      voice->mod_dry_inc   = (modDry - voice->mod_dry_cur) * recBlockSize;
      voice->mod_wet_inc   = (modWet - voice->mod_wet_cur) * recBlockSize;
   }
   else
   {
      // initial params/modulation (first block, not rendered)
      voice->mod_dry_inc   = modDry;
      voice->mod_dry_cur   = 0.0f;
      voice->mod_wet_inc   = modWet;
      voice->mod_wet_cur   = 0.0f;
   }

}

static void ST_PLUGIN_API loc_process_replace(st_plugin_voice_t  *_voice,
                                              int                 _bMonoIn,
                                              const float        *_samplesIn,
                                              float              *_samplesOut,
                                              unsigned int        _numFrames
                                              ) {
   ST_PLUGIN_VOICE_CAST(valley_plateau_voice_t);
   ST_PLUGIN_VOICE_SHARED_CAST(valley_plateau_shared_t);

   unsigned int k = 0u;

   // Stereo input, stereo output
   for(unsigned int i = 0u; i < _numFrames; i++)
   {
      float l = _samplesIn[k];
      float r = _samplesIn[k + 1u];

      voice->reverb.process(l, r);

      l *= voice->mod_dry_cur;
      r *= voice->mod_dry_cur;

      l += voice->reverb.leftOut  * voice->mod_wet_cur;
      r += voice->reverb.rightOut * voice->mod_wet_cur;

      _samplesOut[k]      = l;
      _samplesOut[k + 1u] = r;

      // Next frame
      k += 2u;
      voice->mod_dry_cur += voice->mod_dry_inc;
      voice->mod_wet_cur += voice->mod_wet_inc;
   }

}

static st_plugin_shared_t *ST_PLUGIN_API loc_shared_new(st_plugin_info_t *_info) {
   valley_plateau_shared_t *ret = (valley_plateau_shared_t *)malloc(sizeof(valley_plateau_shared_t));
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
   (void)_voiceIdx;
   valley_plateau_voice_t *ret = (valley_plateau_voice_t *)malloc(sizeof(valley_plateau_voice_t));
   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));
      ret->base.info = _info;
      new(&ret->reverb)Dattorro();
   }
   return &ret->base;
}

static void ST_PLUGIN_API loc_voice_delete(st_plugin_voice_t *_voice) {
   ST_PLUGIN_VOICE_CAST(valley_plateau_voice_t);
   voice->reverb.~Dattorro();
   free((void*)_voice);
}

static void ST_PLUGIN_API loc_plugin_exit(st_plugin_info_t *_info) {
   free((void*)_info);
}

extern "C" {
st_plugin_info_t *valley_plateau_init(void) {
   valley_plateau_info_t *ret = NULL;

   ret = (valley_plateau_info_t *)malloc(sizeof(valley_plateau_info_t));

   if(NULL != ret)
   {
      memset((void*)ret, 0, sizeof(*ret));

      ret->base.api_version = ST_PLUGIN_API_VERSION;
      ret->base.id          = "valley plateau";  // unique id. don't change this in future builds.
      ret->base.author      = "Valley & bsp";
      ret->base.name        = "Valley Plateau";
      ret->base.short_name  = "Plateau";
      ret->base.flags       = ST_PLUGIN_FLAG_FX | ST_PLUGIN_FLAG_TRUE_STEREO_OUT;
      ret->base.category    = ST_PLUGIN_CAT_REVERB;
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

ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init(unsigned int _pluginIdx) {
   if(0u == _pluginIdx)
   {
      return valley_plateau_init();
   }
   return NULL;
}
} // extern "C"
