/* ----
 * ---- file   : prime.h
 * ---- author : bsp
 * ---- legal  : Distributed under terms of the MIT LICENSE (MIT).
 * ----
 * ---- Permission is hereby granted, free of charge, to any person obtaining a copy
 * ---- of this software and associated documentation files (the "Software"), to deal
 * ---- in the Software without restriction, including without limitation the rights
 * ---- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * ---- copies of the Software, and to permit persons to whom the Software is
 * ---- furnished to do so, subject to the following conditions:
 * ----
 * ---- The above copyright notice and this permission notice shall be included in
 * ---- all copies or substantial portions of the Software.
 * ----
 * ---- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * ---- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * ---- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * ---- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * ---- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * ---- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * ---- THE SOFTWARE.
 * ----
 * ---- info   : Eratosthenes Sieve prime number calculation
 * ----
 * ---- changed: 09Jul2019
 * ----
 * ----
 * ----
 */

#ifndef PRIME_H__
#define PRIME_H__


class Prime {
  public:
   sUI *flags;
   int  size;
   sUI  num_primes;

  public:
   Prime(void) {
      flags = NULL;
      size  = 0;
      num_primes = 0u;
   }

   ~Prime() {
      if(NULL != flags)
      {
         delete [] flags;
         flags = NULL;
         size  = 0;
         num_primes = 0u;
      }
   }

   sBool init(sUI _num) {
      sBool r = YAC_FALSE;

      flags = new(std::nothrow) sUI[_num];

      if(NULL != flags)
      {
         size = int(_num);
         int i, prime;

         num_primes = 0u;
         for(i = 0; i <= size; i++) flags[i] = 1;
         for(i = 0; i <= size; i++)
         {
            if(flags[i])
            {
               prime = i + i + 3;
               int k = i + prime;
               while(k <= size)
               {
                  flags[k] = 0;
                  k += prime;
               }
               num_primes++;
            }
         }

         r = YAC_TRUE;
      }

      return r;
   }
};


class PrimeLock {
  public:
   Prime prime;
   sUI *prime_tbl;  // idx => nearest prime

   PrimeLock(void) {
      prime_tbl = NULL;
   }

   ~PrimeLock() {
      if(NULL != prime_tbl)
      {
         delete [] prime_tbl;
         prime_tbl = NULL;
      }
   }

   void init(sUI _num) {

      prime_tbl = new(std::nothrow) sUI[_num];

      if(NULL != prime_tbl)
      {
         if(prime.init(_num))
         {
            sUI k = 0u;
            for(sUI i = 0u; i < _num; i++)
            {
               if(prime.flags[i])
                  k = i;
               prime_tbl[i] = k;
               /* printf("prime[%u]=%u\n", i, prime_tbl[i]); */
            }
         }
         else
         {
            delete [] prime_tbl;
            prime_tbl = NULL;
         }
      }
   }
};


#endif // PRIME_H__
