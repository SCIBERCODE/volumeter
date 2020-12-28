#pragma once
#include <JuceHeader.h>
#include <spuce/filters/iir.h>
#include <spuce/filters/butterworth_iir.h>
#include "main.h"

using namespace spuce;

extern unique_ptr<settings_> _opt;

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
    void set_sample_rate(double sample_rate) {
        sampling_rate = sample_rate;
    }
    void reset() {
        angles.fill(0.0);
    }
    void set_freq(float frequency) {
        freq = frequency;
        auto cycles_per_sample = frequency / sampling_rate;
        angle_delta = cycles_per_sample * 2.0 * 3.14159265358979323846;
    }
    auto sample(level_t channel) {
        auto current_sample = sin(angles.at(channel));
        angles.at(channel) += angle_delta;
        return static_cast<float>(current_sample);
    }

private:
    double           sampling_rate, freq, angle_delta;
    array<double, 2> angles;
};

//=========================================================================================
class circle_
//=========================================================================================
{
protected:
    struct iterator
    {
        size_t  size;
        size_t  pointer;
        level_t channel;
    };
private:
    array<unique_ptr<float[]>, 2> buffer;
    array<size_t, 2>              tail;
    size_t                        max_size;
    iterator                      it;
public:
    circle_(size_t max_size)
           : buffer({ make_unique<float[]>(max_size),
                      make_unique<float[]>(max_size) }),
             max_size(max_size)
    {
        clear();
    }

    void clear()
    {
        if (buffer.at(LEFT))  fill_n(buffer.at(LEFT).get(),  max_size, numeric_limits<float>::quiet_NaN());
        if (buffer.at(RIGHT)) fill_n(buffer.at(RIGHT).get(), max_size, numeric_limits<float>::quiet_NaN());
        tail.fill(0);
    }

    void enqueue(level_t channel, float value)
    {
        if (max_size && value)
        {
            buffer.at(channel)[tail.at(channel)] = value;
            tail.at(channel) = (tail.at(channel) + 1) % max_size;
        }
    }

    double get_rms(level_t channel)
    {
        double sum = 0.0;
        size_t size = 0;

        for (size_t k = 0; k < max_size; ++k)
        {
            auto sample = buffer.at(channel)[k];
            if (isfinite(sample) && sample) {
                sum += sample;
                size++;
            }
        }
        return sqrt(sum / size);
    }

    bool get_first_value(level_t channel, size_t size, float& value) {
        if (size == 0 || size > max_size)
            return false;

        it.pointer = tail.at(channel) - 1;
        it.size    = size;
        it.channel = channel;
        return get_next_value(value);
    }

    bool get_next_value(float& value) {
        value = numeric_limits<float>::quiet_NaN();
        if (it.pointer >= 0 && it.size)
        {
            value = buffer.at(it.channel)[it.pointer];
            if (it.pointer == 0)
                it.pointer = max_size - 1;
            else
                it.pointer--;

            it.size--;
        };
        if (!isfinite(value))
            it = { 0 };

        return isfinite(value);
    }
};


//=========================================================================================
class signal_
//=========================================================================================
{
public:

    signal_() : sample_rate(0.0), order(120)
    {
        zero.fill(0.0);
        minmax_clear();
    }

    void zero_set() {
        lock_guard<mutex> locker(audio_process);
        zero.at(LEFT)  = gain2db(buff->get_rms(LEFT));
        zero.at(RIGHT) = gain2db(buff->get_rms(RIGHT));
        minmax_clear();
    }
    auto zero_get(level_t channel) {
        return zero.at(channel);
    }
    void zero_clear() {
        zero.fill(0.0);
        minmax_clear();
    }

    auto minmax_set(const vector<double>& rms) {
        for (auto line = LEFT; line < LEVEL_SIZE; line++) // bug: на вольтах при переключении нижн€€ граница не похожа на правду
            if (isfinite(rms.at(line)))
            {
                if (!isfinite(minmax.at(line).at(MIN))) minmax.at(line).at(MIN) = rms.at(line);
                if (!isfinite(minmax.at(line).at(MAX))) minmax.at(line).at(MAX) = rms.at(line);
                minmax.at(line).at(MIN) = min(minmax.at(line).at(MIN), rms.at(line));
                minmax.at(line).at(MAX) = max(minmax.at(line).at(MAX), rms.at(line));
            }
    }
    auto minmax_get(level_t channel) {
        return minmax.at(channel);
    }
    void minmax_clear() {
        for (auto& sub : minmax)
            sub.fill(numeric_limits<float>::quiet_NaN());
    }

    double gain2db(double gain) {
        return 20 * log10(gain);
    }
    void set_freq(double freq) {
        osc.set_freq(static_cast<float>(freq));
        osc.reset();
    }
    void change_buff_size(int new_size) {
        lock_guard<mutex> locker(audio_process);
        if (new_size && sample_rate) {
            auto number_of_samples = static_cast<size_t>(new_size / (1000.0 / sample_rate));
            buff = make_unique<circle_>(number_of_samples);
        }
    }
    void clear_data() {
        lock_guard<mutex> locker(audio_process);
        if (buff)
            buff->clear();
    }
    void set_order(int new_order) {
        order = new_order;
    }
    vector<double> get_rms() {
        vector<double> result;
        if (buff)
        {
            result.push_back(buff->get_rms(LEFT));
            result.push_back(buff->get_rms(RIGHT));
        };
        return result;
    }

    void filter_init()
    {
        lock_guard<mutex> locker(audio_process);
        for (auto channel = LEFT; channel <= RIGHT; channel++)
        {
            if (_opt->load_int("use_pass_high"))
            {
                iir_coeff high_pass(order, filter_type::high);
                butterworth_iir(high_pass, 20.0 / sample_rate, 3.0);
                filter.at(channel).at(HIGH_PASS) = make_unique<iir<float_type, float_type>>(high_pass);
            }
            if (_opt->load_int("use_pass_low"))
            {
                iir_coeff low_pass(order, filter_type::low);
                butterworth_iir(low_pass, _opt->load_int("use_pass_low") / sample_rate, 3.0);
                filter.at(channel).at(LOW_PASS) = make_unique<iir<float_type, float_type>>(low_pass);
            }
        }
    };

    void prepare_to_play(double sample_rate_)
    {
        sample_rate = sample_rate_;

        osc.set_sample_rate(sample_rate);
        osc.set_freq(static_cast<float>(_opt->load_int("tone_value")));
        osc.reset();

        filter_init();
    }

    void next_audio_block(const AudioSourceChannelInfo& buffer)
    {
        if (audio_process.try_lock())
        {
            if (buff)
            {
                auto read          = buffer.buffer->getArrayOfReadPointers();
                auto write         = buffer.buffer->getArrayOfWritePointers();
                auto use_high_pass = _opt->load_int("use_pass_high") && filter.at(HIGH_PASS).at(LEFT) && filter.at(HIGH_PASS).at(RIGHT);
                auto use_low_pass  = _opt->load_int("use_pass_low")  && filter.at(LOW_PASS ).at(LEFT) && filter.at(LOW_PASS ).at(RIGHT);
                auto use_tone      = _opt->load_int("tone");

                for (auto channel = LEFT; channel <= RIGHT; channel++)
                    for (auto sample_index = 0; sample_index < buffer.numSamples; ++sample_index)
                    {
                        buff->enqueue(channel, read[channel][sample_index] * read[channel][sample_index]);
                        if (use_high_pass || use_low_pass || use_tone)
                        {
                            float_type sample = use_tone ? osc.sample(channel) : read[channel][sample_index];
                            if (use_high_pass) sample = filter.at(channel).at(HIGH_PASS)->clock(sample);
                            if (use_low_pass)  sample = filter.at(channel).at(LOW_PASS )->clock(sample);
                            write[channel][sample_index] = static_cast<float>(sample);
                        }                        
                    }

                if (!use_high_pass && !use_low_pass && !use_tone)
                    buffer.clearActiveBufferRegion();
            }
            audio_process.unlock();
        }
    }

private:
    double                     sample_rate;
    size_t                     order;
    mutex                      audio_process;
    sin_                       osc;
    unique_ptr<circle_>        buff;
    array<double, 2>           zero;                                    // [LEFT>RIGHT        ]
    array<array<double, 2>, 3> minmax;                                  // [LEFT>RIGHT>BALANCE][MIN>MAX]
    array<array<unique_ptr<iir<float_type, float_type>>, 2>, 2> filter; // [LEFT>RIGHT        ][HIGH_PASS>LOW_PASS]
};
