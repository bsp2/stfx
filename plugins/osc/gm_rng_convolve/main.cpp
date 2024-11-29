// ----
// ---- file   : gm_rng_convolve.cpp
// ---- author : gm (paper) + bsp (voice plugin implementation)
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
// ---- info   : GM's RNG convolver (see https://gearspace.com/board/attachments/electronic-music-instruments-and-electronic-music-production/1014442d1650910676-novel-old-1970s-style-anolge-physical-modeling-efficient-real-time-pulse-convolution-synthesis.pdf)
// ---- created: 26Apr2022
// ---- changed: 27Apr2022
// ----
// ----
// ----

#include "../../../plugin.h"

extern "C" st_plugin_info_t *gm_rng_convolve_init (void);

ST_PLUGIN_APICALL st_plugin_info_t *ST_PLUGIN_API st_plugin_init(unsigned int _pluginIdx) {
   switch(_pluginIdx)
   {
      case 0u:
         return gm_rng_convolve_init();
   }
   return NULL;
}
