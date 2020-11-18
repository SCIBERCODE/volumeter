// Copyright (c) 2015 Tony Kirke.  Boost Software License - Version 1.0  (http://www.opensource.org/licenses/BSL-1.0)
//! \author Tony Kirke
#define _USE_MATH_DEFINES
#include <spuce/filters/chebyshev_iir.h>
#include <cfloat>
namespace spuce {
//! fcd = cut-off (1=sampling rate)
//! ord = Filter order
//! ripple = passband ripple in dB
void chebyshev_iir(iir_coeff& filt, float_type fcd, float_type ripple = 3.0) {
  const float_type ten = 10.0;
  auto order = filt.getOrder();
  float_type rlin = pow(ten, ripple/ten);
  float_type epi  = sqrt(rlin - 1.0);
  //! wca - pre-warped angular frequency
  float_type wca = (filt.get_type()==filter_type::high) ? tan(M_PI * (0.5-fcd)) : tan(M_PI*fcd);
  chebyshev_s(filt, wca, epi, order);
  filt.bilinear();
	if (filt.get_type()==filter_type::bandpass || filt.get_type()==filter_type::bandstop) {
		filt.make_band(filt.get_center());
	} else {
		filt.convert_to_ab();
	}
	if (filt.get_type()==filter_type::bandpass) filt.set_bandpass_gain();
  // Must scale even order filter by this factor
  float_type gain = 1.0/sqrt(rlin); 
  if (!filt.isOdd()) filt.apply_gain(gain);
}
//! Calculate poles (chebyshev)
void chebyshev_s(iir_coeff& filt, float_type wp, float_type epi, size_t n) {
  auto l = 1;
  size_t n2 = (n + 1) / 2;
  float_type arg;
  float_type x = 1 / epi;
  float_type lambda = pow(x*(1.0 + sqrt(1.0 + epi*epi)),1.0/n);
  float_type sm = 0.5*((1.0/lambda) - lambda);
  float_type cm = 0.5*((1.0/lambda) + lambda);
  for (size_t j = 0; j < n2; j++) {
    arg = 0.5 * M_PI * (2*l-1) / ((float_type)(n));
    if (filt.get_type()==filter_type::low) {
        filt.set_pole(-wp * std::complex<float_type>(sm * sin(arg), cm * cos(arg)), n2-1-j);
        filt.set_zero(FLT_MAX, n2-1-j);
    } else {
        filt.set_pole(-1.0 / (wp * std::complex<float_type>(sm * sin(arg), cm * cos(arg))), n2-1-j);
        filt.set_zero(0, n2-1-j);
    }
    l++;
  }
}
}  // namespace spuce
