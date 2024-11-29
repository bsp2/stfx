// ----
// ---- file   : delay.h
// ---- author : bsp
// ---- info   : monophonic delay line
// ----
// ---- created: 24May2020
// ---- changed: 13Oct2021, 27Sep2024
// ----
// ----
// ----

#ifndef __ST_DELAY_H__
#define __ST_DELAY_H__

#ifndef ST_DELAY_SIZE
// ~743ms @44.1kHz
#define ST_DELAY_SIZE  (32768u)
#define ST_DELAY_MASK  (32767u)
#else
#ifndef ST_DELAY_MASK
#define ST_DELAY_MASK (ST_DELAY_SIZE-1u)
#endif // ST_DELAY_MASK
#endif // ST_DELAY_SIZE

struct StDelay {
   unsigned int io_offset;
   float        last_out;
   float        history[ST_DELAY_SIZE];

   StDelay(void) {
      reset();
   }

   ~StDelay() {
   }

   void reset(void) {
      io_offset = 0u;
      last_out  = 0.0f;
      memset((void*)history, 0, sizeof(history));
   }

   void push(float _smp, float _fbAmt) {
      // fb
      io_offset = (io_offset + 1u) & ST_DELAY_MASK;
      history[io_offset] = _smp + (last_out * _fbAmt);
   }

   void pushRaw(float _smp) {
      // no fb  (or external fb)
      io_offset = (io_offset + 1u) & ST_DELAY_MASK;
      history[io_offset] = _smp;
   }

   float readNearest(unsigned int _offset) {
      _offset = (io_offset - _offset) & ST_DELAY_MASK;
      last_out = history[_offset];
      return last_out;
   }

   float readLinear(float _offset) {
      unsigned int offC = (unsigned int)(_offset);
      unsigned int offN = (unsigned int)(_offset + 1u);
      offC = (io_offset - offC) & ST_DELAY_MASK;
      offN = (io_offset - offN) & ST_DELAY_MASK;
      float t = (_offset - (int)_offset);
      last_out = history[offC] + (history[offN] - history[offC]) * t;
      return last_out;
   }

   float readLinearNorm(float _offsetNorm) {
      _offsetNorm *= (float)(ST_DELAY_SIZE - 1u);
      if(_offsetNorm >= (float)(ST_DELAY_SIZE - 1u))
         _offsetNorm = (float)(ST_DELAY_SIZE - 1u);
      last_out = readLinear(_offsetNorm);
      return last_out;
   }

   void add(float _smp) {
      history[io_offset] += _smp;
   }
};


#endif // __ST_DELAY_H__
