// ----
// ---- file   : plugin.h
// ---- legal  : Distributed under terms of the MIT license (https://opensource.org/licenses/MIT)
// ----          Copyright 2020-2023 by bsp
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
// ----
// ---- info   : A simple "C"-linkage voice plugin interface
// ----
// ---- created: 16May2020
// ---- changed: 17May2020, 18May2020, 19May2020, 20May2020, 24May2020, 31May2020, 06Jun2020
// ----          08Jun2020, 16Aug2021, 03Sep2023, 30Nov2023, 04Dec2023
// ----
// ----
// ----

#ifndef __ST_PLUGIN_INTERFACE_H__
#define __ST_PLUGIN_INTERFACE_H__

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef _MSC_VER
#ifdef ST_PLUGIN_IMPORT
#define ST_PLUGIN_APICALL __declspec(dllimport)
#else
#define ST_PLUGIN_APICALL __declspec(dllexport)
#endif // ST_PLUGIN_IMPORT
#define ST_PLUGIN_API
#else
#define ST_PLUGIN_APICALL extern
#define ST_PLUGIN_API
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif


// Forward declarations
struct st_plugin_info_s;
typedef struct st_plugin_info_s st_plugin_info_t;

struct st_plugin_shared_s;
typedef struct st_plugin_shared_s st_plugin_shared_t;

struct st_plugin_voice_s;
typedef struct st_plugin_voice_s st_plugin_voice_t;

typedef void *st_sample_handle_t;
typedef void *st_samplebank_handle_t;
typedef void *st_sampleplayer_handle_t;

typedef void *st_ui_handle_t;


// plugin api version
#define ST_PLUGIN_API_VERSION  (1u)

// flags: plugin implements a post-processing effect
#define ST_PLUGIN_FLAG_FX               (1u << 0)

// flags: plugin implements one or many oscillators that generate new initial sample data
#define ST_PLUGIN_FLAG_OSC              (1u << 1)

// flags: plugin generates true stereo output
//         (note) when unset, stereo output is actually mono (l=r) when input is mono
#define ST_PLUGIN_FLAG_TRUE_STEREO_OUT  (1u << 2)

// flags: plugin supports voicebus cross modulation
#define ST_PLUGIN_FLAG_XMOD             (1u << 3)

// Maximum number of layers / voice buses
#define ST_PLUGIN_MAX_LAYERS  (32u)

// Plugin categories
#define ST_PLUGIN_CAT_UNKNOWN     (1u <<  0)
#define ST_PLUGIN_CAT_RINGMOD     (1u <<  1)
#define ST_PLUGIN_CAT_WAVESHAPER  (1u <<  2)
#define ST_PLUGIN_CAT_FILTER      (1u <<  3)
#define ST_PLUGIN_CAT_EQ          (1u <<  4)
#define ST_PLUGIN_CAT_COMPRESSOR  (1u <<  5)
#define ST_PLUGIN_CAT_LIMITER     (1u <<  6)
#define ST_PLUGIN_CAT_ENHANCE     (1u <<  7)
#define ST_PLUGIN_CAT_DAC         (1u <<  8)
#define ST_PLUGIN_CAT_LOFI        (1u <<  9)
#define ST_PLUGIN_CAT_GATE        (1u << 10)
#define ST_PLUGIN_CAT_AMP         (1u << 11)
#define ST_PLUGIN_CAT_PAN         (1u << 12)
#define ST_PLUGIN_CAT_PITCH       (1u << 13)
#define ST_PLUGIN_CAT_CHORUS      (1u << 14)
#define ST_PLUGIN_CAT_FLANGER     (1u << 15)
#define ST_PLUGIN_CAT_DELAY       (1u << 16)
#define ST_PLUGIN_CAT_REVERB      (1u << 17)
#define ST_PLUGIN_CAT_TRANSIENT   (1u << 18)
#define ST_PLUGIN_CAT_CONVOLVER   (1u << 19)
#define ST_PLUGIN_CAT_MIXER       (1u << 20)
#define ST_PLUGIN_CAT_COMPARATOR  (1u << 21)
#define ST_PLUGIN_CAT_UTILITY     (1u << 22)


//
// Plugin descriptor
//  - a singleton per plugin library
//
struct st_plugin_info_s {

   // Plugin API version info
   //  - must be set to ST_PLUGIN_API_VERSION
   unsigned int api_version;

   // Unique identifier string
   //  - used internally to find plugins, e.g. when loading patches
   //  - e.g. "my awesome filter 16May2020"
   //  - id must _NOT_ change in future builds
   //  - not displayed in the UI
   //  - must NOT be NULL
   const char *id;

   // Author string
   //  - used to display information about the plugin in the UI
   //  - length should not exceed 24 chars
   //  - UTF-8 encoded string (please try to use ASCII, though)
   //  - e.g. "my company name"
   //  - must NOT be NULL
   const char *author;

   // Name string
   //  - used to display information about the plugin in the UI
   //  - length should not exceed 32 chars
   //  - UTF-8 encoded string (please try to use ASCII, though)
   //  - e.g. "my awesome filter"
   //  - must NOT be NULL
   const char *name;

   // Short name string
   //  - used to display information about the plugin in the UI
   //  - length should not exceed 8 chars
   //  - UTF-8 encoded string (please try to use ASCII, though)
   //  - e.g. "filter"
   //  - must NOT be NULL
   const char *short_name;

   // Flags
   //  - see ST_PLUGIN_FLAG_xxx
   //  - 0: plugin implements a post-processing effect
   unsigned int flags;

   // Category
   //  - see ST_PLUGIN_CAT_xxx
   unsigned int category;

   // Number of parameters
   //  - up to 8 (at the moment)
   //  - for static configuration
   unsigned int num_params;

   // Number of modulation destinations
   //  - up to 8 (at the moment)
   //  - can be targeted in the voice mod matrix
   unsigned int num_mods;

   // Create new (shared) plugin instance
   //  - fxn pointer must NOT be NULL
   //  - must return new plugin instance (dynamically allocated memory)
   //  - shared by all voices
   //  - stores common parameters
   st_plugin_shared_t *(ST_PLUGIN_API *shared_new)    (st_plugin_info_t *_info);

   // Delete (shared) plugin instance
   //  - fxn pointer must NOT be NULL
   //  - must free plugin instance memory
   //  - called after all voice instances have been deleted
   void                (ST_PLUGIN_API *shared_delete) (st_plugin_shared_t *_shared);

   // Create new (per-voice) plugin instance
   //  - fxn pointer must NOT be NULL
   //  - must return new plugin instance (dynamically allocated memory)
   //  - stores voice modulation parameters and state
   st_plugin_voice_t  *(ST_PLUGIN_API *voice_new)    (st_plugin_info_t *_info);

   // Delete (shared) plugin instance
   //  - fxn pointer must NOT be NULL
   //  - must free plugin instance memory
   //  - called after all voice instances have been deleted
   void                (ST_PLUGIN_API *voice_delete) (st_plugin_voice_t *_voice);

   // Set parent sample player (opaque handle)
   //  - 'polyphony' is the maximum number of voices
   //  - fxn pointer can be NULL if plugin is not interested in sample player info
   void (ST_PLUGIN_API *set_sampleplayer) (st_plugin_voice_t       *_voice,
                                           st_sampleplayer_handle_t _samplePlayer,
                                           unsigned int             _polyphony
                                           );

   // Set parent sample bank (zone) (opaque handle)
   //  - fxn pointer can be NULL if plugin is not interested in sample info
   void (ST_PLUGIN_API *set_samplebank) (st_plugin_voice_t     *_voice,
                                         st_samplebank_handle_t _sampleBank
                                         );

   // Set parent sample (opaque handle)
   //  - fxn pointer can be NULL if plugin is not interested in sample info
   void (ST_PLUGIN_API *set_sample) (st_plugin_voice_t  *_voice,
                                     st_sample_handle_t  _sample
                                     );

   // Set output sample rate
   //  - e.g. 44100
   //  - fxn pointer can be NULL if plugin is not interested in sample rate info
   void (ST_PLUGIN_API *set_sample_rate) (st_plugin_voice_t *_voice,
                                          float              _sampleRate
                                          );

   // Query parameter name
   //  - displayed in the UI
   //  - UTF-8 encoded string (please try to use ASCII, though)
   //  - length should not exceed 16 chars
   //  - do NOT allocate new memory here !
   //  - fxn pointer can be NULL if the plugin has no parameters
   //  - the first param should be "Dry / Wet" (not a requirement, though)
   const char *(ST_PLUGIN_API *get_param_name) (st_plugin_info_t *_info,
                                                unsigned int      _paramIdx
                                                );

   // Query parameter reset value
   //  - fxn pointer can be NULL (reset value will be 0.0f)
   //  - the returned value must be in the range 0..1
   //  - caller ensures that 'paramIdx' is in range 0..(num_params-1)
   float (ST_PLUGIN_API *get_param_reset) (st_plugin_info_t *_info,
                                           unsigned int      _paramIdx
                                           );

   // Query parameter value
   //  - fxn pointer can be NULL if the plugin has no parameters
   //  - the returned value must be in the range 0..1
   //  - caller ensures that 'paramIdx' is in range 0..(num_params-1)
   float (ST_PLUGIN_API *get_param_value) (st_plugin_shared_t *_shared,
                                           unsigned int        _paramIdx
                                           );

   // Query parameter value string
   //  - used to convert parameter value to human readable string (e.g. "440 Hz")
   //  - fxn pointer can be NULL if the plugin has no parameters
   //  - up to 'bufSize' chars may be written to 'buf'
   void (ST_PLUGIN_API *get_param_value_string) (st_plugin_shared_t  *_shared,
                                                 unsigned int         _paramIdx,
                                                 char                *_buf,
                                                 unsigned int         _bufSize
                                                 );

   // Set new parameter value
   //  - fxn pointer can be NULL if the plugin has no parameters
   //  - this is usually called when the parameter is edited in the UI
   //  - 'value' is a normalized float value in the range 0..1
   //  - parameters changes are protected by a mutex, i.e. they will NOT occur while the plugin is rendering
   //  - caller ensures that 'paramIdx' is in range 0..(num_params-1)
   void (ST_PLUGIN_API *set_param_value) (st_plugin_shared_t *_shared,
                                          unsigned int        _paramIdx,
                                          float               _value
                                          );

   // Query modulation target name
   //  - displayed in the UI
   //  - UTF-8 encoded string (please try to use ASCII, though)
   //  - length should not exceed 16 chars
   //  - do NOT allocate new memory here !
   //  - fxn pointer can be NULL if the plugin has no parameters
   const char *(ST_PLUGIN_API *get_mod_name) (st_plugin_info_t *_info,
                                              unsigned int      _modIdx
                                              );

   // Set new modulation value
   //  - fxn pointer can be NULL if the plugin has no parameters
   //  - this is called once per 'block' (1000Hz update rate), if the target is being modulated via the mod matrix
   //  - the plugin may interpolate between the previous and new modulation values
   //  - 'value' is a normalized float value, usually in the range -1..1 (relative to param base value)
   //     - please note that the effective mod value (param+mod) may be outside the 0..1 range
   //  - 'frameOffset' is the exact position in the next block at when the modulation value shall be reached
   //     - the value is relative to the block end, i.e. 0 means "end of block", 1 means "end of block - 1 frame"
   //     - as of now (20May2020), this is ignored by the Eureka host
   //  - modulation is always changed in the same thread context as the process*() callbacks
   //  - caller ensures that 'modIdx' is in range 0..(num_mods-1)
   void (ST_PLUGIN_API *set_mod_value) (st_plugin_voice_t *_voice,
                                        unsigned int       _modIdx,
                                        float              _value,
                                        unsigned int       _frameOff
                                        );

   // Begin / prepare first or next block
   //  - fxn pointer can be NULL if the plugin does not implement parameter interpolation or
   //     does not require any other precalculations
   //  - this is called once per 'block' (1000Hz update rate) after zero or more modulation values have been updated
   //  - after a note on, this is called twice to set up both the initial state and the state at the end of the first block
   //     - 'numFrames' is set to 0 during the first call, and >0 during the next calls (~mix_rate / STPLUGIN_MOD_RATE)
   //  - the plugin can use this function to update the voice state for the next sample block
   //  - 'numFrames' is size of the next block (number of sample frames)
   //  - 'freqHz' is the (modulated) target note frequency (at the end of the next block)
   //  - 'note' is the (modulated) normalized target note (including pitchbend / freq modulation) (0..127)
   //  - 'vol' is the target volume (0..~1) (at the end of the next block)
   //     (note) this is just for info purposes, the volume multiplication is handled outside the plugin
   //  - 'pan' is the target panning (-1..1) (at the end of the next block)
   //     (note) this is just for info purposes, the volume multiplication is handled outside the plugin
   void (ST_PLUGIN_API *prepare_block) (st_plugin_voice_t *_voice,
                                        unsigned int       _numFrames,
                                        float              _freqHz,
                                        float              _note,
                                        float              _vol,
                                        float              _pan
                                        );

   // Handle note-on
   //  - 'bGlide' is true (1) when voice portamento / glissando is active
   //     - plugin may skip state reset when glide is active
   //     - 'bGlide' is maybe (-1) when 'smp' reset is enabled (=> glide note but restart envs)
   //  - 'note' is the MIDI note number (0..127)
   //  - 'vel' is the normalized velocity (0..1)
   //  - (note) additional info can be read from the voice base struct (layer_idx etc)
   //  - fxn pointer can be NULL if the plugin is not interested in note ons
   //     - NOTE: if the plugin has mods it's usually a good idea to reset them during note_on() !
   //  - called in the same thread context as the process*() callbacks
   void (ST_PLUGIN_API *note_on) (st_plugin_voice_t  *_voice,
                                  int                 _bGlide,
                                  unsigned char       _note,
                                  float               _vel
                                  );

   // Handle note-off
   //  - note is the MIDI note number (0..127)
   //     (the same note that was passed to note_on())
   //  - vel is the normalized release velocity (0..1)
   //  - fxn pointer can be NULL if the plugin is not interested in note offs
   //  - called in the same thread context as the process*() callbacks
   void (ST_PLUGIN_API *note_off) (st_plugin_voice_t  *_voice,
                                   unsigned char       _note,
                                   float               _vel
                                   );

   // Process interleaved stereo data ('block')
   //  - used for FX type plugins
   //  - replaces samples in buffer 'samplesOut' with new samples
   //  - this is usually called once per 'block' (1000Hz update rate)
   //     - block size can be smaller at outer processing block boundaries, though
   //  - when 'samplesIn'=='samplesOut', process in place
   //  - 'bMonoIn' is a hint that is set when the (stereo) input buffer is actually mono, i.e. l/r samples are equal
   //     - the plugin may calculate a mono effect and simply copy the left channel output to the right channel,
   //        unless it is a true mono->stereo effect
   //  - oscillator-type plugins (ST_PLUGIN_FLAG_OSC) may ignore 'samplesIn' and 'bMonoIn'
   //  - 'numFrames' is the number of sample frames to process (=> write (2 * numFrames) samples)
   //  - do NOT use multithreading, this is already handled by the host
   //  - fxn pointer can be NULL if the plugin's FX flag is not set
   //  - freqStartHz is the note frequency at the beginning of the block
   //  - freqStepHz is the note frequency per-sample-frame increment
   void (ST_PLUGIN_API *process_replace) (st_plugin_voice_t  *_voice,
                                          int                 _bMonoIn,
                                          const float        *_samplesIn,
                                          float              *_samplesOut,
                                          unsigned int        _numFrames
                                          );

   // Exit plugin.
   //  - called after all voice and shared instances have been deleted
   //  - called immediately before plugin library is closed
   //  - callee must free info struct
   void (ST_PLUGIN_API *plugin_exit) (st_plugin_info_t *_info);


   // Create host-specific extended plugin instance (user interface)
   //  - fxn pointer can be NULL
   //  - 'hostName' is a unique host name identifier
   //  - 'hostVersion' is the host version
   //  - 'hostInfo' is a host-specific struct/class pointer
   //  - called from UI thread
   st_ui_handle_t (ST_PLUGIN_API *ui_new) (st_plugin_shared_t *_shared,
                                           const char         *_hostName,
                                           unsigned int        _hostVersion,
                                           void               *_hostInfo
                                           );

   // Delete host-specific extended plugin instance (user interface)
   //  - called from UI thread
   //  - fxn pointer can be NULL
   void (ST_PLUGIN_API *ui_delete) (st_ui_handle_t _uiInstanceHandle);

   // Query dynamic parameter name
   //  - for plugins that can dynamically map internal parameters (per patch)
   //  - can also be used to map (static or dynamic) parameter (preset) values to names
   //  - fxn pointer can be NULL if the plugin has no, or just static parameters
   //  - displayed in the UI
   //  - UTF-8 encoded string (please try to use ASCII, though). Terminated by 0 ('ASCIIZ').
   //  - length should not exceed 16 chars
   //  - 'retBuf' points to a char buffer than can hold up to 'retBufSize' characters
   //  - the function returns the number of characters written to 'retBuf' (last char is always 0)
   //  - host implementations should fall back to get_param_name() when the fxn ptr is NULL
   unsigned int (ST_PLUGIN_API *query_dynamic_param_name) (st_plugin_shared_t *_shared,
                                                           const unsigned int  _paramIdx,
                                                           char               *_retBuf,
                                                           const unsigned int  _retBufSize
                                                           );

   // Query dynamic modulation target name
   //  - for plugins that can dynamically map internal parameters (per patch)
   //  - fxn pointer can be NULL if the plugin has no, or just static modulation targets
   //  - displayed in the UI
   //  - UTF-8 encoded string (please try to use ASCII, though). Terminated by 0 ('ASCIIZ').
   //  - length should not exceed 16 chars
   //  - 'retBuf' points to a char buffer than can hold up to 'retBufSize' characters
   //  - the function returns the number of characters written to 'retBuf' (last char is always 0)
   //  - host implementations should fall back to get_mod_name() when the fxn ptr is NULL
   unsigned int (ST_PLUGIN_API *query_dynamic_mod_name) (st_plugin_shared_t *_shared,
                                                         const unsigned int  _modIdx,
                                                         char               *_retBuf,
                                                         const unsigned int  _retBufSize
                                                         );

   // Query parameter preset values
   //  - fxn pointer can be NULL (no presets)
   //  - caller ensures that 'paramIdx' is in range 0..(num_params-1)
   //  - 'retValues' points to an array that can hold up to 'retValuesSize' 32bit float values
   //  - the function returns the number of preset values written to 'retValues'
   //  - when 'retValues' is NULL, the function returns the total number of values it wants to write
   unsigned int (ST_PLUGIN_API *query_dynamic_param_preset_values) (st_plugin_shared_t *_shared,
                                                                    const unsigned int  _paramIdx,
                                                                    float              *_retValues,
                                                                    const unsigned int  _retValuesSize
                                                                    );

   // Query parameter preset name
   //  - fxn pointer can be NULL (no presets)
   //  - caller ensures that 'paramIdx' is in range 0..(num_params-1)
   //  - 'presetIdx' is in range 0..(query_dynamic_param_preset_values() return value - 1)
   //  - 'retBuf' points to a char buffer than can hold up to 'retBufSize' characters
   //  - the function returns the number of characters written to 'retBuf' (last char is always 0)
   //  - when 'retBuf' is NULL, the function returns the total number of characters required (including ASCIIZ)
   unsigned int (ST_PLUGIN_API *query_dynamic_param_preset_name) (st_plugin_shared_t *_shared,
                                                                  const unsigned int  _paramIdx,
                                                                  const unsigned int  _presetIdx,
                                                                  char               *_retBuf,
                                                                  const unsigned int  _retBufSize
                                                                  );

   // Query parameter group name
   //  - fxn pointer can be NULL (no parameter groups)
   //  - parameter group indices are in the range 0..(num_groups-1)
   //  - caller finishes group names queries when this function returns NULL
   const char * (ST_PLUGIN_API *get_param_group_name) (st_plugin_info_t   *_info,
                                                       const unsigned int  _paramGroupIdx
                                                       );

   // Query parameter group assignment
   //  - fxn pointer can be NULL (no parameter groups)
   //  - when the returned parameter group index (e.g. ~0u) can not be mapped to a parameter group name,
   //     the parameter is not assigned to any group
   unsigned int (ST_PLUGIN_API *get_param_group_idx) (st_plugin_info_t   *_info,
                                                      const unsigned int  _paramIdx
                                                      );

   void *_future[64 - 37];
};


//
// Plugin (shared) instance (base) structure
//
//  - must be the first field in actual/derived plugin structs
//  - once instance per sample plugin slot
//  - stores shared voice patch parameters
//
struct st_plugin_shared_s {

   // Reference to plugin info struct
   //  - must NOT be NULL
   st_plugin_info_t *info;

   void *_future[16 - 1];

};


//
// Plugin (per-voice) instance (base) structure
//
//  - must be the first field in actual/derived plugin structs
//  - there are usually as many instances as there are sample players, multiplied by their polyphony
//  - the sampleplayer / samplebank / sample pointers can be used to find neighbouring voices, e.g.
//     for cross modulation purposes
//  - plugin implementations can store their voice state here
//
struct st_plugin_voice_s {

   // Reference to plugin info struct
   //  - must NOT be NULL
   st_plugin_info_t *info;

   // Reference to shared plugin struct (parameters)
   //  - set by host when voice is started
   st_plugin_shared_t *shared;

   // updated immediately before prepare_block()
   //  - indexed by voice bus index (0..ST_PLUGIN_MAX_LAYERS-1)
   //  - each array element points to an array of interleaved stereo float samples
   //  - used for cross-zone modulation (e.g. ring modulate layers 1+2)
   float        **voice_bus_buffers;
   unsigned int   voice_bus_read_offset;  // #samples (_not_ frames)

   // valid at note_on()
   //  - stores the idx in a group of simultaneously triggered (layered) zones/layers (0..ST_PLUGIN_MAX_LAYERS-1)
   //  - can be used to determine the current or previous voice_bus index (when the layer output bus is set to 'cur')
   unsigned int layer_idx;

   // valid at note_on()
   //  - in the range 0..(polyphony-1) (see set_sampleplayer())
   unsigned int voice_idx;

   // valid at note_on()
   //  - in the range 0..(num_currently_pressed_keys-1)
   unsigned int active_note_idx;

   // valid at note_on()
   //  - 'noteHz' is the unmodulated note frequency in Hz (according to current frequency table)
   //  - (note) the actual, modulated note frequency is passed to the prepare_block() function
   float note_hz;

   void *_future[16 - 8];

};


// Initialize and query plugin information
//  - called immediately after opening plugin library
//  - callee must allocate info struct
//  - callee must return NULL when there are no more plugins to initialize in this plugin library
#define ST_PLUGIN_MAX  (1024u)
ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init (unsigned int _pluginIdx);
typedef st_plugin_info_t *(*ST_PLUGIN_API st_plugin_init_fxn_t) (unsigned int _pluginIdx);

//
// Utility macros
//
#define ST_PLUGIN_SHARED_CAST(a) a *shared = (a*)_shared
#define ST_PLUGIN_INFO_CAST(a) a *info = (a*)_info
#define ST_PLUGIN_VOICE_CAST(a) a *voice = (a*)_voice
#define ST_PLUGIN_VOICE_INFO_CAST(a) a *info = (a*)_voice->shared->info
#define ST_PLUGIN_VOICE_SHARED_CAST(a) a *shared = (a*)_voice->shared

// Clip value to min/max range
#define Dstplugin_clamp(a,m,x) ( ((a) < (m)) ? (m) : ((a) > (x)) ? (x) : (a) )

// Remap normalized mod/param value 'a' to range min='m', max='x'
#define Dstplugin_val_to_range(a,m,x)  ( ((a) * ((x) - (m))) + (m) )

// Fix denormalized float value
//  (note) denormals considerably degrade floating point performance on Intel CPUs
//  (note) use this macro to process the plugin's output values to get rid of them
#define Dstplugin_fix_denorm_32(a) ( ((a)+10.0f) - 10.0f )

#define Dstplugin_min(a,b) (((a)>(b))?(b):(a))
#define Dstplugin_max(a,b) (((a)>(b))?(a):(b))
#define Dstplugin_sign(x) (((x)==0)?0:(((x)>0)?1:-1))
#define Dstplugin_abs(x) (((x)>0)?(x):-(x))
#define Dstplugin_align(x,a) (((x)+(a)-1)&~((a)-1))

// Math constants
#define ST_PLUGIN_PI    3.1415926535897932384626433832795029
#define ST_PLUGIN_2PI   (ST_PLUGIN_PI * 2.0)
#define ST_PLUGIN_PI2   (ST_PLUGIN_PI / 2.0)
#define ST_PLUGIN_PI4   (ST_PLUGIN_PI / 4.0)
#define ST_PLUGIN_LN2   0.6931471805599453094172321214581766
#define ST_PLUGIN_LN10  2.3025850929940456840179914546843642
#define ST_PLUGIN_E     2.7182818284590452353602874713526625

#define ST_PLUGIN_PI_F    ((float)ST_PLUGIN_PI)
#define ST_PLUGIN_2PI_F   ((float)ST_PLUGIN_2PI)
#define ST_PLUGIN_PI2_F   ((float)ST_PLUGIN_PI2)
#define ST_PLUGIN_PI4_F   ((float)ST_PLUGIN_PI4)
#define ST_PLUGIN_LN2_F   ((float)ST_PLUGIN_LN2)
#define ST_PLUGIN_LN10_F  ((float)ST_PLUGIN_LN10)
#define ST_PLUGIN_E_F     ((float)ST_PLUGIN_E)

// Calc voicebus idx from param/mod
#define Dstplugin_voicebus(i,p) \
   if((p) < 0.0f) (p) = 0.0f;   \
   (p) *= 100.0f;                          \
   i = (unsigned int)((p) + 0.5f);   \
   if(i >= ST_PLUGIN_MAX_LAYERS) \
      i = ST_PLUGIN_MAX_LAYERS - 1u; \
   if(0u == i) \
   { \
      if(0u != voice->base.layer_idx) \
         i = voice->base.layer_idx - 1u;  /* previous layer output */ \
      /* else: illegal ref to previous layer in first layer (=> use first layer) */ \
   } \
   else \
      i--

typedef union stplugin_fi_u {
   float        f;
   unsigned int ui;
   signed   int si;
} stplugin_fi_t;

typedef union stplugin_us8_u {
   char s;
   unsigned char u;
} stplugin_us8_t;

typedef union stplugin_us16_u {
   short s;
   unsigned short u;
} stplugin_us16_t;


#ifdef __cplusplus
}
#endif

#endif // __ST_PLUGIN_H__
