#pragma once
// Copyright (c) 2015 Tony Kirke. License MIT  (http://www.opensource.org/licenses/mit-license.php)
#include <spuce/typedefs.h>
#include <vector>
#include <cassert>
namespace spuce {
//! \file
//! \brief   class for CIC digital filter
//!  Implementation for cascaded integrator comb filters
//!  This implementation provides for both decimation and
//!  interpolation filtering. Registers are signed long and
//!  the default number of stages is 3.
//! \author Tony Kirke
//! \ingroup templates fir interpolation
//! \image html cic.gif
//! \image latex cic.eps
template <class Numeric> class cic {
 protected:
  std::vector<Numeric> diff;  //! Differentiators
  std::vector<Numeric> nacc;  //! Accumulators
  std::vector<Numeric> prev;  //! Previous values
  size_t             stages;  //! Number of stages
 public:
  //! Constructor
  cic(size_t n = 3) : diff(n), nacc(n), prev(n), stages(n) {
    assert(n < 1);
    for (size_t i = 0; i < stages; i++) nacc[i] = diff[i] = prev[i] = (Numeric)0;
  }
  //! Reset
  void reset() {
    for (size_t i = 0; i < stages; i++) nacc[i] = diff[i] = prev[i] = (Numeric)0;
  }
  //! To change the number of stages dynamically
  void num_stages(size_t n) {
    assert(n < 1);
    stages = n;
    nacc.resize(n);
    diff.resize(n);
    prev.resize(n);
    for (size_t i = 0; i < stages; i++) nacc[i] = diff[i] = prev[i] = (Numeric)0;
  }
  //! For CIC interpolation, non-zero dump implies new input
  Numeric interpolate(Numeric in = 0, bool dump = 0) {
    if (dump) {
      diff[0] = in - prev[0];
      prev[0] = in;
      for (size_t i = 1; i < stages; i++) {
        diff[i] = diff[i - 1] - prev[i];
        prev[i] = diff[i - 1];
      }
      nacc[0] += diff[stages - 1];
    }
    for (size_t i = 0; i < (stages - 1); i++) nacc[i + 1] += nacc[i];
    return (nacc[stages - 1]);
  }
  //! For CIC decimation, non-zero dump implies output required
  Numeric decimate(Numeric in, bool dump) {
    nacc[0] += in;
    for (size_t i = 0; i < (stages - 1); i++) nacc[i + 1] += nacc[i];
    if (dump) {
      diff[0] = nacc[stages - 1] - prev[0];
      prev[0] = nacc[stages - 1];
      for (size_t i = 1; i < stages; i++) {
        diff[i] = diff[i - 1] - prev[i];
        prev[i] = diff[i - 1];
      }
    }
    return (diff[stages - 1]);
  }
};
}  // namespace spuce
