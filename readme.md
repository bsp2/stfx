# stfx

a lightweight, C-linkage audio plugin API that supports per-voice effects.



## Design considerations

- clutter-free, stable binary interface
- not tied to a specific plugin host
- platform agnostic (tested on Linux and Windows)
- written in "C" (=> plugins can be written in virtually any natively compiled language)
- single `.h` file, see [plugin.h](https://github.com/bsp2/stfx/blob/master/plugin.h)
- liberal open source license that allows the interface to also be used in closed source applications (MIT)


## A brief outline of the plugin interface

There are three fundamental data structures:

### st_plugin_info_t:

- plugin descriptor (see `struct st_plugin_info_s`)
- must be the first field in a _derived_ myplugin_info struct
- `myplugin_info_t *info = st_plugin_init(pluginIdx)`
    - `st_plugin_init` is the main (and only) DLL/SO plugin entry point
- a single DLL may contain many plugins: simply call `st_plugin_init(pluginIdx++)` until it returns NULL
- `info->plugin_exit(info)` frees the plugin descriptor

### st_plugin_shared_t:

- plugin instance that is common to all voices (see `struct st_plugin_shared_s`)
- must be the first field in a _derived_ myplugin_shared struct
- up to 8 normalized (0..1) float parameters (see set_param_value())
- `myplugin_shared_t *shared = info->shared_new(info)`
- `info->shared_delete(shared)` frees the shared plugin instance
- shared instance must be freed before freeing the plugin descriptor (info)

### st_plugin_voice_t

- per voice instance (see `struct st_plugin_voice_s`)
- must be the first field in a _derived_ myplugin_voice struct
- up to 8 modulation slots (usually in the range -1..1)
- (optional) support for up to 32 voice buses, e.g. for cross-voice modulation
- `myplugin_voice_t *voice = info->voice_new(info)`
- `info->voice_delete(voice)` frees the voice instance
- voices must be freed before freeing the shared instance


## Usage

``` c
/* open DLL/SO and query st_plugin_init() function address (platform-specific) */
#ifdef _MSC_VER
// Windows
HINSTANCE dllHandle = LoadLibrary("myplugin.dll"/*pathName*/);
FARPROC fxnHandle = GetProcAddress(dllHandle, "st_plugin_init");
#else
// Linux / MacOS
void *dllHandle = dlopen("myplugin.so"/*pathName*/, RTLD_NOW/*flags*/)
void *fxnHandle = dlsym(dllHandle, "st_plugin_init");
#endif
st_plugin_init_fxn_t initFxn = (st_plugin_init_fxn_t)fxnHandle;

/* get plugin descriptor for first sub-plugin */
st_plugin_info_t *info = initFxn(0u/*pluginIdx*/);

/* create 'shared' plugin instance (common to all voices) */
st_plugin_shared_t *shared = info->shared_new(info);

/* create voice plugin instance */
st_plugin_voice_t *voice = info->voice_new(info);

/* set sample rate */
info->set_sample_rate(voice, 44100.0f);

/* note on (e.g. reset modulation when not gliding) */
info->note_on(voice,
              0/*b_glide=false*/,
              (unsigned char)midiNote,
              velocity/*0..1*/
              );
              
/* set a parameter (host must check that param exists) */
if(info->num_params > 0u)
  info->set_param_value(shared, 0u/*paramIdx*/, 0.5f/*value*/);
  
/* modulate a parameter (host must check that modulation slot exists) */
if(info->num_mods > 0u)
  info->set_mod_value(voice, 0u/*modIdx*/, -0.25f/*value*/, 0u/*frameOff*/);

/* prepare first audio chunk after note on (set up initial parameter/modulation interpolation) */
info->prepare_block(voice,
                    0u/*numFrames. 0u=first chunk*/,
                    freqHz/*0..n Hz*/,
                    freqNote/*0..127 (MIDI note with fractional part)*/,
                    vol/*0..1*/,
                    pan/*-1..1*/
                    );
                                       
/* prepare next audio chunk (1..n frames) */
/*  (e.g. update per-sample-frame parameter/modulation interpolation) */
info->prepare_block(voice,
                    1u/*numFrames*/,
                    freqHz,
                    freqNote,
                    vol,
                    pan
                    );

/* render interleaved stereo buffer (single stereo sample frame) */
ioBuf[0] = ioBuf[1] = 0.0f;
voice->voice_bus_read_offset = 0u;
voice->voice_bus_buffers = NULL;
voice->info->process_replace(voice,
                             0/*bMonoIn=false*/,
                             ioBuf/*samplesIn*/,
                             ioBuf/*samplesOut*/,
                             1u/*numFrames*/
                             );
                             
/* delete voice instance */
info->voice_delete(voice);

/* delete shared instance */
info->shared_delete(shared);

/* delete plugin descriptor */
info->plugin_exit(info);

/* close plugin library (platform-specific) */
#ifdef _MSC_VER
// Windows
FreeLibrary(dllHandle);
#else
// Linux / MacOS
dlclose(dllHandle);
#endif

```



## Example plugin
see [fx_example](https://github.com/bsp2/stfx/blob/master/plugins/fx/fx_example/fx_example.c)


## Status

The interface can be considered stable.
There are a few reserved bytes in each struct for future extensions. These should be set to 0.

The following applications can currently load `stfx` plugins:

- `Eureka` software sampler / synth, see [miditracker.org](http://miditracker.org)
- `Cycle` modular softsynth, see [miditracker.org](http://miditracker.org)

At the moment there are ~100 plugins available.
