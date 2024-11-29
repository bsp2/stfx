/// vst2_main.cpp
///
/// (c) 2019 bsp. very loosely based on pongasoft's "hello, world" example plugin.
///
///   Licensed under the Apache License, Version 2.0 (the "License");
///   you may not use this file except in compliance with the License.
///   You may obtain a copy of the License at
///
///       http://www.apache.org/licenses/LICENSE-2.0
///
///   Unless required by applicable law or agreed to in writing, software
///   distributed under the License is distributed on an "AS IS" BASIS,
///   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
///   See the License for the specific language governing permissions and
///   limitations under the License.
///
/// created: 28Jun2019
/// changed: 07Jul2019, 08Jul2019, 10Jul2019, 12Jul2019
///
///
///

// #define DEBUG_PRINT_EVENTS  defined
// #define DEBUG_PRINT_PARAMS  defined

#define NUM_INPUTS    (   2)
#define NUM_OUTPUTS   (   2)

#define VST2_PARAM_IN_AMT       0
#define VST2_PARAM_IN_HPF       1
#define VST2_PARAM_WET_AMT      2 
#define VST2_PARAM_WET_GAIN     3
#define VST2_PARAM_CB_MIX       4
#define VST2_PARAM_CLIP_LVL     5
#define VST2_PARAM_DLY_LSPD     6
#define VST2_PARAM_FBMOD        7
#define VST2_PARAM_FCBMOD       8
#define VST2_PARAM_FAPMOD       9
#define VST2_PARAM_FDLYMOD     10 
#define VST2_PARAM_FREQSCLL    11
#define VST2_PARAM_FREQSCLR    12
#define VST2_PARAM_CB1_NOTE    13
#define VST2_PARAM_CB2_NOTE    14
#define VST2_PARAM_CB3_NOTE    15
#define VST2_PARAM_DLYX_NOTE   16
#define VST2_PARAM_CB1_FBAMT   17
#define VST2_PARAM_CB2_FBAMT   18
#define VST2_PARAM_CB3_FBAMT   19
#define VST2_PARAM_DLYX_FBAMT  20
#define VST2_PARAM_AP1_B       21
#define VST2_PARAM_AP1_C       22
#define VST2_PARAM_AP1_NOTE    23
#define VST2_PARAM_AP2_B       24
#define VST2_PARAM_AP2_C       25
#define VST2_PARAM_AP2_NOTE    26
#define VST2_PARAM_APFB_B      27
#define VST2_PARAM_APFB_C      28
#define VST2_PARAM_APFB_NOTE   29
#define VST2_PARAM_CB1_FBDCY   30
#define VST2_PARAM_CB2_FBDCY   31
#define VST2_PARAM_CB3_FBDCY   32
#define VST2_PARAM_DLYX_FBDCY  33
#define VST2_PARAM_X1_AMT      34
#define VST2_PARAM_X2_AMT      35
#define VST2_PARAM_X3_AMT      36
#define VST2_PARAM_X4_AMT      37
#define VST2_PARAM_SWAYCBF     38
#define VST2_PARAM_SWAYCBM     39
#define VST2_PARAM_SWAYCBX     40
#define VST2_PARAM_SWAYAPF     41
#define VST2_PARAM_SWAYAPM     42
#define VST2_PARAM_SWAYAPX     43
#define VST2_PARAM_SWAYDLYF    44
#define VST2_PARAM_SWAYDLYM    45
#define VST2_PARAM_SWAYDLYX    46
#define VST2_PARAM_SWAYFB      47
#define VST2_PARAM_BUFSZCB     48
#define VST2_PARAM_BUFSZDLY    49 
#define VST2_PARAM_LINEAR      50
#define VST2_PARAM_PRIME       51
#define VST2_PARAM_OUT_LPF     52
#define VST2_PARAM_CB_MIX_IN   53
#define VST2_PARAM_OUT_LVL     54
#define VST2_MAX_PARAM_IDS     55

// (note) causes reason to shut down when console is freed (when plugin is deleted)
// #define USE_CONSOLE  defined
// #define VST2_OPCODE_DEBUG  defined

#undef RACK_HOST

#define Dprintf if(0);else printf
// #define Dprintf if(1);else printf

// #define Dprintf_idle if(0);else printf
#define Dprintf_idle if(1);else printf

#include <aeffect.h>
#include <aeffectx.h>
#include <stdio.h>
#ifdef HAVE_UNIX
#include <unistd.h>
#endif

// #define YAC_TYPES_ONLY  defined
#include <yac.h>
YAC_Host *yac_host;  // unused, just to satisfy the linker
#include <yac_host.cpp>


#include "sway.h"
#include "allpass_ff_dly.h"
#include "comb.h"
#include "delay.h"
#include "prime.h"


#if defined(_WIN32) || defined(_WIN64)
#define HAVE_WINDOWS defined

#define WIN32_LEAN_AND_MEAN defined
#include <windows.h>
#include <xmmintrin.h>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;


// Windows:
#define VST_EXPORT  extern "C" __declspec(dllexport)


struct PluginMutex {
   CRITICAL_SECTION handle; 

   PluginMutex(void) {
      ::InitializeCriticalSection( &handle ); 
   }

   ~PluginMutex() {
      ::DeleteCriticalSection( &handle ); 
   }

   void lock(void) {
      ::EnterCriticalSection(&handle); 
   }

   void unlock(void) {
      ::LeaveCriticalSection(&handle); 
   }
};

#else

// MacOSX, Linux:
#define HAVE_UNIX defined

#define VST_EXPORT extern

#include <pthread.h> 
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <fenv.h>  // fesetround()
#include <stdarg.h>

// #define USE_LOG_PRINTF  defined

#ifdef USE_LOG_PRINTF
static FILE *logfile;
#undef Dprintf
#define Dprintf log_printf

void log_printf(const char *logData, ...) {
   static char buf[16*1024]; 
   va_list va; 
   va_start(va, logData); 
   vsprintf(buf, logData, va);
   va_end(va); 
   printf(buf); 
   fputs(buf, logfile);
   fflush(logfile);
}
#endif // USE_LOG_PRINTF


// #define _GNU_SOURCE
#include <dlfcn.h>

#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
static pthread_mutex_t loc_pthread_mutex_t_init = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#elif defined(PTHREAD_RECURSIVE_MUTEX_INITIALIZER)
//macOS
static pthread_mutex_t loc_pthread_mutex_t_init = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#endif

// static pthread_mutex_t loc_pthread_mutex_t_init = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
// //static pthread_mutex_t loc_pthread_mutex_t_init = PTHREAD_MUTEX_INITIALIZER;

struct PluginMutex {
   pthread_mutex_t handle;

   PluginMutex(void) {
      ::memcpy((void*)&handle, (const void*)&loc_pthread_mutex_t_init, sizeof(pthread_mutex_t));
   }

   ~PluginMutex() {
   }

   void lock(void) {
		::pthread_mutex_lock(&handle); 
   }

   void unlock(void) {
		::pthread_mutex_unlock(&handle); 
   }
};

#endif // _WIN32||_WIN64



class PluginString : public YAC_String {
public:
   static const sUI STRFLQMASK  = (QUOT | QUOTR | QUOT2);

   void           safeFreeChars      (void);
   sSI          _realloc             (sSI _numChars);
   sSI           lastIndexOf         (sChar _c, sUI _start) const;
   void          getDirName          (PluginString *_r) const;
   void          replace             (sChar _c, sChar _o);
};

void PluginString::safeFreeChars(void) {
   if(bflags & PluginString::DEL)
   {
      // if(!(bflags & PluginString::LA))
      {
         Dyacfreechars(chars);
      }
   }
}

sSI PluginString::_realloc(sSI _numBytes) { 

   // Force alloc if a very big string is about to shrink a lot or there is simply not enough space available
   if( ((buflen >= 1024) && ( (((sUI)_numBytes)<<3) < buflen )) || 
       (NULL == chars) || 
       (buflen < ((sUI)_numBytes))
       ) // xxx (!chars) hack added 180702 
   {
      if(NULL != chars) 
      { 
         sUI l = length; 

         if(((sUI)_numBytes) < l)
         {
            l = _numBytes;
         }

         sU8 *nc = Dyacallocchars(_numBytes + 1);
         sUI i = 0;

         for(; i<l; i++)
         {
            nc[i] = chars[i];
         }

         nc[i] = 0; 
 
         safeFreeChars();
		   buflen = (_numBytes + 1);
         bflags = PluginString::DEL | (bflags & PluginString::STRFLQMASK); // keep old stringflags
         length = i + 1;
         chars  = nc;
         key    = YAC_LOSTKEY;

         return YAC_TRUE;
      } 
      else 
      {
		   return PluginString::alloc(_numBytes + 1);
      }
   } 
	else 
	{ 
      key = YAC_LOSTKEY; // new 010208

		return YAC_TRUE;
	} 
}

sSI PluginString::lastIndexOf(sChar _c, sUI _start) const {
   sSI li = -1;

   if(NULL != chars)
   {
      sUI i = _start;

      for(; i<length; i++)
      {
         if(chars[i] == ((sChar)_c))
         {
            li = i;
         }
      }
   }

   return li;
}

void PluginString::replace(sChar _c, sChar _o) {
   if(NULL != chars)
   {
      for(sUI i = 0; i < length; i++)
      {
         if(chars[i] == _c)
            chars[i] = _o;
      }
   }
}

void PluginString::getDirName(PluginString *_r) const {
   sSI idxSlash     = lastIndexOf('/', 0);
   sSI idxBackSlash = lastIndexOf('\\', 0);
   sSI idxDrive     = lastIndexOf(':', 0);
   sSI idx = -1;

   if(idxSlash > idxBackSlash)
   {
      idx = idxSlash;
   }
   else
   {
      idx = idxBackSlash;
   }

   if(idxDrive > idx)
   {
      idx = idxDrive;
   }

   if(-1 != idx)
   {
      _r->_realloc(idx + 2);
      _r->length = idx + 2;

      sSI i;
      for(i=0; i<=idx; i++)
      {
         _r->chars[i] = chars[i];
      }

      _r->chars[i++] = 0;
      _r->key = YAC_LOSTKEY;
   }
   else
   {
      _r->empty();
   }
}

#define MAX_FLOATARRAYALLOCSIZE (1024*1024*64)

class PluginFloatArray : public YAC_FloatArray {
public:
   sSI  alloc          (sSI _maxelements);
};

sSI PluginFloatArray::alloc(sSI _max_elements) {
   if(((sUI)_max_elements)>MAX_FLOATARRAYALLOCSIZE)  
   {
      printf("[---] FloatArray::insane array size (maxelements=%08x)\n", _max_elements);
      return 0;
   }
   if(own_data) 
   {
      if(elements)  
      { 
         delete [] elements; 
         elements = NULL;
      } 
   }
   if(_max_elements)
   {
      elements = new(std::nothrow) sF32[_max_elements];
      if(elements)
      {
         max_elements = _max_elements;
         num_elements = 0; 
         own_data     = 1;
         return 1;
      }
   }
   num_elements = 0;
   max_elements = 0;
   return 0;
}



/*
 * I find the naming a bit confusing so I decided to use more meaningful names instead.
 */

/**
 * The VSTHostCallback is a function pointer so that the plugin can communicate with the host (not used in this small example)
 */
typedef audioMasterCallback VSTHostCallback;

/**
 * The VSTPlugin structure (AEffect) contains information about the plugin (like version, number of inputs, ...) and
 * callbacks so that the host can call the plugin to do its work. The primary callback will be `processReplacing` for
 * single precision (float) sample processing (or `processDoubleReplacing` for double precision (double)).
 */
typedef AEffect VSTPlugin;


// Since the host is expecting a very specific API we need to make sure it has C linkage (not C++)
extern "C" {

/*
 * This is the main entry point to the VST plugin.
 *
 * The host (DAW like Maschine, Ableton Live, Reason, ...) will look for this function with this exact API.
 *
 * It is the equivalent to `int main(int argc, char *argv[])` for a C executable.
 *
 * @param vstHostCallback is a callback so that the plugin can communicate with the host (not used in this small example)
 * @return a pointer to the AEffect structure
 */
VST_EXPORT VSTPlugin *VSTPluginMain (VSTHostCallback vstHostCallback);

// note this looks like this without the type aliases (and is obviously 100% equivalent)
// extern AEffect *VSTPluginMain(audioMasterCallback audioMaster);

}

/*
 * Constant for the version of the plugin. For example 1100 for version 1.1.0.0
 */
const VstInt32 PLUGIN_VERSION = 1000;



struct SchroederCh {
   AllPassFFDly ap_1;
   AllPassFFDly ap_2;
   AllPassFFDly ap_fb;
   Comb      cb_1;
   Comb      cb_2;
   Comb      cb_3;
   Delay     dly_x;

   sF32 last_in;
   sF32 last_f;
   sF32 last_out;

   void init(sF32 _sampleRate) {

      ap_1.setSampleRate(_sampleRate);
      ap_2.setSampleRate(_sampleRate);
      ap_fb.setSampleRate(_sampleRate);

      ap_1.setB( 0.3235f);
      ap_1.setC(-0.3235f);
      ap_2.setB(0.63147f);
      ap_2.setC(-0.63147f);
      ap_fb.setB(0.234234f);
      ap_fb.setC(-0.3234234f);

      sF32  cbFbAmt    = 0.87f;
      sF32  noteShift  = 0.0f;
      sF32  noteScl    = 0.5f;

      cb_1.setSampleRate(_sampleRate);
      cb_1.setNote( (noteShift + 12*3 + 0.0f) * noteScl );
      cb_1.setFbAmt(cbFbAmt);

      cb_2.setSampleRate(_sampleRate);
      cb_2.setNote( (noteShift + 12*2 + 3.5f) * noteScl );
      cb_2.setFbAmt(cbFbAmt);

      cb_3.setSampleRate(_sampleRate);
      cb_3.setNote( (noteShift + 12*4 + 7.35f) * noteScl );
      cb_3.setFbAmt(cbFbAmt);

      dly_x.setSampleRate(_sampleRate);
      dly_x.setNote( (noteShift + 12*2 + 0.0f) * noteScl );
      dly_x.setFbAmt(cbFbAmt);

      cb_1.handleParamsUpdated();
      cb_2.handleParamsUpdated();
      cb_3.handleParamsUpdated();
      dly_x.handleParamsUpdated();

      last_in  = 0.0f;
      last_f   = 0.0f;
      last_out = 0.0f;

      dly_x.setFbAmt(0.3f);
      dly_x.setFbDcy(0.2f);
   }

   void setDlyLerpSpdNorm(sF32 _spd) {
      cb_1.setDlyLerpSpdNorm(_spd);
      cb_2.setDlyLerpSpdNorm(_spd);
      cb_3.setDlyLerpSpdNorm(_spd);
      dly_x.setDlyLerpSpdNorm(_spd);
      ap_1.setDlyLerpSpdNorm(_spd);
      ap_2.setDlyLerpSpdNorm(_spd);
      ap_fb.setDlyLerpSpdNorm(_spd);
   }

   void setPrimeTbl(const sUI *_primeTbl) {
      cb_1.setPrimeTbl(_primeTbl);
      cb_2.setPrimeTbl(_primeTbl);
      cb_3.setPrimeTbl(_primeTbl);
      dly_x.setPrimeTbl(_primeTbl);
   }
};



/**
 * Encapsulates the plugin as a C++ class. It will keep both the host callback and the structure required by the
 * host (VSTPlugin). This class will be stored in the `VSTPlugin.object` field (circular reference) so that it can
 * be accessed when the host calls the plugin back (for example in `processDoubleReplacing`).
 */
class VSTPluginWrapper {
public:
   static const sUI MIN_SAMPLE_RATE = 8192u;
   static const sUI MAX_SAMPLE_RATE = 384000u;
   static const sUI MIN_BLOCK_SIZE  = 64u;
   static const sUI MAX_BLOCK_SIZE  = 16384u;

public:

protected:
   PluginString dllname;
   PluginString cwd;

public:
   float    sample_rate;   // e.g. 44100.0

protected:
   sUI block_size;    // e.g. 64   

   PluginMutex mtx_audio;

public:
   PluginMutex mtx_mididev;

public:

   bool b_open;
   bool b_processing;  // true=generate output, false=suspended
   bool b_offline;  // true=offline rendering (HQ)
   bool b_check_offline;  // true=ask host if it's in offline rendering mode

   sBool b_fix_denorm;  // true=fix denormalized floats + clip to -4..4. fixes broken audio in FLStudio and Reason.

   char *last_program_chunk_str;

   static sSI instance_count;
   sSI instance_id;


public:
   SchroederCh rev[2];

   sF32 in_amt;
   sF32 in_hpf;
   sF32 wet_amt;

   sF32 x1_amt;  // crosschannel feedback
   sF32 x2_amt;
   sF32 x3_amt;
   sF32 x4_amt;

   sF32 wet_gain;
   sF32 comb_mix;
   sF32 comb_mix_in;  // input mix
   sF32 clip_lvl;
   sF32 dly_lerp_spd;
   sF32 out_lpf;
   sF32 out_lvl;

   PrimeLock prime_lock;


public:
   VSTPluginWrapper(VSTHostCallback vstHostCallback,
                    VstInt32 vendorUniqueID,
                    VstInt32 vendorVersion,
                    VstInt32 numParams,
                    VstInt32 numPrograms,
                    VstInt32 numInputs,
                    VstInt32 numOutputs
                    );

   ~VSTPluginWrapper();

   VSTPlugin *getVSTPlugin(void) {
      return &_vstPlugin;
   }

   void initSchroeder(void) {
      rev[0].init(sample_rate);
      rev[1].init(sample_rate);
      in_amt = 0.3f;
      in_hpf = 0.92f;
      wet_amt = 0.29f;
      x1_amt = 1.0f;
      x2_amt = 0.4f;
      x3_amt = 0.0f;
      x4_amt = 1.0f;
      wet_gain = 1.0f;
      comb_mix = 0.333f;
      comb_mix_in = 0.0f;
      clip_lvl = 1.0;
      dly_lerp_spd = 0.05f;
      rev[0].setDlyLerpSpdNorm(dly_lerp_spd);
      rev[1].setDlyLerpSpdNorm(dly_lerp_spd);
      rev[0].setPrimeTbl(prime_lock.prime_tbl);
      rev[1].setPrimeTbl(prime_lock.prime_tbl);

      rev[0].cb_1.setFreqScl(0.43f);
      rev[0].cb_2.setFreqScl(0.43f);
      rev[0].cb_3.setFreqScl(0.43f);

      rev[1].cb_1.setFreqScl(0.47f);
      rev[1].cb_2.setFreqScl(0.47f);
      rev[1].cb_3.setFreqScl(0.47f);

      out_lpf = 0.0f;
      out_lvl = 1.0f;
   }

   void process(sF32**_inputs, sF32**_outputs, sUI _numFrames) {
      // printf("xxx process %u frames\n", _numFrames);
      for(sUI frameIdx = 0u; frameIdx < _numFrames; frameIdx++)
      {
         for(sUI chIdx = 0u; chIdx < 2u; chIdx++)
         {
            SchroederCh *ch = &rev[chIdx];
            SchroederCh *chO = &rev[chIdx ^ 1u];

            sF32 f;
            sF32 inSmpRaw = _inputs[chIdx][frameIdx];
            sF32 inSmp;

            inSmp = inSmpRaw - ((ch->last_in + inSmpRaw) * in_hpf * 0.5f);
            ch->last_in = inSmpRaw;

            sF32 fO = ch->ap_fb.process(chO->last_out * x1_amt);
            fO = ch->dly_x.process(fO);

            // // f = ch->ap_1.process(inSmp * in_amt + fO * x2_amt);  // v4
            f = ch->ap_1.process(inSmp * in_amt);
            f = ch->ap_2.process(f);

            sF32 fCb1 = ch->cb_1.process(f);
            sF32 fCb2 = ch->cb_2.process(f);
            sF32 fCb3 = ch->cb_3.process(f);

            // // f = fCb1 + fCb2 + fCb3;
            // // f *= comb_mix; //0.333f in v4

            f = (fCb1 + fCb2 + fCb3) * comb_mix + f * comb_mix_in;

            f += fO * x2_amt;  // v5

            if(f > clip_lvl)
               f = clip_lvl;
            else if(f < -clip_lvl)
               f = -clip_lvl;

            f = f + 10.0f;
            f = f - 10.0f;

            ch->last_out = ch->last_out * x3_amt + f * x4_amt;
            f *= wet_gain;
            sF32 lpf = ((ch->last_f + f) * 0.5f);
            ch->last_f = f;
            f = f + (lpf - f) * out_lpf;
            f  = inSmpRaw + ((f - inSmpRaw) * wet_amt) * wet_amt;
         
            _outputs[chIdx][frameIdx] = f * out_lvl;
         }
      }
   }

   sSI openEffect(void) {

      Dprintf("xxx vstschroeder_plugin::openEffect\n");

      // (todo) use mutex 
      instance_id = instance_count;
      Dprintf("xxx vstschroeder_plugin::openEffect: instance_id=%d\n", instance_id);

#ifdef USE_CONSOLE
      AllocConsole();
      freopen("CON", "w", stdout);
      freopen("CON", "w", stderr);
      freopen("CON", "r", stdin); // Note: "r", not "w".
#endif // USE_CONSOLE

      char oldCWD[1024];
      char dllnameraw[1024];
      char *dllnamerawp = dllnameraw;

#ifdef HAVE_WINDOWS
      ::GetCurrentDirectory(1024, (LPSTR) oldCWD);
      // ::GetModuleFileNameA(NULL, dllnameraw, 1024); // returns executable name (not the dll pathname)
      GetModuleFileNameA((HINSTANCE)&__ImageBase, dllnameraw, 1024);
#elif defined(HAVE_UNIX)
      getcwd(oldCWD, 1024);
#if 0
      // this does not work, it reports the path of the host, not the plugin
      // (+the string is not NULL-terminated from the looks of it)
      readlink("/proc/self/exe", dllnameraw, 1024);
#else
      Dl_info dlInfo;
      ::dladdr((void*)VSTPluginMain, &dlInfo);
      // // dllnamerawp = (char*)dlInfo.dli_fname;
      if('/' != dlInfo.dli_fname[0])
      {
         // (note) 'dli_fname' can be a relative path (e.g. when loaded from vst2_debug_host)
         Dyac_snprintf(dllnameraw, 1024, "%s/%s", oldCWD, dlInfo.dli_fname);
      }
      else
      {
         // Absolute path (e.g. when loaded from Renoise host)
         dllnamerawp = (char*)dlInfo.dli_fname;
      }
#endif
#endif

      Dprintf("xxx vstschroeder_plugin::openEffect: dllnamerawp=\"%s\"\n", dllnamerawp);
      dllname.visit(dllnamerawp);
      dllname.getDirName(&cwd);

      Dprintf("xxx vstschroeder_plugin::openEffect: cd to \"%s\"\n", (const char*)cwd.chars);
#ifdef HAVE_WINDOWS
      ::SetCurrentDirectory((const char*)cwd.chars);
#elif defined(HAVE_UNIX)
      chdir((const char*)cwd.chars);
#endif
      Dprintf("xxx vstschroeder_plugin::openEffect: cwd change done\n");
      // cwd.replace('\\', '/');

//       int argc = 1;
//       char *argv[1];
//       //argv[0] = (char*)cwd.chars;
//       argv[0] = (char*)dllnamerawp;
//       Dprintf("xxx argv[0]=%p\n", argv[0]);
//       Dprintf("xxx vstschroeder_plugin::openEffect: dllname=\"%s\"\n", argv[0]);
//       (void)vst2_init(argc, argv,
// #ifdef VST2_EFFECT
//                       true/*bFX*/
// #else
//                       false/*bFX*/
// #endif // VST2_EFFECT
//                       );
//       Dprintf("xxx vstschroeder_plugin::openEffect: vst2_init() done\n");

      Dprintf("xxx vstschroeder_plugin::openEffect: restore cwd=\"%s\"\n", oldCWD);      

#ifdef HAVE_WINDOWS
      ::SetCurrentDirectory(oldCWD);
#elif defined(HAVE_UNIX)
      chdir(oldCWD);
#endif

      // Init actual effect
      initSchroeder();

      setSampleRate(sample_rate);

      b_open = true;

      Dprintf("xxx vstschroeder_plugin::openEffect: LEAVE\n");
      return 1;
   }

   void closeEffect(void) {

      // (todo) use mutex
      Dprintf("xxx vstschroeder_plugin::closeEffect: last_program_chunk_str=%p\n", last_program_chunk_str);
      if(NULL != last_program_chunk_str)
      {
         ::free(last_program_chunk_str);
         last_program_chunk_str = NULL;
      }

      Dprintf("xxx vstschroeder_plugin::closeEffect: b_open=%d\n", b_open);

      if(b_open)
      {
         b_open = false;

#ifdef USE_CONSOLE
         Sleep(5000);
         // FreeConsole();
#endif // USE_CONSOLE
      }

   }

   void lockAudio(void) {
      mtx_audio.lock();
   }

   void unlockAudio(void) {
      mtx_audio.unlock();
   }

   VstInt32 getNumInputs(void) const {
      return _vstPlugin.numInputs;
   }

   VstInt32 getNumOutputs(void) const {
      return _vstPlugin.numOutputs;
   }

   bool setSampleRate(float _rate, bool _bLock = true) {
      bool r = false;

      // Dprintf("xxx setSampleRate: ENTER bLock=%d\n", _bLock);

      if((_rate >= float(MIN_SAMPLE_RATE)) && (_rate <= float(MAX_SAMPLE_RATE)))
      {
         if(_bLock)
         {
            lockAudio();
         }

         sample_rate = _rate;

         if(_bLock)
         {
            unlockAudio();
         }

         r = true;
      }

      // Dprintf("xxx setSampleRate: LEAVE\n");

      return r;
   }

   bool setBlockSize(sUI _blockSize) {
      bool r = false;

      if((_blockSize >= MIN_BLOCK_SIZE) && (_blockSize <= MAX_BLOCK_SIZE))
      {
         lockAudio();
         block_size = _blockSize;
         unlockAudio();
         r = true;
      }

      return r;
   }

   void setEnableProcessingActive(bool _bEnable) {
      lockAudio();
      b_processing = _bEnable;
      unlockAudio();
   }

   void checkOffline(void) {
      // Called by VSTPluginProcessReplacingFloat32()
      if(b_check_offline)
      {
         if(NULL != _vstHostCallback)
         {
            int level = (int)_vstHostCallback(&_vstPlugin, audioMasterGetCurrentProcessLevel, 0, 0/*value*/, NULL/*ptr*/, 0.0f/*opt*/);
            // (note) Reason sets process level to kVstProcessLevelUser during "bounce in place"
            bool bOffline = (kVstProcessLevelOffline == level) || (kVstProcessLevelUser == level);

#if 0
            {
               static int i = 0;
               if(0 == (++i & 127))
               {
                  Dprintf("xxx vstschroeder_plugin: audioMasterGetCurrentProcessLevel: level=%d\n", level);
               }
            }
#endif

            if(b_offline ^ bOffline)
            {
               // Offline mode changed, update resampler
               b_offline = bOffline;

               Dprintf("xxx vstschroeder_plugin: mode changed to %d\n", int(bOffline));
            }
         }
      }      
   }

   sUI getBankChunk(sU8 **_addr) {
      return 0;
   }

   void setEnableFixDenorm(sSI _bEnable) {
      b_fix_denorm = (0 != _bEnable);
      Dprintf("vst2_main:setEnableFixDenorm(%d)\n", b_fix_denorm);
   }

   sSI getEnableFixDenorm(void) const {
      return sSI(b_fix_denorm);
   }

   sUI getProgramChunk(sU8**_addr) {
      sUI r = 0;
      // if(NULL != last_program_chunk_str)
      // {
      //    ::free(last_program_chunk_str);
      // }
      // // last_program_chunk_str = rack::global_ui->app.gRackWidget->savePatchToString();
      // if(NULL != last_program_chunk_str)
      // {
      //    *_addr = (sU8*)last_program_chunk_str;
      //    r = (sUI)strlen(last_program_chunk_str) + 1/*ASCIIZ*/;
      // }
      return r;
   }

   bool setBankChunk(size_t _size, sU8 *_addr) {
      bool r = false;
      return r;
   }

   bool setProgramChunk(size_t _size, sU8 *_addr) {
//       Dprintf("xxx vstschroeder_plugin:setProgramChunk: ENTER\n");
//       lockAudio();
// #if 0
//       Dprintf("xxx vstschroeder_plugin:setProgramChunk: size=%u\n", _size);
// #endif
//       // bool r = rack::global_ui->app.gRackWidget->loadPatchFromString((const char*)_addr);
//       unlockAudio();
//       Dprintf("xxx vstschroeder_plugin:setProgramChunk: LEAVE\n");
//       return r;
      return 0;
   }

#ifdef HAVE_WINDOWS
   void sleepMillisecs(sUI _num) {
      ::Sleep((DWORD)_num);
   }
#elif defined(HAVE_UNIX)
   void sleepMillisecs(sUI _num) {
      ::usleep(1000u * _num);
   }
#endif

   void getParamName(sSI _index, char *_ptr, sSI _maxParamStrLen) {
      // printf("xxx getParamName: index=%d ptr=%p\n", _index, _ptr);
      switch(_index)
      {
         default:
         case VST2_PARAM_IN_AMT:
            strcpy(_ptr, "In Lvl");
            break;

         case VST2_PARAM_IN_HPF:
            strcpy(_ptr, "In HPF");
            break;

         case VST2_PARAM_WET_AMT:
            strcpy(_ptr, "Dry/Wet");
            break;

         case VST2_PARAM_FREQSCLL:
            strcpy(_ptr, "FreqSclL");
            break;

         case VST2_PARAM_FREQSCLR:
            strcpy(_ptr, "FreqSclR");
            break;

         case VST2_PARAM_CB1_NOTE:
            strcpy(_ptr, "Cb1.Note");
            break;

         case VST2_PARAM_CB2_NOTE:
            strcpy(_ptr, "Cb2.Note");
            break;

         case VST2_PARAM_CB3_NOTE:
            strcpy(_ptr, "Cb3.Note");
            break;

         case VST2_PARAM_DLYX_NOTE:
            strcpy(_ptr, "DlX.Note");
            break;

         case VST2_PARAM_CB1_FBAMT:
            strcpy(_ptr, "Cb1.Fb");
            break;

         case VST2_PARAM_CB2_FBAMT:
            strcpy(_ptr, "Cb2.Fb");
            break;

         case VST2_PARAM_CB3_FBAMT:
            strcpy(_ptr, "Cb3.Fb");
            break;

         case VST2_PARAM_DLYX_FBAMT:
            strcpy(_ptr, "DlX.Fb");
            break;

         case VST2_PARAM_AP1_B:
            strcpy(_ptr, "Ap1.B");
            break;

         case VST2_PARAM_AP1_C:
            strcpy(_ptr, "Ap1.C");
            break;

         case VST2_PARAM_AP1_NOTE:
            strcpy(_ptr, "Ap1.Note");
            break;

         case VST2_PARAM_AP2_B:
            strcpy(_ptr, "Ap2.B");
            break;

         case VST2_PARAM_AP2_C:
            strcpy(_ptr, "Ap2.C");
            break;

         case VST2_PARAM_AP2_NOTE:
            strcpy(_ptr, "Ap2.Note");
            break;

         case VST2_PARAM_APFB_B:
            strcpy(_ptr, "ApFB.B");
            break;

         case VST2_PARAM_APFB_C:
            strcpy(_ptr, "ApFB.C");
            break;

         case VST2_PARAM_APFB_NOTE:
            strcpy(_ptr, "ApFB.Note");
            break;

         case VST2_PARAM_CB1_FBDCY:
            strcpy(_ptr, "Cb1.Dcy");
            break;

         case VST2_PARAM_CB2_FBDCY:
            strcpy(_ptr, "Cb2.Dcy");
            break;

         case VST2_PARAM_CB3_FBDCY:
            strcpy(_ptr, "Cb3.Dcy");
            break;

         case VST2_PARAM_DLYX_FBDCY:
            strcpy(_ptr, "DlX.Dcy");
            break;

         case VST2_PARAM_X1_AMT:
            strcpy(_ptr, "DlX.X1");
            break;

         case VST2_PARAM_X2_AMT:
            strcpy(_ptr, "DlX.X2");
            break;

         case VST2_PARAM_X3_AMT:
            strcpy(_ptr, "DlX.X3");
            break;

         case VST2_PARAM_X4_AMT:
            strcpy(_ptr, "DlX.X4");
            break;

         case VST2_PARAM_WET_GAIN:
            strcpy(_ptr, "Wet Gain");
            break;

         case VST2_PARAM_CB_MIX:
            strcpy(_ptr, "Cb Mix");
            break;

         case VST2_PARAM_CLIP_LVL:
            strcpy(_ptr, "Clip Lvl");
            break;

         case VST2_PARAM_DLY_LSPD:
            strcpy(_ptr, "Dly LSpd");
            break;

         case VST2_PARAM_FCBMOD:
            strcpy(_ptr, "FCbMod");
            break;

         case VST2_PARAM_FAPMOD:
            strcpy(_ptr, "FApMod");
            break;

         case VST2_PARAM_FDLYMOD:
            strcpy(_ptr, "FDlyMod");
            break;

         case VST2_PARAM_FBMOD:
            strcpy(_ptr, "FbMod");
            break;

         case VST2_PARAM_BUFSZCB:
            strcpy(_ptr, "BufSzCb");
            break;

         case VST2_PARAM_BUFSZDLY:
            strcpy(_ptr, "BufSzDly");
            break;

         case VST2_PARAM_LINEAR:
            strcpy(_ptr, "Linear");
            break;

         case VST2_PARAM_SWAYCBF:
            strcpy(_ptr, "SwayCbF");
            break;

         case VST2_PARAM_SWAYCBM:
            strcpy(_ptr, "SwayCbM");
            break;

         case VST2_PARAM_SWAYCBX:
            strcpy(_ptr, "SwayCbX");
            break;

         case VST2_PARAM_SWAYAPF:
            strcpy(_ptr, "SwayApF");
            break;

         case VST2_PARAM_SWAYAPM:
            strcpy(_ptr, "SwayApM");
            break;

         case VST2_PARAM_SWAYAPX:
            strcpy(_ptr, "SwayApX");
            break;

         case VST2_PARAM_SWAYDLYF:
            strcpy(_ptr, "SwayDlyF");
            break;

         case VST2_PARAM_SWAYDLYM:
            strcpy(_ptr, "SwayDlyM");
            break;

         case VST2_PARAM_SWAYDLYX:
            strcpy(_ptr, "SwayDlyX");
            break;

         case VST2_PARAM_SWAYFB:
            strcpy(_ptr, "SwayFb");
            break;

         case VST2_PARAM_PRIME:
            strcpy(_ptr, "Prime");
            break;

         case VST2_PARAM_OUT_LPF:
            strcpy(_ptr, "Out LPF");
            break;

         case VST2_PARAM_CB_MIX_IN:
            strcpy(_ptr, "Cb MixIn");
            break;

         case VST2_PARAM_OUT_LVL:
            strcpy(_ptr, "Out Lvl");
            break;
      }
      // printf("xxx   2 getParamName: index=%d ptr=%p\n", _index, _ptr);
   }

   void setParam(sSI _index, sF32 _value) {
      switch(_index)
      {
         default:
         case VST2_PARAM_IN_AMT:
            in_amt = _value;
            break;

         case VST2_PARAM_IN_HPF:
            in_hpf = _value;
            break;

         case VST2_PARAM_WET_AMT:
            wet_amt = _value;
            break;

         case VST2_PARAM_FREQSCLL:
            rev[0].cb_1.setFreqScl(_value);
            rev[0].cb_2.setFreqScl(_value);
            rev[0].cb_3.setFreqScl(_value);
            rev[0].dly_x.setFreqScl(_value);

            rev[0].cb_1.handleParamsUpdated();
            rev[0].cb_2.handleParamsUpdated();
            rev[0].cb_3.handleParamsUpdated();
            rev[0].dly_x.handleParamsUpdated();
            break;

         case VST2_PARAM_FREQSCLR:
            rev[1].cb_1.setFreqScl(_value);
            rev[1].cb_2.setFreqScl(_value);
            rev[1].cb_3.setFreqScl(_value);
            rev[1].dly_x.setFreqScl(_value);

            rev[1].cb_1.handleParamsUpdated();
            rev[1].cb_2.handleParamsUpdated();
            rev[1].cb_3.handleParamsUpdated();
            rev[1].dly_x.handleParamsUpdated();
            break;

         case VST2_PARAM_CB1_NOTE:
            rev[0].cb_1.setNote(_value * 100.0f);
            rev[1].cb_1.setNote(rev[0].cb_1.note);
            rev[0].cb_1.handleParamsUpdated();
            rev[1].cb_1.handleParamsUpdated();
            break;

         case VST2_PARAM_CB2_NOTE:
            rev[0].cb_2.setNote(_value * 100.0f);
            rev[1].cb_2.setNote(rev[0].cb_2.note);
            rev[0].cb_2.handleParamsUpdated();
            rev[1].cb_2.handleParamsUpdated();
            break;

         case VST2_PARAM_CB3_NOTE:
            rev[0].cb_3.setNote(_value * 100.0f);
            rev[1].cb_3.setNote(rev[0].cb_3.note);
            rev[0].cb_3.handleParamsUpdated();
            rev[1].cb_3.handleParamsUpdated();
            break;

         case VST2_PARAM_DLYX_NOTE:
            rev[0].dly_x.setNote(_value * 100.0f);
            rev[1].dly_x.setNote(rev[0].dly_x.note);
            rev[0].dly_x.handleParamsUpdated();
            rev[1].dly_x.handleParamsUpdated();
            break;

         case VST2_PARAM_CB1_FBAMT:
            rev[0].cb_1.setFbAmt(_value);
            rev[1].cb_1.setFbAmt(rev[0].cb_1.fb_amt);
            break;

         case VST2_PARAM_CB2_FBAMT:
            rev[0].cb_2.setFbAmt(_value);
            rev[1].cb_2.setFbAmt(rev[0].cb_2.fb_amt);
            break;

         case VST2_PARAM_CB3_FBAMT:
            rev[0].cb_3.setFbAmt(_value);
            rev[1].cb_3.setFbAmt(rev[0].cb_3.fb_amt);
            break;

         case VST2_PARAM_DLYX_FBAMT:
            rev[0].dly_x.setFbAmt(_value);
            rev[1].dly_x.setFbAmt(rev[0].dly_x.fb_amt);
            break;

         case VST2_PARAM_AP1_B:
            rev[0].ap_1.setB((_value - 0.5f) * 2.0f);
            rev[1].ap_1.setB(rev[0].ap_1.b);
            break;

         case VST2_PARAM_AP1_C:
            rev[0].ap_1.setC((_value - 0.5f) * 2.0f);
            rev[1].ap_1.setC(rev[0].ap_1.c);
            break;

         case VST2_PARAM_AP1_NOTE:
            rev[0].ap_1.setNote(_value * 100.0f);
            rev[1].ap_1.setNote(rev[0].ap_1.note);
            rev[0].ap_1.handleParamsUpdated();
            rev[1].ap_1.handleParamsUpdated();
            break;

         case VST2_PARAM_AP2_B:
            rev[0].ap_2.setB((_value - 0.5f) * 2.0f);
            rev[1].ap_2.setB(rev[0].ap_2.b);
            break;

         case VST2_PARAM_AP2_C:
            rev[0].ap_2.setC((_value - 0.5f) * 2.0f);
            rev[1].ap_2.setC(rev[0].ap_2.c);
            break;

         case VST2_PARAM_AP2_NOTE:
            rev[0].ap_2.setNote(_value * 100.0f);
            rev[1].ap_2.setNote(rev[0].ap_2.note);
            rev[0].ap_2.handleParamsUpdated();
            rev[1].ap_2.handleParamsUpdated();
            break;

         case VST2_PARAM_APFB_B:
            rev[0].ap_fb.setB((_value - 0.5f) * 2.0f);
            rev[1].ap_fb.setB(rev[0].ap_fb.b);
            break;

         case VST2_PARAM_APFB_C:
            rev[0].ap_fb.setC((_value - 0.5f) * 2.0f);
            rev[1].ap_fb.setC(rev[0].ap_fb.c);
            break;

         case VST2_PARAM_APFB_NOTE:
            rev[0].ap_fb.setNote(_value * 100.0f);
            rev[1].ap_fb.setNote(rev[0].ap_fb.note);
            rev[0].ap_fb.handleParamsUpdated();
            rev[1].ap_fb.handleParamsUpdated();
            break;

         case VST2_PARAM_CB1_FBDCY:
            rev[0].cb_1.setFbDcy((_value - 0.5f) * 2.0f);
            rev[1].cb_1.setFbDcy(rev[0].cb_1.fb_dcy);
            break;

         case VST2_PARAM_CB2_FBDCY:
            rev[0].cb_2.setFbDcy((_value - 0.5f) * 2.0f);
            rev[1].cb_2.setFbDcy(rev[0].cb_2.fb_dcy);
            break;

         case VST2_PARAM_CB3_FBDCY:
            rev[0].cb_3.setFbDcy((_value - 0.5f) * 2.0f);
            rev[1].cb_3.setFbDcy(rev[0].cb_3.fb_dcy);
            break;

         case VST2_PARAM_DLYX_FBDCY:
            rev[0].dly_x.setFbDcy((_value - 0.5f) * 2.0f);
            rev[1].dly_x.setFbDcy(rev[0].dly_x.fb_dcy);
            break;

         case VST2_PARAM_X1_AMT:
            x1_amt = _value;
            break;

         case VST2_PARAM_X2_AMT:
            x2_amt = _value;
            break;

         case VST2_PARAM_X3_AMT:
            x3_amt = _value;
            break;

         case VST2_PARAM_X4_AMT:
            x4_amt = _value;
            break;

         case VST2_PARAM_WET_GAIN:
            wet_gain = _value * 2.0f;
            break;

         case VST2_PARAM_CB_MIX:
            comb_mix = (_value - 0.5f) * 2.0f;
            break;

         case VST2_PARAM_CLIP_LVL:
            clip_lvl = _value * 10.0f;
            break;

         case VST2_PARAM_DLY_LSPD:
            dly_lerp_spd = _value;
            rev[0].setDlyLerpSpdNorm(dly_lerp_spd);
            rev[1].setDlyLerpSpdNorm(dly_lerp_spd);
            break;

         case VST2_PARAM_FCBMOD:
            rev[0].cb_1.setFreqModNorm(_value);
            rev[0].cb_2.setFreqModNorm(_value);
            rev[0].cb_3.setFreqModNorm(_value);

            rev[1].cb_1.setFreqModNorm(_value);
            rev[1].cb_2.setFreqModNorm(_value);
            rev[1].cb_3.setFreqModNorm(_value);

            rev[0].cb_1.handleParamsUpdated();
            rev[0].cb_2.handleParamsUpdated();
            rev[0].cb_3.handleParamsUpdated();

            rev[1].cb_1.handleParamsUpdated();
            rev[1].cb_2.handleParamsUpdated();
            rev[1].cb_3.handleParamsUpdated();
            break;

         case VST2_PARAM_FAPMOD:
            rev[0].ap_1.setFreqModNorm(_value);
            rev[0].ap_2.setFreqModNorm(_value);
            rev[0].ap_fb.setFreqModNorm(_value);

            rev[1].ap_1.setFreqModNorm(_value);
            rev[1].ap_2.setFreqModNorm(_value);
            rev[1].ap_fb.setFreqModNorm(_value);

            rev[0].ap_1.handleParamsUpdated();
            rev[0].ap_2.handleParamsUpdated();
            rev[0].ap_fb.handleParamsUpdated();

            rev[1].ap_1.handleParamsUpdated();
            rev[1].ap_2.handleParamsUpdated();
            rev[1].ap_fb.handleParamsUpdated();
            break;

         case VST2_PARAM_FDLYMOD:
            rev[0].dly_x.setFreqModNorm(_value);
            rev[1].dly_x.setFreqModNorm(_value);

            rev[0].dly_x.handleParamsUpdated();
            rev[1].dly_x.handleParamsUpdated();
            break;

         case VST2_PARAM_FBMOD:
            rev[0].dly_x.setFbModNorm(_value);
            rev[0].cb_1.setFbModNorm(_value);
            rev[0].cb_2.setFbModNorm(_value);
            rev[0].cb_3.setFbModNorm(_value);

            rev[1].dly_x.setFbModNorm(_value);
            rev[1].cb_1.setFbModNorm(_value);
            rev[1].cb_2.setFbModNorm(_value);
            rev[1].cb_3.setFbModNorm(_value);
            break;

         case VST2_PARAM_BUFSZCB:
            rev[0].cb_1.setDlyBufSizeNorm(_value);
            rev[0].cb_2.setDlyBufSizeNorm(_value);
            rev[0].cb_3.setDlyBufSizeNorm(_value);

            rev[1].cb_1.setDlyBufSizeNorm(_value);
            rev[1].cb_2.setDlyBufSizeNorm(_value);
            rev[1].cb_3.setDlyBufSizeNorm(_value);
            break;

         case VST2_PARAM_BUFSZDLY:
            rev[0].dly_x.setDlyBufSizeNorm(_value);
            rev[1].dly_x.setDlyBufSizeNorm(_value);
            break;

         case VST2_PARAM_LINEAR:
            rev[0].cb_1.b_linear  = (_value >= 0.5f);
            rev[0].cb_2.b_linear  = (_value >= 0.5f);
            rev[0].cb_3.b_linear  = (_value >= 0.5f);
            rev[0].dly_x.b_linear = (_value >= 0.5f);

            rev[1].cb_1.b_linear  = (_value >= 0.5f);
            rev[1].cb_2.b_linear  = (_value >= 0.5f);
            rev[1].cb_3.b_linear  = (_value >= 0.5f);
            rev[1].dly_x.b_linear = (_value >= 0.5f);
            break;

         case VST2_PARAM_SWAYCBF:
            rev[0].cb_1.sway_freq.setScaleA(_value);
            rev[0].cb_2.sway_freq.setScaleA(_value);
            rev[0].cb_3.sway_freq.setScaleA(_value);

            rev[1].cb_1.sway_freq.setScaleA(_value);
            rev[1].cb_2.sway_freq.setScaleA(_value);
            rev[1].cb_3.sway_freq.setScaleA(_value);
            break;

         case VST2_PARAM_SWAYCBM:
            rev[0].cb_1.sway_freq.setMinMaxT(_value, rev[0].cb_1.sway_freq.getMaxT());
            rev[0].cb_2.sway_freq.setMinMaxT(_value, rev[0].cb_2.sway_freq.getMaxT());
            rev[0].cb_3.sway_freq.setMinMaxT(_value, rev[0].cb_3.sway_freq.getMaxT());

            rev[1].cb_1.sway_freq.setMinMaxT(_value, rev[1].cb_1.sway_freq.getMaxT());
            rev[1].cb_2.sway_freq.setMinMaxT(_value, rev[1].cb_2.sway_freq.getMaxT());
            rev[1].cb_3.sway_freq.setMinMaxT(_value, rev[1].cb_3.sway_freq.getMaxT());
            break;

         case VST2_PARAM_SWAYCBX:
            rev[0].cb_1.sway_freq.setMinMaxT(rev[0].cb_1.sway_freq.getMinT(), _value);
            rev[0].cb_2.sway_freq.setMinMaxT(rev[0].cb_2.sway_freq.getMaxT(), _value);
            rev[0].cb_3.sway_freq.setMinMaxT(rev[0].cb_3.sway_freq.getMaxT(), _value);
               
            rev[1].cb_1.sway_freq.setMinMaxT(rev[1].cb_1.sway_freq.getMaxT(), _value);
            rev[1].cb_2.sway_freq.setMinMaxT(rev[1].cb_2.sway_freq.getMaxT(), _value);
            rev[1].cb_3.sway_freq.setMinMaxT(rev[1].cb_3.sway_freq.getMaxT(), _value);
            break;

         case VST2_PARAM_SWAYAPF:
            rev[0].ap_1.sway_freq.setScaleA(_value);
            rev[0].ap_2.sway_freq.setScaleA(_value);
            rev[0].ap_fb.sway_freq.setScaleA(_value);

            rev[1].ap_1.sway_freq.setScaleA(_value);
            rev[1].ap_2.sway_freq.setScaleA(_value);
            rev[1].ap_fb.sway_freq.setScaleA(_value);
            break;

         case VST2_PARAM_SWAYAPM:
            rev[0].ap_1.sway_freq.setMinMaxT(_value, rev[0].ap_1.sway_freq.getMaxT());
            rev[0].ap_2.sway_freq.setMinMaxT(_value, rev[0].ap_2.sway_freq.getMaxT());
            rev[0].ap_fb.sway_freq.setMinMaxT(_value, rev[0].ap_fb.sway_freq.getMaxT());

            rev[1].ap_1.sway_freq.setMinMaxT(_value, rev[1].ap_1.sway_freq.getMaxT());
            rev[1].ap_2.sway_freq.setMinMaxT(_value, rev[1].ap_2.sway_freq.getMaxT());
            rev[1].ap_fb.sway_freq.setMinMaxT(_value, rev[1].ap_fb.sway_freq.getMaxT());
            break;

         case VST2_PARAM_SWAYAPX:
            rev[0].ap_1.sway_freq.setMinMaxT(rev[0].ap_1.sway_freq.getMinT(), _value);
            rev[0].ap_2.sway_freq.setMinMaxT(rev[0].ap_2.sway_freq.getMaxT(), _value);
            rev[0].ap_fb.sway_freq.setMinMaxT(rev[0].ap_fb.sway_freq.getMaxT(), _value);
               
            rev[1].ap_1.sway_freq.setMinMaxT(rev[1].ap_1.sway_freq.getMaxT(), _value);
            rev[1].ap_2.sway_freq.setMinMaxT(rev[1].ap_2.sway_freq.getMaxT(), _value);
            rev[1].ap_fb.sway_freq.setMinMaxT(rev[1].ap_fb.sway_freq.getMaxT(), _value);
            break;

         case VST2_PARAM_SWAYDLYF:
            rev[0].dly_x.sway_freq.setScaleA(_value);
            rev[1].dly_x.sway_freq.setScaleA(_value);
            break;

         case VST2_PARAM_SWAYDLYM:
            rev[0].dly_x.sway_freq.setMinMaxT(_value, rev[0].dly_x.sway_freq.getMaxT());
            rev[1].dly_x.sway_freq.setMinMaxT(_value, rev[1].dly_x.sway_freq.getMaxT());
            break;

         case VST2_PARAM_SWAYDLYX:
            rev[0].dly_x.sway_freq.setMinMaxT(rev[0].dly_x.sway_freq.getMinT(), _value);
            rev[1].dly_x.sway_freq.setMinMaxT(rev[1].dly_x.sway_freq.getMinT(), _value);
            break;

         case VST2_PARAM_SWAYFB:
            rev[0].cb_1.sway_fb.setScaleA(_value);
            rev[0].cb_2.sway_fb.setScaleA(_value);
            rev[0].cb_3.sway_fb.setScaleA(_value);

            rev[1].cb_1.sway_fb.setScaleA(_value);
            rev[1].cb_2.sway_fb.setScaleA(_value);
            rev[1].cb_3.sway_fb.setScaleA(_value);
            break;

         case VST2_PARAM_PRIME:
            rev[0].setPrimeTbl( (_value >= 0.5f) ? prime_lock.prime_tbl : NULL );
            rev[1].setPrimeTbl( (_value >= 0.5f) ? prime_lock.prime_tbl : NULL );
            break;

         case VST2_PARAM_OUT_LPF:
            out_lpf = _value;
            break;

         case VST2_PARAM_CB_MIX_IN:
            comb_mix_in = (_value - 0.5f) * 2.0f;
            break;

         case VST2_PARAM_OUT_LVL:
            out_lvl = _value * 2.0f;
            break;
      }
   }

   sF32 getParam(sSI _index) {
      sF32 r = 0.0f;
      switch(_index)
      {
         default:
         case VST2_PARAM_IN_AMT:
            r = in_amt;
            break;

         case VST2_PARAM_IN_HPF:
            r = in_hpf;
            break;

         case VST2_PARAM_WET_AMT:
            r = wet_amt;
            break;

         case VST2_PARAM_FREQSCLL:
            r = rev[0].cb_1.freq_scl;
            break;

         case VST2_PARAM_FREQSCLR:
            r = rev[1].cb_1.freq_scl;
            break;

         case VST2_PARAM_CB1_NOTE:
            r = (rev[0].cb_1.note / 100.0f);
            break;

         case VST2_PARAM_CB2_NOTE:
            r = (rev[0].cb_2.note / 100.0f);
            break;

         case VST2_PARAM_CB3_NOTE:
            r = (rev[0].cb_3.note / 100.0f);
            break;

         case VST2_PARAM_DLYX_NOTE:
            r = (rev[0].dly_x.note / 100.0f);
            break;

         case VST2_PARAM_CB1_FBAMT:
            r = rev[0].cb_1.fb_amt;
            break;

         case VST2_PARAM_CB2_FBAMT:
            r = rev[0].cb_2.fb_amt;
            break;

         case VST2_PARAM_CB3_FBAMT:
            r = rev[0].cb_3.fb_amt;
            break;

         case VST2_PARAM_DLYX_FBAMT:
            r = rev[0].dly_x.fb_amt;
            break;

         case VST2_PARAM_AP1_B:
            r = (rev[0].ap_1.b * 0.5f) + 0.5f;
            break;

         case VST2_PARAM_AP1_C:
            r = (rev[0].ap_1.c * 0.5f) + 0.5f;
            break;

         case VST2_PARAM_AP1_NOTE:
            r = (rev[0].ap_1.note / 100.0f);
            break;

         case VST2_PARAM_AP2_B:
            r = (rev[0].ap_2.b * 0.5f) + 0.5f;
            break;

         case VST2_PARAM_AP2_C:
            r = (rev[0].ap_2.c * 0.5f) + 0.5f;
            break;

         case VST2_PARAM_AP2_NOTE:
            r = (rev[0].ap_2.note / 100.0f);
            break;

         case VST2_PARAM_APFB_B:
            r = (rev[0].ap_fb.b * 0.5f) + 0.5f;
            break;

         case VST2_PARAM_APFB_C:
            r = (rev[0].ap_fb.c * 0.5f) + 0.5f;
            break;

         case VST2_PARAM_APFB_NOTE:
            r = (rev[0].ap_fb.note / 100.0f);
            break;

         case VST2_PARAM_CB1_FBDCY:
            r = (rev[0].cb_1.fb_dcy * 0.5f) + 0.5f;
            break;

         case VST2_PARAM_CB2_FBDCY:
            r = (rev[0].cb_2.fb_dcy * 0.5f) + 0.5f;
            break;

         case VST2_PARAM_CB3_FBDCY:
            r = (rev[0].cb_3.fb_dcy * 0.5f) + 0.5f;
            break;

         case VST2_PARAM_DLYX_FBDCY:
            r = (rev[0].dly_x.fb_dcy * 0.5f) + 0.5f;
            break;

         case VST2_PARAM_X1_AMT:
            r = x1_amt;
            break;

         case VST2_PARAM_X2_AMT:
            r = x2_amt;
            break;

         case VST2_PARAM_X3_AMT:
            r = x3_amt;
            break;

         case VST2_PARAM_X4_AMT:
            r = x4_amt;
            break;

         case VST2_PARAM_WET_GAIN:
            r = wet_gain * 0.5f;
            break;

         case VST2_PARAM_CB_MIX:
            r = comb_mix * 0.5f + 0.5f;
            break;

         case VST2_PARAM_CLIP_LVL:
            r = clip_lvl / 10.0f;
            break;

         case VST2_PARAM_DLY_LSPD:
            r = dly_lerp_spd;
            break;

         case VST2_PARAM_FCBMOD:
            r = rev[0].cb_1.getFreqModNorm();
            break;

         case VST2_PARAM_FAPMOD:
            r = rev[0].ap_1.getFreqModNorm();
            break;

         case VST2_PARAM_FDLYMOD:
            r = rev[0].dly_x.getFreqModNorm();
            break;

         case VST2_PARAM_FBMOD:
            r = rev[0].cb_1.getFbModNorm();
            break;

         case VST2_PARAM_BUFSZCB:
            r = rev[0].cb_1.getDlyBufSizeNorm();
            break;

         case VST2_PARAM_BUFSZDLY:
            r = rev[0].dly_x.getDlyBufSizeNorm();
            break;

         case VST2_PARAM_LINEAR:
            r = rev[0].dly_x.b_linear ? 1.0f : 0.0f;
            break;

         case VST2_PARAM_SWAYCBF:
            r = rev[0].cb_1.sway_freq.getScaleA();
            break;

         case VST2_PARAM_SWAYCBM:
            r = rev[0].cb_1.sway_freq.getMinT();
            break;

         case VST2_PARAM_SWAYCBX:
            r = rev[0].cb_1.sway_freq.getMaxT();
            break;

         case VST2_PARAM_SWAYAPF:
            r = rev[0].ap_1.sway_freq.getScaleA();
            break;

         case VST2_PARAM_SWAYAPM:
            r = rev[0].ap_1.sway_freq.getMinT();
            break;

         case VST2_PARAM_SWAYAPX:
            r = rev[0].ap_1.sway_freq.getMaxT();
            break;

         case VST2_PARAM_SWAYDLYF:
            r = rev[0].dly_x.sway_freq.getScaleA();
            break;

         case VST2_PARAM_SWAYDLYM:
            r = rev[0].dly_x.sway_freq.getMinT();
            break;

         case VST2_PARAM_SWAYDLYX:
            r = rev[0].dly_x.sway_freq.getMaxT();
            break;

         case VST2_PARAM_SWAYFB:
            r = rev[0].cb_1.sway_fb.getScaleA();
            break;

         case VST2_PARAM_PRIME:
            r = (NULL != rev[0].cb_1.prime_tbl) ? 1.0f : 0.0f;
            break;

         case VST2_PARAM_OUT_LPF:
            r = out_lpf;
            break;

         case VST2_PARAM_CB_MIX_IN:
            r = comb_mix_in * 0.5f + 0.5f;
            break;

         case VST2_PARAM_OUT_LVL:
            r = out_lvl * 0.5f;
            break;
      }
      return r;
   }

   sBool getParameterProperties(sUI _index, struct VstParameterProperties *_ret) {
      sBool r = 0;

      ::memset((void*)_ret, 0, sizeof(struct VstParameterProperties));

      if(_index < VST2_MAX_PARAM_IDS)
      {
         _ret->stepFloat = 0.001f;
         _ret->smallStepFloat = 0.001f;
         _ret->largeStepFloat = 0.01f;
         _ret->flags = 0;
         _ret->displayIndex = VstInt16(_index);
         _ret->category = 0; // 0=no category
         _ret->numParametersInCategory = 0;

         getParamName(_index, _ret->label, kVstMaxLabelLen);
         getParamName(_index, _ret->shortLabel, kVstMaxShortLabelLen);

         r = 1;
      }

      return r;
   }

   void handleUIParam(int uniqueParamId, float normValue) {
      if(NULL != _vstHostCallback)
         (void)_vstHostCallback(&_vstPlugin, audioMasterAutomate, uniqueParamId, 0/*value*/, NULL/*ptr*/, normValue/*opt*/);
   }

   void getTimingInfo(int *_retPlaying, float *_retBPM, float *_retSongPosPPQ) {
      *_retPlaying = 0;

      if(NULL != _vstHostCallback)
      {
         VstIntPtr result = _vstHostCallback(&_vstPlugin, audioMasterGetTime, 0,
                                             (kVstTransportPlaying | kVstTempoValid | kVstPpqPosValid)/*value=requestmask*/,
                                             NULL/*ptr*/, 0.0f/*opt*/
                                             );
         if(0 != result)
         {
            const struct VstTimeInfo *timeInfo = (const struct VstTimeInfo *)result;

            *_retPlaying = (0 != (timeInfo->flags & kVstTransportPlaying));

            if(0 != (timeInfo->flags & kVstTempoValid))
            {
               *_retBPM = float(timeInfo->tempo);
            }

            if(0 != (timeInfo->flags & kVstPpqPosValid))
            {
               *_retSongPosPPQ = (float)timeInfo->ppqPos;
            }
         }
      }
   }

private:
   // the host callback (a function pointer)
   VSTHostCallback _vstHostCallback;

   // the actual structure required by the host
   VSTPlugin _vstPlugin;
};

sSI VSTPluginWrapper::instance_count = 0;



/*******************************************
 * Callbacks: Host -> Plugin
 *
 * Defined here because they are used in the rest of the code later
 */

/**
 * This is the callback that will be called to process the samples in the case of single precision. This is where the
 * meat of the logic happens!
 *
 * @param vstPlugin the object returned by VSTPluginMain
 * @param inputs an array of array of input samples. You read from it. First dimension is for inputs, second dimension is for samples: inputs[numInputs][sampleFrames]
 * @param outputs an array of array of output samples. You write to it. First dimension is for outputs, second dimension is for samples: outputs[numOuputs][sampleFrames]
 * @param sampleFrames the number of samples (second dimension in both arrays)
 */
void VSTPluginProcessReplacingFloat32(VSTPlugin *vstPlugin,
                                      float    **_inputs,
                                      float    **_outputs,
                                      VstInt32   sampleFrames
                                      ) {
   if(sUI(sampleFrames) > VSTPluginWrapper::MAX_BLOCK_SIZE)
      return;  // should not be reachable

   // we can get a hold to our C++ class since we stored it in the `object` field (see constructor)
   VSTPluginWrapper *wrapper = static_cast<VSTPluginWrapper *>(vstPlugin->object);
   // Dprintf("xxx vstschroeder_plugin: VSTPluginProcessReplacingFloat32: ENTER\n");

   // vst2_handle_queued_params();
   
   wrapper->lockAudio();

   // if(wrapper->b_check_offline)
   // {
   //    // Check if offline rendering state changed and update resampler when necessary
   //    wrapper->checkOffline();
   // }

#ifdef HAVE_WINDOWS
   _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
#endif // HAVE_WINDOWS

#ifdef YAC_LINUX
   fesetround(FE_TOWARDZERO);
#endif // YAC_LINUX

   sUI chIdx;

   if(wrapper->b_processing)
   {
      wrapper->process(_inputs, _outputs, sUI(sampleFrames));
   }
   else
   {
      // Idle, clear output buffers
      for(chIdx = 0u; chIdx < NUM_OUTPUTS; chIdx++)
      {
         ::memset((void*)_outputs[chIdx], 0, sizeof(sF32)*sampleFrames);
      }
   }

   wrapper->unlockAudio();

   //Dprintf("xxx vstschroeder_plugin: VSTPluginProcessReplacingFloat32: LEAVE\n");
}



#ifdef VST2_OPCODE_DEBUG
static const char *vst2_opcode_names[] = {
	"effOpen",
	"effClose",
	"effSetProgram",
	"effGetProgram",
	"effSetProgramName",
	"effGetProgramName",
   "effGetParamLabel",
	"effGetParamDisplay",
	"effGetParamName",
	"DEPR_effGetVu",
	"effSetSampleRate",
	"effSetBlockSize",
	"effMainsChanged",
	"effEditGetRect",
	"effEditOpen",
	"effEditClose",
	"DEPR_effEditDraw",
	"DEPR_effEditMouse",
	"DEPR_effEditKey",
	"effEditIdle",
   "DEPR_effEditTop",
	"DEPR_effEditSleep",
	"DEPR_effIdentify",
   "effGetChunk",
	"effSetChunk",
   // extended:
	"effProcessEvents",
	"effCanBeAutomated",
	"effString2Parameter",
	"effGetNumProgramCategories",
	"effGetProgramNameIndexed",
   "DEPR_effCopyProgram",
	"DEPR_effConnectInput",
	"DEPR_effConnectOutput",
   "effGetInputProperties",
	"effGetOutputProperties",
	"effGetPlugCategory",
	"DEPR_effGetCurrentPosition",
	"DEPR_effGetDestinationBuffer",
	"effOfflineNotify",
	"effOfflinePrepare",
	"effOfflineRun",
	"effProcessVarIo",
	"effSetSpeakerArrangement",
	"DEPR_effSetBlockSizeAndSampleRate",
	"effSetBypass",
	"effGetEffectName",
	"DEPR_effGetErrorText",
	"effGetVendorString",
	"effGetProductString",
	"effGetVendorVersion",
	"effVendorSpecific",
	"effCanDo",
	"effGetTailSize",
	"DEPR_effIdle",
	"DEPR_effGetIcon",
	"DEPR_effSetViewPosition",
	"effGetParameterProperties",
	"DEPR_effKeysRequired",
	"effGetVstVersion",
	"effEditKeyDown",
	"effEditKeyUp",
	"effSetEditKnobMode",
	"effGetMidiProgramName",
	"effGetCurrentMidiProgram",
	"effGetMidiProgramCategory",
	"effHasMidiProgramsChanged",
	"effGetMidiKeyName",
   "effBeginSetProgram",
	"effEndSetProgram",
	"effGetSpeakerArrangement",
	"effShellGetNextPlugin",
	"effStartProcess",
	"effStopProcess",
	"effSetTotalSampleToProcess",
	"effSetPanLaw",
   "effBeginLoadBank",
	"effBeginLoadProgram",
	"effSetProcessPrecision",
	"effGetNumMidiInputChannels",
	"effGetNumMidiOutputChannels",
};
#endif // VST2_OPCODE_DEBUG

/**
 * This is the plugin called by the host to communicate with the plugin, mainly to request information (like the
 * vendor string, the plugin category...) or communicate state/changes (like open/close, frame rate...)
 *
 * @param vstPlugin the object returned by VSTPluginMain
 * @param opCode defined in aeffect.h/AEffectOpcodes and which continues in aeffectx.h/AEffectXOpcodes for a grand
 *        total of 79 of them! Only a few of them are implemented in this small plugin.
 * @param index depend on the opcode
 * @param value depend on the opcode
 * @param ptr depend on the opcode
 * @param opt depend on the opcode
 * @return depend on the opcode (0 is ok when you don't implement an opcode...)
 */
VstIntPtr VSTPluginDispatcher(VSTPlugin *vstPlugin,
                              VstInt32   opCode,
                              VstInt32   index,
                              VstIntPtr  value,
                              void      *ptr,
                              float      opt
                              ) {
#ifdef VST2_OPCODE_DEBUG
   Dprintf("vstschroeder_plugin: called VSTPluginDispatcher(%d) (%s)\n", opCode, vst2_opcode_names[opCode]);
#else
   // Dprintf("vstschroeder_plugin: called VSTPluginDispatcher(%d)\n", opCode);
#endif // VST2_OPCODE_DEBUG

   VstIntPtr r = 0;

   // we can get a hold to our C++ class since we stored it in the `object` field (see constructor)
   VSTPluginWrapper *wrapper = static_cast<VSTPluginWrapper *>(vstPlugin->object);

   // see aeffect.h/AEffectOpcodes and aeffectx.h/AEffectXOpcodes for details on all of them
   switch(opCode)
   {
      case effGetPlugCategory:
         // request for the category of the plugin: in this case it is an effect since it is modifying the input (as opposed
         // to generating sound)
// #ifdef VST2_EFFECT
         return kPlugCategEffect;
// #else
//          return kPlugCategSynth;
// #endif // VST2_EFFECT

      case effOpen:
         // called by the host after it has obtained the effect instance (but _not_ during plugin scans)
         //  (note) any heavy-lifting init code should go here
         Dprintf("vstschroeder_plugin<dispatcher>: effOpen\n");
         r = wrapper->openEffect();
         break;

      case effClose:
         // called by the host when the plugin was called... time to reclaim memory!
         wrapper->closeEffect();
         // (note) hosts usually call effStopProcess before effClose
         delete wrapper;
         break;

      case effSetProgram:
         r = 1;
         break;

      case effGetProgram:
         r = 0;
         break;

      case effGetVendorString:
         // request for the vendor string (usually used in the UI for plugin grouping)
         ::strncpy(static_cast<char *>(ptr), "bsp", kVstMaxVendorStrLen);
         r = 1;
         break;

      case effGetVendorVersion:
         // request for the version
         return PLUGIN_VERSION;

      case effVendorSpecific:
         break;

      case effGetVstVersion:
          r = kVstVersion;
          break;

      case effGetEffectName:
         ::strncpy((char*)ptr, "SchroederVerb v8", kVstMaxEffectNameLen);
         r = 1;
         break;

      case effGetProductString:
         ::strncpy((char*)ptr, "SchroederVerb v8", kVstMaxProductStrLen);
         r = 1;
         break;

      case effGetNumMidiInputChannels:
         r = 0;
         break;

      case effGetNumMidiOutputChannels:
         r = 0;
         break;

      case effGetInputProperties:
         {
            VstPinProperties *pin = (VstPinProperties*)ptr;
            Dyac_snprintf(pin->label, kVstMaxLabelLen, "Input #%d", index);
            pin->flags           = kVstPinIsActive | ((0 == (index & 1)) ? kVstPinIsStereo : 0);
            pin->arrangementType = ((0 == (index & 1)) ? kSpeakerArrStereo : kSpeakerArrMono);
            Dyac_snprintf(pin->shortLabel, kVstMaxShortLabelLen, "in%d", index);
            memset((void*)pin->future, 0, 48);
            r = 1;
         }
         break;

      case effGetOutputProperties:
         {
            VstPinProperties *pin = (VstPinProperties*)ptr;
            ::snprintf(pin->label, kVstMaxLabelLen, "Output #%d", index);
            pin->flags           = kVstPinIsActive | ((0 == (index & 1)) ? kVstPinIsStereo : 0);
            pin->arrangementType = ((0 == (index & 1)) ? kSpeakerArrStereo : kSpeakerArrMono);
            ::snprintf(pin->shortLabel, kVstMaxShortLabelLen, "out%d", index);
            memset((void*)pin->future, 0, 48);
            r = 1;
         }
         break;

      case effSetSampleRate:
         r = wrapper->setSampleRate(opt) ? 1 : 0;
         break;

      case effSetBlockSize:
         r = wrapper->setBlockSize(sUI(value)) ? 1 : 0;
         break;

      case effCanDo:
         // ptr:
         // "sendVstEvents"
         // "sendVstMidiEvent"
         // "sendVstTimeInfo"
         // "receiveVstEvents"
         // "receiveVstMidiEvent"
         // "receiveVstTimeInfo"
         // "offline"
         // "plugAsChannelInsert"
         // "plugAsSend"
         // "mixDryWet"
         // "noRealTime"
         // "multipass"
         // "metapass"
         // "1in1out"
         // "1in2out"
         // "2in1out"
         // "2in2out"
         // "2in4out"
         // "4in2out"
         // "4in4out"
         // "4in8out"
         // "8in4out"
         // "8in8out"
         // "midiProgramNames"
         // "conformsToWindowRules"
#ifdef VST2_OPCODE_DEBUG
         printf("xxx effCanDo \"%s\" ?\n", (char*)ptr);
#endif // VST2_OPCODE_DEBUG
         if(!strcmp((char*)ptr, "receiveVstEvents"))
            r = 0;
         else if(!strcmp((char*)ptr, "receiveVstMidiEvent"))  // (note) required by Jeskola Buzz
            r = 0;
         else
            r = 0;
         break;

      case effGetProgramName:
         Dyac_snprintf((char*)ptr, kVstMaxProgNameLen, "default");
         r = 1;
         break;

      case effSetProgramName:
         r = 1;
         break;

      case effGetProgramNameIndexed:
         Dyac_snprintf((char*)ptr, kVstMaxProgNameLen, "default");
         r = 1;
         break;

      case effGetParamName:
         // kVstMaxParamStrLen(8), much longer in other plugins
         // printf("xxx vstschroeder_plugin: effGetParamName: ptr=%p\n", ptr);
         wrapper->getParamName(index, (char*)ptr, kVstMaxParamStrLen);
         r = 1;
         break;

      case effCanBeAutomated:
         // fix Propellerhead Reason VST parameter support
         r = 1;
         break;

      case effGetParamLabel:
         // e.g. "dB"
         break;

      case effGetParamDisplay:
         // e.g. "-20"
         break;

#if 0
      case effGetParameterProperties:
         // [index]: parameter index [ptr]: #VstParameterProperties* [return value]: 1 if supported
         wrapper->setGlobals();
         r = wrapper->getParameterProperties(sUI(index), (struct VstParameterProperties*)ptr);
         break;
#endif

      case effGetChunk:
         // // Query bank (index=0) or program (index=1) state
         // //  value: 0
         // //    ptr: buffer address
         // //      r: buffer size
         // Dprintf("xxx vstschroeder_plugin: effGetChunk index=%d ptr=%p\n", index, ptr);
         // // // if(0 == index)
         // // // {
         // // //    r = wrapper->getBankChunk((sU8**)ptr);
         // // // }
         // // // else
         // // // {
         //    r = wrapper->getProgramChunk((sU8**)ptr);
         // // // }
         break;

      case effSetChunk:
         // // Restore bank (index=0) or program (index=1) state
         // //  value: buffer size
         // //    ptr: buffer address
         // //      r: 1
         // Dprintf("xxx vstschroeder_plugin: effSetChunk index=%d size=%d ptr=%p\n", index, (int)value, ptr);
         // // // if(0 == index)
         // // // {
         // // //    r = wrapper->setBankChunk(size_t(value), (sU8*)ptr) ? 1 : 0;
         // // // }
         // // // else
         // // // {
         //    r = wrapper->setProgramChunk(size_t(value), (sU8*)ptr) ? 1 : 0;
         // // // }
         // Dprintf("xxx vstschroeder_plugin: effSetChunk LEAVE\n");
         break;

      case effShellGetNextPlugin:
         // For shell plugins (e.g. Waves), returns next sub-plugin UID (or 0)
         //  (note) plugin uses audioMasterCurrentId while it's being instantiated to query the currently selected sub-plugin
         //          if the host returns 0, it will then call effShellGetNextPlugin to enumerate the sub-plugins
         //  ptr: effect name string ptr (filled out by the plugin)
         r = 0;
         break;

      case effMainsChanged:
         // value = 0=suspend, 1=resume
         wrapper->setEnableProcessingActive((value > 0) ? true : false);
         r = 1;
         break;

      case effStartProcess:
         wrapper->setEnableProcessingActive(true);
         r = 1;
         break;

      case effStopProcess:
         wrapper->setEnableProcessingActive(false);
         r = 1;
         break;

      case effProcessEvents:
//          // ptr: VstEvents*
//          {
//             VstEvents *events = (VstEvents*)ptr;
//             // Dprintf("vstschroeder_plugin:effProcessEvents: recvd %d events", events->numEvents);
//             VstEvent**evAddr = &events->events[0];

//             if(events->numEvents > 0)
//             {
//                wrapper->setGlobals();
//                wrapper->mtx_mididev.lock();
            
//                for(sUI evIdx = 0u; evIdx < sUI(events->numEvents); evIdx++, evAddr++)
//                {
//                   VstEvent *ev = *evAddr;

//                   if(NULL != ev)  // paranoia
//                   {
// #ifdef DEBUG_PRINT_EVENTS
//                      Dprintf("vstschroeder_plugin:effProcessEvents: ev[%u].byteSize    = %u\n", evIdx, sUI(ev->byteSize));  // sizeof(VstMidiEvent) = 32
//                      Dprintf("vstschroeder_plugin:effProcessEvents: ev[%u].deltaFrames = %u\n", evIdx, sUI(ev->deltaFrames));
// #endif // DEBUG_PRINT_EVENTS

//                      switch(ev->type)
//                      {
//                         default:
//                            //case kVstAudioType:      // deprecated
//                            //case kVstVideoType:      // deprecated
//                            //case kVstParameterType:  // deprecated
//                            //case kVstTriggerType:    // deprecated
//                            break;

//                         case kVstMidiType:
//                            // (note) ev->data stores the actual payload (up to 16 bytes)
//                            // (note) e.g. 0x90 0x30 0x7F for a C-4 note-on on channel 1 with velocity 127
//                            // (note) don't forget to use a mutex (lockAudio(), unlockAudio()) when modifying the audio processor state!
//                         {
//                            VstMidiEvent *mev = (VstMidiEvent *)ev;
// #ifdef DEBUG_PRINT_EVENTS
//                            Dprintf("vstschroeder_plugin:effProcessEvents<midi>: ev[%u].noteLength      = %u\n", evIdx, sUI(mev->noteLength));  // #frames
//                            Dprintf("vstschroeder_plugin:effProcessEvents<midi>: ev[%u].noteOffset      = %u\n", evIdx, sUI(mev->noteOffset));  // #frames
//                            Dprintf("vstschroeder_plugin:effProcessEvents<midi>: ev[%u].midiData        = %02x %02x %02x %02x\n", evIdx, sU8(mev->midiData[0]), sU8(mev->midiData[1]), sU8(mev->midiData[2]), sU8(mev->midiData[3]));
//                            Dprintf("vstschroeder_plugin:effProcessEvents<midi>: ev[%u].detune          = %d\n", evIdx, mev->detune); // -64..63
//                            Dprintf("vstschroeder_plugin:effProcessEvents<midi>: ev[%u].noteOffVelocity = %d\n", evIdx, mev->noteOffVelocity); // 0..127
// #endif // DEBUG_PRINT_EVENTS

//                            if(VSTPluginWrapper::IDLE_DETECT_MIDI == wrapper->getCurrentIdleDetectMode())
//                            {
//                               if(0x90u == (mev->midiData[0] & 0xF0u)) // Note on ?
//                               {
//                                  wrapper->lockAudio();
//                                  if(wrapper->b_idle)
//                                  {
//                                     Dprintf_idle("xxx vstschroeder_plugin: become active after MIDI note on\n");
//                                  }
//                                  wrapper->b_idle = false;
//                                  wrapper->idle_output_framecount = 0u;
//                                  wrapper->idle_frames_since_noteon = 0u;
//                                  wrapper->unlockAudio();
//                               }
//                            }

//                            vst2_process_midi_input_event(mev->midiData[0],
//                                                          mev->midiData[1],
//                                                          mev->midiData[2]
//                                                          );

//                         }
//                         break;

//                         case kVstSysExType:
//                         {
// #ifdef DEBUG_PRINT_EVENTS
//                            VstMidiSysexEvent *xev = (VstMidiSysexEvent*)ev;
//                            Dprintf("vstschroeder_plugin:effProcessEvents<syx>: ev[%u].dumpBytes = %u\n", evIdx, sUI(xev->dumpBytes));  // size
//                            Dprintf("vstschroeder_plugin:effProcessEvents<syx>: ev[%u].sysexDump = %p\n", evIdx, xev->sysexDump);            // buffer addr
// #endif // DEBUG_PRINT_EVENTS

//                            // (note) don't forget to use a mutex (lockAudio(), unlockAudio()) when modifying the audio processor state!
//                         }
//                         break;
//                      }
//                   } // if ev
//                } // loop events

//                wrapper->mtx_mididev.unlock();
//             } // if events
//          }
         break;

      case effGetTailSize: // 52
         break;
#if 1
      //case effIdle:
      case 53:
         // Periodic idle call (from UI thread), e.g. at 20ms intervals (depending on host)
         //  (note) deprecated in vst2.4 (but some plugins still rely on this)
         r = 0;
         break;
#endif

      case effEditIdle:
// #ifdef YAC_LINUX
//          // pump event queue (when not using _XEventProc callback)
//          wrapper->events();
// #endif // YAC_LINUX
//          if(0 == wrapper->redraw_ival_ms)
//          {
//             wrapper->queueRedraw();
//          }
         break;

      case effEditGetRect:
         // // Query editor window geometry
         // // ptr: ERect* (on Windows)
         // if(NULL != ptr)
         // {
         //    // ...
         //    printf("xxx vstschroeder_plugin: effEditGetRect: (%d; %d; %d; %d)\n",
         //           wrapper->editor_rect.top,
         //           wrapper->editor_rect.left,
         //           wrapper->editor_rect.bottom,
         //           wrapper->editor_rect.right
         //           );
         //    *(void**)ptr = (void*) &wrapper->editor_rect;
         //    r = 1;
         // }
         // else
         // {
         //    r = 0;
         // }
         break;

#if 0
      case effEditTop:
         // deprecated in vst2.4
         r = 0;
         break;
#endif

      case effEditOpen:
         // // Show editor window
         // // ptr: native window handle (hWnd on Windows)
         // wrapper->openEditor(ptr);
         // r = 1;
         break;

      case effEditClose:
         // // Close editor window
         // wrapper->closeEditor();
         // r = 1;
         break;

      case effEditKeyDown:
         // // [index]: ASCII character [value]: virtual key [opt]: modifiers [return value]: 1 if key used  @see AEffEditor::onKeyDown
         // // (note) only used for touch input
         // // Dprintf("xxx effEditKeyDown: ascii=%d (\'%c\') vkey=0x%08x mod=0x%08x\n", index, index, value, opt);
         // if(rack::b_touchkeyboard_enable)
         // {
         //    wrapper->setGlobals();
         //    {
         //       sUI vkey = 0u;

         //       switch(sUI(value))
         //       {
         //          // see aeffectx.h
         //          case VKEY_BACK:      vkey = LGLW_VKEY_BACKSPACE; break;
         //          case VKEY_TAB:       vkey = LGLW_VKEY_TAB;       break;
         //          case VKEY_RETURN:    vkey = LGLW_VKEY_RETURN;    break;
         //          case VKEY_ESCAPE:    vkey = LGLW_VKEY_ESCAPE;    break;
         //          case VKEY_END:       vkey = LGLW_VKEY_END;       break;
         //          case VKEY_HOME:      vkey = LGLW_VKEY_HOME;      break;
         //          case VKEY_LEFT:      vkey = LGLW_VKEY_LEFT;      break;
         //          case VKEY_UP:        vkey = LGLW_VKEY_UP;        break;
         //          case VKEY_RIGHT:     vkey = LGLW_VKEY_RIGHT;     break;
         //          case VKEY_DOWN:      vkey = LGLW_VKEY_DOWN;      break;
         //          case VKEY_PAGEUP:    vkey = LGLW_VKEY_PAGEUP;    break;
         //          case VKEY_PAGEDOWN:  vkey = LGLW_VKEY_PAGEDOWN;  break;
         //          case VKEY_ENTER:     vkey = LGLW_VKEY_RETURN;    break;
         //          case VKEY_INSERT:    vkey = LGLW_VKEY_INSERT;    break;
         //          case VKEY_DELETE:    vkey = LGLW_VKEY_DELETE;    break;
         //             break;

         //          default:
         //             vkey = (char)index;
         //             // (note) some(most?) DAWs don't send the correct VKEY_xxx values for all special keys
         //             switch(vkey)
         //             {
         //                case 8:  vkey = LGLW_VKEY_BACKSPACE;  break;
         //                   //    case 13: vkey = LGLW_VKEY_RETURN;     break;
         //                   //    case 9:  vkey = LGLW_VKEY_TAB;        break;
         //                   //    case 27: vkey = LGLW_VKEY_ESCAPE;     break;
         //                default:
         //                   if(vkey >= 128)
         //                      vkey = 0u;
         //                   break;
         //             }
         //             break;
         //       }
            
         //       if(vkey > 0u)
         //       {
         //          r = vst2_handle_effeditkeydown(vkey);
         //       }
         //    }
         // }
         break;

      case effGetParameterProperties/*56*/:
      case effGetMidiKeyName/*66*/:
         // (todo) Bitwig (Linux) sends a lot of them
         break;

      default:
         // ignoring all other opcodes
         Dprintf("vstschroeder_plugin:dispatcher: unhandled opCode %d [ignored] \n", opCode);
         break;

   }

   return r;
}


/**
 * Set parameter setting
 */
void VSTPluginSetParameter(VSTPlugin *vstPlugin,
                           VstInt32   index,
                           float      parameter
                           ) {
#ifdef DEBUG_PRINT_PARAMS
   Dprintf("vstschroeder_plugin: called VSTPluginSetParameter(%d, %f)\n", index, parameter);
#endif // DEBUG_PRINT_PARAMS

   // we can get a hold to our C++ class since we stored it in the `object` field (see constructor)
   VSTPluginWrapper *wrapper = static_cast<VSTPluginWrapper *>(vstPlugin->object);

   wrapper->setParam(index, parameter);
}


/**
 * Query parameter
 */
float VSTPluginGetParameter(VSTPlugin *vstPlugin,
                            VstInt32   index
                            ) {
#ifdef DEBUG_PRINT_PARAMS
   Dprintf("vstschroeder_plugin: called VSTPluginGetParameter(%d)\n", index);
#endif // DEBUG_PRINT_PARAMS
   // we can get a hold to our C++ class since we stored it in the `object` field (see constructor)
   VSTPluginWrapper *wrapper = static_cast<VSTPluginWrapper *>(vstPlugin->object);

   float r = wrapper->getParam(index);

   return r;
}


/**
 * Main constructor for our C++ class
 */
VSTPluginWrapper::VSTPluginWrapper(audioMasterCallback vstHostCallback,
                                   VstInt32 vendorUniqueID,
                                   VstInt32 vendorVersion,
                                   VstInt32 numParams,
                                   VstInt32 numPrograms,
                                   VstInt32 numInputs,
                                   VstInt32 numOutputs
                                   ) : _vstHostCallback(vstHostCallback)
{
   instance_count++;

   // Make sure that the memory is properly initialized
   memset(&_vstPlugin, 0, sizeof(_vstPlugin));

   // this field must be set with this constant...
   _vstPlugin.magic = kEffectMagic;

   // storing this object into the VSTPlugin so that it can be retrieved when called back (see callbacks for use)
   _vstPlugin.object = this;

   // specifying that we handle both single and NOT double precision (there are other flags see aeffect.h/VstAEffectFlags)
   _vstPlugin.flags = 
// #ifndef VST2_EFFECT
//       effFlagsIsSynth                  |
// #endif
      effFlagsCanReplacing             | 
      // (effFlagsCanDoubleReplacing & 0) |
      0;
      // effFlagsProgramChunks            |
      // effFlagsHasEditor                ;

   // initializing the plugin with the various values
   _vstPlugin.uniqueID    = vendorUniqueID;
   _vstPlugin.version     = vendorVersion;
   _vstPlugin.numParams   = numParams;
   _vstPlugin.numPrograms = numPrograms;
   _vstPlugin.numInputs   = numInputs;
   _vstPlugin.numOutputs  = numOutputs;

   // setting the callbacks to the previously defined functions
   _vstPlugin.dispatcher             = &VSTPluginDispatcher;
   _vstPlugin.getParameter           = &VSTPluginGetParameter;
   _vstPlugin.setParameter           = &VSTPluginSetParameter;
   _vstPlugin.processReplacing       = &VSTPluginProcessReplacingFloat32;
   _vstPlugin.processDoubleReplacing = NULL;//&VSTPluginProcessReplacingFloat64;

   // report latency
   _vstPlugin.initialDelay = 0;

   sample_rate  = 44100.0f;
   block_size   = 64u;
   b_processing = true;
   b_offline    = false;
   b_check_offline = false;

   b_fix_denorm = false;

   last_program_chunk_str = NULL;

   b_open = false;

   prime_lock.init(512u*1024u); // max dly len, see delay.h / comb.h

   // printf("xxx end c'tor\n");
}

/**
 * Destructor called when the plugin is closed (see VSTPluginDispatcher with effClose opCode). In this very simply plugin
 * there is nothing to do but in general the memory that gets allocated MUST be freed here otherwise there might be a
 * memory leak which may end up slowing down and/or crashing the host
 */
VSTPluginWrapper::~VSTPluginWrapper() {
   closeEffect();
   instance_count--;
}



/**
 * Implementation of the main entry point of the plugin
 */
VST_EXPORT VSTPlugin *VSTPluginMain(VSTHostCallback vstHostCallback) {

#ifdef USE_LOG_PRINTF
   logfile = fopen("/tmp/vst2_log.txt", "w");
#endif // USE_LOG_PRINTF

   Dprintf("vstschroeder_plugin: called VSTPluginMain... \n");

#if 0
   if(!vstHostCallback(0, audioMasterVersion, 0, 0, 0, 0))
   {
		return 0;  // old version
   }
#endif

   // simply create our plugin C++ class
   VSTPluginWrapper *plugin =
      new VSTPluginWrapper(vstHostCallback,
                           // registered with Steinberg (http://service.steinberg.de/databases/plugin.nsf/plugIn?openForm)
                           CCONST('c', 'n', 'x', 'l'),
                           PLUGIN_VERSION, // version
                           VST2_MAX_PARAM_IDS,    // num params
                           1,    // one program
                           NUM_INPUTS,
                           NUM_OUTPUTS
                           );

   // Dprintf("xxx plugin=%p\n", plugin);

   return plugin->getVSTPlugin();
}
