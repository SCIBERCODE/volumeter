#pragma once
// Copyright (c) 2015 Tony Kirke.  Boost Software License - Version 1.0  (http://www.opensource.org/licenses/BSL-1.0)
#include <cmath>
#include <spuce/filters/fir_coeff.h>
#include <spuce/filters/raised_cosine_imp.h>
namespace spuce {
//! \file
//! \brief  Raised Cosine functions
//! \author Tony Kirke
//! \ingroup template_functions comm
template <class T> void raised_cosine(fir_coeff<T>& rcfir, float_type alpha, float_type rate) {
  size_t num_taps = rcfir.number_of_taps();
  float_type sum = 0;
  for (size_t i = 0; i < num_taps; i++) {
    sum += (float_type)raised_cosine_imp(alpha, float_type(i), rate, num_taps);
  }
  // Normalize impulse response
  for (size_t i = 0; i < num_taps; i++) {
    rcfir.settap(i,(T)((1.0/sum)*raised_cosine_imp(alpha, float_type(i), rate, num_taps)));
  }
}

}  // namespace spuce
