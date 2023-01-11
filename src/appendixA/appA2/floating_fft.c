// Copyright 2020-2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.


#include "floating_fft.h"

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


static inline
complex_float_t complex_f32_mul(
    complex_float_t x,
    complex_float_t y)
{
  complex_float_t z = {
    x.re * y.re - x.im * y.im,
    x.re * y.im + x.im * y.re };
  return z;
}

static inline
complex_float_t complex_f32_conj_mul(
    complex_float_t x,
    complex_float_t y)
{
  complex_float_t z = {
    x.re * y.re - x.im * -y.im,
    x.re * -y.im + x.im * y.re };
  return z;
}

static inline
complex_float_t complex_f32_add(
    const complex_float_t x,
    const complex_float_t y)
{
  const complex_float_t z = {
    x.re + y.re,
    x.im + y.im
  };
  return z;
}

static inline
complex_float_t complex_f32_sub(
    const complex_float_t x,
    const complex_float_t y)
{
  const complex_float_t z = {
    x.re - y.re,
    x.im - y.im
  };
  return z;
}

static inline 
void vfttf(
    complex_float_t vD[])
{
  complex_float_t s[4];

  s[0].re = vD[0].re + vD[1].re;
  s[0].im = vD[0].im + vD[1].im;
  s[1].re = vD[0].re - vD[1].re;
  s[1].im = vD[0].im - vD[1].im;
  s[2].re = vD[2].re + vD[3].re;
  s[2].im = vD[2].im + vD[3].im;
  s[3].re = vD[2].im - vD[3].im;
  s[3].im = vD[3].re - vD[2].re;

  vD[0].re = s[0].re + s[2].re;
  vD[0].im = s[0].im + s[2].im;
  vD[1].re = s[1].re + s[3].re;
  vD[1].im = s[1].im + s[3].im;
  vD[2].re = s[0].re - s[2].re;
  vD[2].im = s[0].im - s[2].im;
  vD[3].re = s[1].re - s[3].re;
  vD[3].im = s[1].im - s[3].im;
}


static inline
void vfttb(
    complex_float_t vD[])
{
  complex_float_t s[4];

  s[0].re = vD[0].re + vD[1].re;
  s[0].im = vD[0].im + vD[1].im;
  s[1].re = vD[0].re - vD[1].re;
  s[1].im = vD[0].im - vD[1].im;
  s[2].re = vD[2].re + vD[3].re;
  s[2].im = vD[2].im + vD[3].im;
  s[3].re = vD[3].im - vD[2].im;
  s[3].im = vD[2].re - vD[3].re;

  vD[0].re = s[0].re + s[2].re;
  vD[0].im = s[0].im + s[2].im;
  vD[1].re = s[1].re + s[3].re;
  vD[1].im = s[1].im + s[3].im;
  vD[2].re = s[0].re - s[2].re;
  vD[2].im = s[0].im - s[2].im;
  vD[3].re = s[1].re - s[3].re;
  vD[3].im = s[1].im - s[3].im;
}


void flt_fft_forward_float(
    complex_float_t x[],
    const unsigned N,
    const complex_float_t W[])
{

  const unsigned FFT_N_LOG2 = 31 - CLS_S32(N);

  complex_float_t vC[4] = {{0}};

  for(int j = 0; j < (N>>2); j++){
    vfttf(&x[4*j]);
  }

  if(N == 4) return;

  for(int n = 0; n < FFT_N_LOG2-2; n++){
    
    int b = 1<<(n+2);
    int a = 1<<((FFT_N_LOG2-3)-n);

    for(int k = b-4; k >= 0; k -= 4){

      int s = k;

      // Keeping these on the stack will speed things up a lot
      vC[0] = W[0];
      vC[1] = W[1];
      vC[2] = W[2];
      vC[3] = W[3];

      for(int j = 0; j < a; j++){

        for(int i = 0; i < 4; i++){
          complex_float_t vD = x[s+b+i];
          complex_float_t tmp = x[s+i];
          complex_float_t vR = complex_f32_mul(vD, vC[i]);
          vD = complex_f32_sub(tmp, vR);
          vR = complex_f32_add(tmp, vR);
          x[s+i] = vR;
          x[s+b+i] = vD;
        }

        s += 2*b;
      }
      
      W = &W[4];
    }
  }
}

void flt_fft_inverse_float(
    complex_float_t x[], 
    const unsigned N,
    const complex_float_t W[])
{
  const unsigned FFT_N_LOG2 = 31 - CLS_S32(N);

  // Need to divide by N to make sure IFFT(FFT(x)) = x
  const float scale = ldexpf(1,-FFT_N_LOG2);

  for(int i = 0; i < N; i++){
    x[i].re = scale * x[i].re;
    x[i].im = scale * x[i].im;
  }

  complex_float_t vC[4] = {{0}};

  for(int j = 0; j < (N>>2); j++){
    vfttb(&x[4*j]);
  }

  if(N == 4) return;

  for(int n = 0; n < FFT_N_LOG2-2; n++){
    
    int b = 1<<(n+2);
    int a = 1<<((FFT_N_LOG2-3)-n);

    for(int k = b-4; k >= 0; k -= 4){

      int s = k;

      // Keeping these on the stack will speed things up a lot
      vC[0] = W[0];
      vC[1] = W[1];
      vC[2] = W[2];
      vC[3] = W[3];

      for(int j = 0; j < a; j++){

        for(int i = 0; i < 4; i++){
          complex_float_t vD = x[s+b+i];
          complex_float_t tmp = x[s+i];
          complex_float_t vR = complex_f32_conj_mul(vD, vC[i]);
          vD = complex_f32_sub(tmp, vR);
          vR = complex_f32_add(tmp, vR);
          x[s+i] = vR;
          x[s+b+i] = vD;
        }

        s += 2*b;
      }
      
      W = &W[4];
    }
  }
}


void flt_fft_mono_adjust_float(
    complex_float_t x[],
    const unsigned FFT_N,
    const unsigned inverse,
    const complex_float_t W[])
{

  W = &W[FFT_N-8];
  
  // REMEMBER: The length of x[] is only FFT_N/2!
  complex_float_t X0 = x[0];
  complex_float_t XQ = x[FFT_N/4];

  complex_float_t* p_X_lo = &x[0];
  complex_float_t* p_X_hi = &x[FFT_N/2];

  if(inverse){
      complex_float_t* tmp = p_X_hi;
      p_X_hi = p_X_lo;
      p_X_lo = tmp;
  }
  
  if(!inverse){
    p_X_lo = &p_X_lo[1];
    p_X_hi = &p_X_hi[-1];
  } else {
    p_X_lo = &p_X_lo[-1];
    p_X_hi = &p_X_hi[1];
  }

  for(int k = 1; k < (FFT_N/4); k+=1){

    const unsigned G = k % 4;
    if(G == 0) W = &W[-4];

    const complex_float_t X_lo = p_X_lo[0];
    const complex_float_t X_hi = p_X_hi[0];

    // tmp = j*W
    const complex_float_t tmp = { 0.5f * W[G].im, 0.5f * -W[G].re };

    // A = 0.5*(1 - j*W)
    // B = 0.5*(1 + j*W)
    complex_float_t A, B;
    A.re = 0.5f - tmp.re;
    A.im = -tmp.im;
    B.re = 0.5f + tmp.re;
    B.im = tmp.im;

    // new_X_lo = A*X_lo + B*conjugate(X_hi)
    p_X_lo[0] = complex_f32_add(
                    complex_f32_mul(A, X_lo), 
                    complex_f32_conj_mul(B, X_hi));

    // new_X_hi = conjugate(A)*X_hi + conjugate(B)*conjugate(X_lo)
    B.im = -B.im;
    p_X_hi[0] = complex_f32_add(
                    complex_f32_conj_mul(X_hi, A), 
                    complex_f32_conj_mul(B, X_lo));


    if(!inverse){
      p_X_lo = &p_X_lo[1];
      p_X_hi = &p_X_hi[-1];
    } else {
      p_X_lo = &p_X_lo[-1];
      p_X_hi = &p_X_hi[1];
    }
  }

  if(inverse){
    X0.re = 0.5f * X0.re;
    X0.im = 0.5f * X0.im;
  }

  //Fix DC and Nyquist
  x[0].re = X0.re + X0.im;
  x[0].im = X0.re - X0.im;
  x[FFT_N/4].re =  XQ.re;
  x[FFT_N/4].im = -XQ.im;
  
}

