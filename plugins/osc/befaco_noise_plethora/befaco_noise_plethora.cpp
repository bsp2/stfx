// ----
// ---- file   : befaco_noise_plethora.cpp
// ---- author : bsp
// ---- legal  : (c) 2021 Befaco
// ---- license: GPLv3
//
//        Copyright (c) 2021 Befaco
//
//            This program is free software: you can redistribute it and/or modify
//            it under the terms of the GNU General Public License as published by
//            the Free Software Foundation, either version 3 of the License, or
//            (at your option) any later version.
//        
//            This program is distributed in the hope that it will be useful,
//            but WITHOUT ANY WARRANTY; without even the implied warranty of
//            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//            GNU General Public License for more details.
//        
//            You should have received a copy of the GNU General Public License
//            along with this program.  If not, see <https://www.gnu.org/licenses/>. 
// ----
// ---- info   : Befaco Noise Plethora oscillators
// ---- created: 05Dec2021
// ---- changed: 06Dec2021
// ----
// ----
// ----

// (todo) white noise
// (todo) grit noise

#include "../../../plugin.h"

extern "C" {
extern st_plugin_info_t *noiseplethora_ab_init (void);
extern st_plugin_info_t *noiseplethora_cwhite_init (void);
extern st_plugin_info_t *noiseplethora_cgrit_init (void);
extern st_plugin_info_t *np_svf_init (void);
}

extern "C" {
ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init(unsigned int _pluginIdx) {
   switch(_pluginIdx)
   {
      case 0u:
         return noiseplethora_ab_init();

      case 1u:
         return noiseplethora_cwhite_init();

      case 2u:
         return noiseplethora_cgrit_init();

      case 3u:
         return np_svf_init();
   }
   return NULL;
}

} // extern "C"
