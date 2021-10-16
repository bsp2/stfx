# stfx

a lightweight, C-linkage audio plugin API that supports per-voice effects.



## Design considerations

- clutter-free, stable binary interface
- not tied to a specific plugin host
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
- `myplugin_voice_t *voice = info->voice_new(info)`
- `info->voice_delete(voice)` frees the voice instance
- voices must be freed before freeing the shared instance


## Example plugin
see [fx_example](https://github.com/bsp2/stfx/blob/master/plugins/fx/fx_example/fx_example.c)


## Status

The interface can be considered stable.
There are a few reserved bytes in each struct for future extensions. These should be set to 0.

The following applications can currently load `stfx` plugins:

- `Eureka` software sampler / synth, see (miditracker.org)[http://miditracker.org]
- `Cycle` modular softsynth, see (miditracker.org)[http://miditracker.org]

At the moment there are ~100 plugins available.
