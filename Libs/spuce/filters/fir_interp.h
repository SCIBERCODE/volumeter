#pragma once
// Copyright (c) 2015 Tony Kirke. License MIT  (http://www.opensource.org/licenses/mit-license.php)
#include <spuce/filters/fir.h>
namespace spuce {
//! \file
//! \brief template class fir_decim based on FIR class
//!  created to support polyphase FIR interpolation
//! \author Tony Kirke
//! \ingroup double_templates fir interpolation
template <class Numeric, class Coeff = float_type> class fir_interp : public fir<Numeric, Coeff> {
  using fir<Numeric, Coeff>::num_taps;
  using fir<Numeric, Coeff>::coeff;
  using fir<Numeric, Coeff>::z;
  using fir<Numeric, Coeff>::read_taps;
  typedef typename base_type<Numeric>::btype Numeric_base;
  typedef typename mixed_type<Numeric, Coeff>::dtype sum_type;

 public:
  long num_low;
  long rate;       //! upsampling rate
  long phase;      //! current polyphase phase
  long auto_mode;  //! if set, automaticaly increment phase

 public:
  //! Skip output sample but increment phase
  void skip() {
    phase++;
    phase = phase % rate;
  }
  //! Set interpolation rate
  void set_rate(long r) {
    rate = r;
    num_low = (num_taps / r);
  }
  void set_automatic(void) { auto_mode = 1; }
  void set_manual(int def_phase = 0) {
    auto_mode = 0;
    phase = def_phase;
  }

  //! Constructor
  fir_interp<Numeric, Coeff>(const char* i) : fir<Numeric, Coeff>(i), phase(0), auto_mode(1) {}
  fir_interp<Numeric, Coeff>(void) : phase(0), auto_mode(1) {}
  fir_interp<Numeric, Coeff>(long n) : fir<Numeric, Coeff>(n), phase(0), auto_mode(1) {}
  void reset() {
    fir<Numeric, Coeff>::reset();
    phase = 0;
  }
  Coeff coeff_sum() { return (fir<Numeric, Coeff>::coeff_sum()); }
  void input(Numeric in) {
    int i;
    // Update history of inputs
    for (i = num_low; i > 0; i--) z[i] = z[i - 1];
    // Add new input
    z[0] = in;
  }
  //! Explicitly set the phase
  Numeric clock(long set_phase) {
    phase = set_phase;
    return (clock());
  }
  //! Phase increments when in automatic mode
  //! Otherwise phase does not change
  Numeric clock(void) {
    // Perform FIR
    Numeric output;
    int i;
    sum_type sum(0);
    for (i = 0; i < num_low; i++) sum = sum + coeff[i * rate + phase] * z[i];

    output = (sum);
    // Increment to next polyphase filter
    if (auto_mode) phase++;
    phase = phase % rate;
    return (output);
  }
};
}  // namespace spuce
