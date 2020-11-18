// Copyright (c) 2014-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0
/*
 * FIR filter class, by Mike Perkins
 *
 * a simple C++ class for linear phase FIR filtering
 *
 * For background, see the post http://www.cardinalpeak.com/blog?p=1841
 *
 * Copyright (c) 2013, Cardinal Peak, LLC.  http://www.cardinalpeak.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1) Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3) Neither the name of Cardinal Peak nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * CARDINAL PEAK, LLC BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#define _USE_MATH_DEFINES
#include <vector>
#include <cmath>
#include <cfloat>
#include <cstdlib>
#include <numeric>
#include <iostream>
#include <spuce/filters/sinc_helper.h>

//! Band pass: sample rate, lower frequency, upper frequency, window
std::vector<double> sincBPF(const size_t numTaps, const double Fl, const double Fu)
{
    std::vector<double> taps(numTaps);
    const auto lambda = M_PI * Fl / (0.5);
    const auto phi = M_PI * Fu / (0.5);

    for (size_t n = 0; n < numTaps; n++)
    {
        const double mm = n - (numTaps - 1.0) / 2.0;
        if( mm == 0.0 ) taps[n] = (phi - lambda) / M_PI;
        else taps[n] = (   sin( mm * phi ) -
                             sin( mm * lambda )   ) / (mm * M_PI);
    }

    return taps;
}

//! Band stop: sample rate, lower frequency, upper frequency, window
std::vector<double> sincBSF(const size_t numTaps, const double Fl, const double Fu)
{
    std::vector<double> taps(numTaps);
    const auto lambda = M_PI * Fl / (0.5);
    const auto phi = M_PI * Fu / (0.5);

    for (size_t n = 0; n < numTaps; n++)
    {
        const double mm = n - (numTaps - 1.0) / 2.0;
        if( mm == 0.0 ) taps[n] = 1.0 - (phi - lambda) / M_PI;
        else taps[n] = -(   sin( mm * phi ) -
                             sin( mm * lambda )   ) / (mm * M_PI);
    }
    return taps;
}

