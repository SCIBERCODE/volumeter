#pragma once
// Copyright (c) 2015 Tony Kirke.  Boost Software License - Version 1.0  (http://www.opensource.org/licenses/BSL-1.0)
#include <vector>
#include <stdexcept>
#include <spuce/typedefs.h>
namespace spuce {

enum class remez_type {BANDPASS,DIFFERENTIATOR,HILBERT};
enum class remez_symmetry {POSITIVE,NEGATIVE}; 
//!-------------------------------------------------
//! \file
//! \brief template Remez_fir class
//
//! <p>Calculates the optimal (in the Chebyshev/minimax sense)
//! FIR filter impulse response given a set of band edges,
//! the desired reponse on those bands, and the weight given to
//! the error in those bands.
//!
//! <p><b>Inputs:</b>
//! <p>int      numtaps - Number of filter coefficients</p>
//! <p>int      numband - Number of bands in filter specification
//! <p>float_type[] bands   - User-specified band edges [2 * numband]
//! <p>float_type[] des     - User-specified band responses [numband]
//! <p>float_type[] weight  - User-specified error weights [numband]
//! <p>int      type    - Type of filter
//!
//! <p><b>Output:</b>
//! <p>float_type[] h       - Impulse response of final filter [numtaps]
//!
//! \author Tony Kirke
//! \ingroup classes fir
class remez_fir {
 public:
  remez_fir() {}

 private:
  static void createDenseGrid(int r, int numtaps, int numband, std::vector<float_type>& bands,
                              const std::vector<float_type>& des, const std::vector<float_type>& weight, int gridSize,
                              std::vector<float_type>& grid, std::vector<float_type>& d, std::vector<float_type>& w,
                              remez_symmetry symmetry);

  static std::vector<float_type> calc_d(int r, const std::vector<float_type>& x);
  static std::vector<float_type> calc_y(int r, const std::vector<int>& ext, const std::vector<float_type>& des,
                                        const std::vector<float_type>& w, const std::vector<float_type>& d);
  static float_type gee(float_type freq, int r, const std::vector<float_type>& ad, const std::vector<float_type>& x,
                        const std::vector<float_type>& y);
  static void search(int r, std::vector<int>& ext, int gridSize, const std::vector<float_type>& e);
  static bool isDone(int r, const std::vector<int>& ext, const std::vector<float_type>& e);

 public:
  static bool remez(std::vector<float_type>& filt, int n, int numband, std::vector<float_type>& bands,
                    const std::vector<float_type>& des, const std::vector<float_type>& weight, remez_type type);
};
}  // namespace spuce
