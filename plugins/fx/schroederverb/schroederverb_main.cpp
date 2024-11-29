// ----
// ---- file   : schroederverb_main.c
// ---- author : Bastian Spiegel <bs@tkscript.de>
// ---- legal  : (c) 2020-2023 by Bastian Spiegel. 
// ----          Distributed under terms of the GNU LESSER GENERAL PUBLIC LICENSE (LGPL). See 
// ----          http://www.gnu.org/licenses/licenses.html#LGPL or COPYING for further information.
// ----
// ---- info   : plugin entry point for tksampler voice plugin FX example
// ----
// ---- created: 16May2020
// ---- changed: 17May2020, 18May2020, 19May2020, 20May2020, 21May2020, 24May2020, 25May2020
// ----          26May2020, 31May2020, 01Jun2020, 02Jun2020, 06Jun2020, 07Jun2020, 08Jun2020
// ----          09Jun2020, 08Feb2021, 11Feb2021, 23Feb2021, 14Apr2021, 02May2021, 31May2021
// ----          16Aug2021, 13Oct2021, 14Oct2021, 04Jan2023, 30Mar2023
// ----
// ----
// ----

#include "../../../plugin.h"

extern "C" st_plugin_info_t *schroederverb_init (void);

extern "C" {
ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init(unsigned int _pluginIdx) {
   if(0u == _pluginIdx)
     return schroederverb_init();
   return NULL;
}
} // extern "C"
