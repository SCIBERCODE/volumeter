#pragma once
// Copyright (c) 2015 Tony Kirke. License MIT  (http://www.opensource.org/licenses/mit-license.php)
#include <spuce/typedefs.h>
#include <spuce/filters/fir.h>
namespace spuce {
//! \file
//! \brief template class fir_adapt Based on FIR class, created to support LMS adaptive filtering
//! \author Tony Kirke
//! \ingroup double_utemplates equalizers fir
//! \image html fir_adapt.gif
//! \image latex fir_adapt.eps
template <class Numeric, class Coeff = double> class fir_adapt : public fir<Numeric, Coeff> {
 public:
  typedef typename base_type<Numeric>::btype real_type;
  //! Adaptation gain
  real_type u;

 public:
  //! Constructor
  fir_adapt<Numeric, Coeff>(void) : fir<Numeric, Coeff>(), u(0) {}
  //! Constructor with size
  fir_adapt<Numeric, Coeff>(long n, real_type gain) : fir<Numeric, Coeff>(n), u(gain) {}
  //! Reset both history and coefficients!
  void reset() {
    fir<Numeric, Coeff>::reset();
    for (int i = 0; i < fir<Numeric, Coeff>::num_taps; i++) fir<Numeric, Coeff>::coeff[i] = (Coeff)0;
  }
  //! Set adaptation gain
  void set_gain(real_type gain) { u = gain; }
  //! Calculate updated taps using LMS algorithm
  void update_lms(Numeric err) {
    Numeric m = u * err;
    for (int i = 0; i < fir<Numeric, Coeff>::num_taps; i++)
      fir<Numeric, Coeff>::coeff[i] = fir<Numeric, Coeff>::coeff[i] + m * signbit(conj(fir<Numeric, Coeff>::z[i]));
  }
  //! Calculate updated taps using LMS algorithm and sign of input
  void update_lms_sign_coeff(Numeric err) {
    Numeric m = u * err;
    for (int i = 0; i < fir<Numeric, Coeff>::num_taps; i++)
      fir<Numeric, Coeff>::coeff[i] = fir<Numeric, Coeff>::coeff[i] + m * signbit(conj(fir<Numeric, Coeff>::z[i]));
  }
};
}  // namespace spuce
