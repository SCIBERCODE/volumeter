#pragma once
// Copyright (c) 2015 Tony Kirke.  Boost Software License - Version 1.0  (http://www.opensource.org/licenses/BSL-1.0)
#include <vector>
namespace spuce {
//! \file
// \brief Template partial convolve function
//! \author Tony Kirke,  Copyright(c) 2001
//! \ingroup template_array_functions misc
template <class T> std::vector<T> partial_convolve(const std::vector<T>& x, const std::vector<T>& y, size_t N, size_t M) {
  auto L = M + N - 1;
  std::vector<T> c(L);
  for (int i = 0; i < L; i++) {
    c[i] = (T)0;
    for (int j = 0; j < N; j++) {
      if ((i - j >= 0) & (i - j < M)) c[i] += x[j] * y[i - j];
    }
  }
  return (c);
}
}  // namespace spuce
