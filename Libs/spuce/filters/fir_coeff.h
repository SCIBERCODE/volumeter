#pragma once
// Copyright (c) 2015 Tony Kirke.  Boost Software License - Version 1.0  (http://www.opensource.org/licenses/BSL-1.0)
#include <spuce/typedefs.h>
#include <iostream>
#include <vector>
namespace spuce {
//! \file
//! \brief  Template Class for Modeling a Finite Impulse Response filter.
//
//!  Template works for double, long, std::complex, etc
//!  Taps initialized to zeros.
//! \author Tony Kirke
//! \ingroup templates fir
//! \image html fir.gif
//! \image latex fir.eps
template <class Numeric> class fir_coeff {
 public:
  //! Set tap weights
  void settap(size_t i, Numeric tap) { coeff[i] = tap; }
  Numeric gettap(size_t i) { return (coeff[i]); }
  size_t number_of_taps() const { return (num_taps); }
  //! Get sum of coefficients
  Numeric coeff_sum() {
    Numeric s(0);
    for (size_t i = 0; i < num_taps; i++) s += coeff[i];
    return (s);
  }
  //! Constructor
  ~fir_coeff(void) {}
  //! Constructor
  fir_coeff(void) { ; }
  //! Constructor
  fir_coeff(size_t n) : coeff(n), num_taps(n) { set_size(n); }
  //! Set size of Filter
  void set_size(size_t n) {
    num_taps = n;
    if (n > 0) {
      coeff.resize(n);
      for (size_t i = 0; i < n; i++) coeff[i] = (Numeric)0;
    }
  }
  size_t get_size(void) { return (num_taps); }
  //!  Constructor that gets coefficients from file (requires fir_coeff.cpp)
  fir_coeff(const char* file) { read_taps(file); }
  int read_taps(const char* file) { return (0); }
  void make_hpf() {
    bool inv = true;
    for (size_t i = 0; i < num_taps; i++) {
      if (inv) coeff[i] *= -1;
      inv = !inv;
    }
  }
  void print(void) {
    std::cout << "coeff[] = ";
    for (size_t i = 0; i < num_taps; i++) std::cout << coeff[i] << ",";
    std::cout << "\n";
  }
  template <class N> friend std::vector<N> get_taps(fir_coeff<N> x);
  void set_taps(const std::vector<Numeric>& taps) {
    for (size_t i = 0; i < num_taps; i++) coeff[i] = taps[i];
  }
  // Get frequency response at freq
  float_type freqz_mag(float_type freq) {
    std::complex<float_type> z(1, 0);
    std::complex<float_type> z_inc = std::complex<float_type>(cos(freq), sin(freq));
    std::complex<float_type> nom(0);
    for (size_t i = 0; i < num_taps; i++) {
      nom += z * (std::complex<float_type>(coeff[i]));
      z *= z_inc;
    }
    return sqrt(std::norm(nom));
  }

 private:
  std::vector<Numeric> coeff;
  size_t num_taps;
};

template <class T> std::vector<T> get_taps(fir_coeff<T> f) {
  size_t N = f.num_taps;
  std::vector<T> V(N);
  for (size_t i = 0; i < N; i++) V[i] = f.coeff[i];
  return (V);
}
}  // namespace spuce
