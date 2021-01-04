﻿#pragma once
#include <JuceHeader.h>
#include <spuce/filters/iir.h>
#include <spuce/filters/butterworth_iir.h>
#include "main.h"

using namespace spuce;

extern unique_ptr<settings_> __opt;

//=========================================================================================
class sin_
//=========================================================================================
{
public:
    sin_() : sampling_rate(44100.0),
             angle_delta  (0.0),
             freq         (0.0)
    {
        reset();
    }
    void set_sample_rate(const double sample_rate) {
        sampling_rate = sample_rate;
    }
    void reset() {
        for (auto& angle : angles)
            angle = 0.0;
    }
    void set_freq(const float frequency) {
        freq = frequency;
        auto cycles_per_sample = frequency / sampling_rate;
        angle_delta = cycles_per_sample * 2.0 * M_PI;
    }
    auto sample(const volume_t channel) {
        auto current_sample = sin(angles[channel]);
        angles[channel] += angle_delta;

        if (angles[channel] > 2.0 * M_PI)
            angles[channel] -= 2.0 * M_PI;

        return static_cast<float>(current_sample);
    }

private:
    double sampling_rate,
           freq,
           angle_delta,
           angles[VOLUME_SIZE];
};

//=========================================================================================
class circle_
//=========================================================================================
{
protected:
    struct iterator
    {
        size_t   size;
        size_t   pointer;
        volume_t channel;
    };
private:
    unique_ptr<float[]> buffers[VOLUME_SIZE];
    size_t              tails  [VOLUME_SIZE];
    size_t              max_size;
    iterator            it;
public:
    circle_(const size_t max_size) : max_size(max_size)
    {
        for (auto& buffer : buffers)
            buffer = make_unique<float[]>(max_size);

        clear();
    }

    void clear()
    {
        for (auto line = LEFT; line < VOLUME_SIZE; line++) {
            if (buffers[line])
                fill_n(buffers[line].get(), max_size, numeric_limits<float>::quiet_NaN());

            tails[line] = 0;
        }
    }

    void enqueue(const volume_t channel, const float value)
    {
        if (max_size && isfinite(value))
        {
            buffers[channel][tails[channel]] = value;
            tails[channel] = (tails[channel] + 1) % max_size;
        }
    }

    float get_tail(const volume_t channel) {
        return buffers[channel][tails[channel]];
    }

    double get_rms(const volume_t channel)
    {
        double sum = 0.0;
        size_t size = 0;

        for (size_t k = 0; k < max_size; ++k)
        {
            auto sample = buffers[channel][k];
            if (isfinite(sample)) {
                sum += sample;
                size++;
            }
        }
        return sqrt(sum / size);
    }

    bool get_first_value(const volume_t channel, const size_t size, float& value) {
        if (size == 0 || size > max_size || /* bug: исправить логику */ tails[channel] == 0)
            return false;

        it.pointer = tails[channel] - 1;
        it.size    = size;
        it.channel = channel;
        return get_next_value(value);
    }

    bool get_next_value(float& value) {
        auto result = numeric_limits<float>::quiet_NaN();
        if (it.pointer >= 0 && it.size)
        {
            result = buffers[it.channel][it.pointer];
            if (it.pointer == 0)
                it.pointer = max_size - 1;
            else
                it.pointer--;

            it.size--;
        };
        if (!isfinite(result))
            it = { 0 };

        if (isfinite(result))
            value = result;

        return isfinite(result);
    }

    auto get_extremes(const volume_t channel, const size_t size) {
        auto  result = make_unique<float[]>(EXTREMES_SIZE);
        float value;

        if (get_first_value(channel, size, value)) {
            result[MIN] = value;
            result[MAX] = value + 0.00001f; // todo: избавиться
            do {
                if (!isfinite(value)) break;
                result[MIN] = min(value, result[MIN]);
                result[MAX] = max(value, result[MAX]);
            }
            while (get_next_value(value));
        }
       return result;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(circle_)

};

//=========================================================================================
class signal_
//=========================================================================================
{
private:
    double              _sample_rate;
    mutex               _locker;
    sin_                _sin;
    unique_ptr<circle_> _buff;
    unique_ptr<iir<float_type, float_type>>
                        _filters [VOLUME_SIZE][FILTER_TYPE_SIZE];
    size_t              _orders               [FILTER_TYPE_SIZE];
    double              _extremes[VOLUME_SIZE][EXTREMES_SIZE];
    double              _zeros   [VOLUME_SIZE];
public:

    signal_()
    {
        _sample_rate       = __opt->get_int(L"sample_rate"    );
        _orders[HIGH_PASS] = __opt->get_int(L"pass_high_order");
        _orders[LOW_PASS ] = __opt->get_int(L"pass_low_order" );

        zero_clear();
    }

    void zero_set(
        double value_left  = numeric_limits<float>::quiet_NaN(),
        double value_right = numeric_limits<float>::quiet_NaN())
    {
        lock_guard<mutex> locker(_locker);
        if (isfinite(value_left) && isfinite(value_right)) {
            _zeros[LEFT ] = value_left;
            _zeros[RIGHT] = value_right;
        }
        else {
            _zeros[LEFT ] = gain2db(_buff->get_rms(LEFT ));
            _zeros[RIGHT] = gain2db(_buff->get_rms(RIGHT));
            __opt->save(L"zero_value_left" , _zeros[LEFT ]);
            __opt->save(L"zero_value_right", _zeros[RIGHT]);
        }
        extremes_clear();
    }
    auto zero_get(const volume_t channel) {
        return _zeros[channel];
    }
    void zero_clear() {
        for (auto& zero : _zeros) zero = 0.0;
        extremes_clear();
    }

    void extremes_set(const vector<double>& rms) {
        for (auto line = LEFT; line < VOLUME_SIZE; line++) // bug: на вольтах при переключении нижняя граница не похожа на правду
            if (isfinite(rms.at(line)))
            {
                if (!isfinite(_extremes[line][MIN])) _extremes[line][MIN] = rms.at(line);
                if (!isfinite(_extremes[line][MAX])) _extremes[line][MAX] = rms.at(line);
                _extremes[line][MIN] = min(_extremes[line][MIN], rms.at(line));
                _extremes[line][MAX] = max(_extremes[line][MAX], rms.at(line));
            }
    }
    const auto extremes_get(const volume_t channel) {
        return _extremes[channel];
    }
    void extremes_clear() {
        for (auto& line : _extremes)
            for (auto& column : line)
                column = numeric_limits<float>::quiet_NaN();
    }

    double gain2db(const double gain) {
        return 20 * log10(gain);
    }
    void set_freq(const double freq) {
        _sin.set_freq(static_cast<float>(freq));
        _sin.reset();
    }
    void change_buff_size(const int new_size) {
        lock_guard<mutex> locker(_locker);
        if (new_size && _sample_rate) {
            auto number_of_samples = static_cast<size_t>(new_size / (1000.0 / _sample_rate));
            _buff = make_unique<circle_>(number_of_samples);
        }
    }
    void clear_data() {
        lock_guard<mutex> locker(_locker);
        if (_buff)
            _buff->clear();
    }
    void set_order(const filter_type_t filter_type, const int new_order) {
        _orders[filter_type] = new_order;
    }
    vector<double> get_rms() {
        vector<double> result;
        if (_buff)
        {
            result.push_back(_buff->get_rms(LEFT));
            result.push_back(_buff->get_rms(RIGHT));
        };
        return result;
    }

    void filter_init(const filter_type_t filter_type)
    {
        lock_guard<mutex> locker(_locker);
        for (auto channel = LEFT; channel <= RIGHT; channel++)
        {
            if (filter_type == HIGH_PASS)
            {
                iir_coeff high_pass(_orders[filter_type], filter_type::high);
                butterworth_iir(high_pass, __opt->get_int(L"pass_high_freq") / _sample_rate, 3.0);
                _filters[channel][HIGH_PASS] = make_unique<iir<float_type, float_type>>(high_pass);
            }
            if (filter_type == LOW_PASS)
            {
                iir_coeff low_pass(_orders[filter_type], filter_type::low);
                butterworth_iir(low_pass, __opt->get_int(L"pass_low_freq") / _sample_rate, 3.0);
                _filters[channel][LOW_PASS] = make_unique<iir<float_type, float_type>>(low_pass);
            }
        }
    };

    void prepare_to_play(const double sample_rate)
    {
        _sample_rate = sample_rate;

        _sin.set_sample_rate(_sample_rate);
        _sin.set_freq(static_cast<float>(__opt->get_int(L"tone_value")));
        _sin.reset();

        filter_init(HIGH_PASS);
        filter_init(LOW_PASS);
    }

    void next_audio_block(const AudioSourceChannelInfo& buffer)
    {
        if (_locker.try_lock()) // todo: битый буфер не норм, сигнализировать, что не тянет проц текущих настроек
        {
            if (_buff)
            {
                auto read          = buffer.buffer->getArrayOfReadPointers();
                auto write         = buffer.buffer->getArrayOfWritePointers();
                auto use_high_pass = __opt->get_int(L"pass_high") && _filters[LEFT][HIGH_PASS] && _filters[RIGHT][HIGH_PASS];
                auto use_low_pass  = __opt->get_int(L"pass_low" ) && _filters[LEFT][LOW_PASS ] && _filters[RIGHT][LOW_PASS ];
                auto use_tone      = __opt->get_int(L"tone"); // todo: ускорить работу в этой процедуре

                for (auto channel = LEFT; channel <= RIGHT; channel++)
                    for (auto sample_index = 0; sample_index < buffer.numSamples; ++sample_index)
                    {
                        _buff->enqueue(channel, read[channel][sample_index] * read[channel][sample_index]);
                        if (use_high_pass || use_low_pass || use_tone)
                        {
                            float_type sample = use_tone ? _sin.sample(channel) : read[channel][sample_index];
                            if (use_high_pass) sample = _filters[channel][HIGH_PASS]->clock(sample);
                            if (use_low_pass)  sample = _filters[channel][LOW_PASS ]->clock(sample);
                            write[channel][sample_index] = static_cast<float>(sample);
                        }
                    }

                if (!use_high_pass && !use_low_pass && !use_tone)
                    buffer.clearActiveBufferRegion();
            }
            _locker.unlock();
        }
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(signal_)

};
