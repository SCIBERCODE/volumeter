#pragma once
#include <JuceHeader.h>
#include <spuce/filters/iir.h>
#include <spuce/filters/butterworth_iir.h>
#include "main.h"

using namespace spuce;

extern std::unique_ptr<settings_> __opt;

struct value_t
{
    double   value_raw;
    //float    value_db;
    //double   zero;
    uint64_t index;
};

//=================================================================================================
class sin_ {
//=================================================================================================
private:
    double
        _sample_rate,
        _frequency,
        _angle_delta,
        _angles[CHANNEL_SIZE];
public:
    sin_() : _sample_rate(44100.0),
             _angle_delta(0.0),
             _frequency  (0.0)
    {
        reset();
    }
    void set_sample_rate(const double sample_rate) {
        _sample_rate = sample_rate;
    }
    void reset() {
        for (auto& angle : _angles)
            angle = 0.0;
    }
    void set_freq(const float frequency) {
        _frequency = frequency;
        auto cycles_per_sample = frequency / _sample_rate;
        _angle_delta = cycles_per_sample * 2.0 * M_PI;
    }
    auto sample(const channel_t channel) {
        auto current_sample = sin(_angles[channel]);
        _angles[channel] += _angle_delta;

        if (_angles[channel] > 2.0 * M_PI)
            _angles[channel] -= 2.0 * M_PI;

        return static_cast<float>(current_sample);
    }
};

/** круговой буфер
*/
class circle_ {
//=================================================================================================
protected:
    struct iterator_t
    {
        size_t    size;
        size_t    pointer;
        channel_t channel;
        double    coeff;
        double    zero;
    };
private:
    std::unique_ptr<value_t[]> _buffers[CHANNEL_SIZE];
    size_t                     _tails  [CHANNEL_SIZE];
    const size_t               _max_size;
    iterator_t                 _it;
    uint64_t                   _index;
    bool                       _full; // как минимум единажды буфер был заполнен
    bool                       _volts;
public:
    circle_(const size_t max_size) :
        _max_size(max_size), _index(0), _full(false)
    {
        for (auto& buffer : _buffers)
            buffer = std::make_unique<value_t[]>(_max_size);

        clear();
    }

    void clear()
    {
        for (auto line = LEFT; line < CHANNEL_SIZE; line++) {
            if (_buffers[line])
                for (size_t k = 0; k < _max_size; k++)
                {
                    _buffers[line][k].value_raw = std::numeric_limits<double>::quiet_NaN();
                    _buffers[line][k].index     = 0;
                }

            _tails[line] = 0;
        }
    }

    void enqueue(const channel_t channel, const double value)
    {
        if (_max_size && isfinite(value) && _buffers[channel])
        {
            auto& new_value  = _buffers[channel][_tails[channel]];
            _tails[channel] = (_tails[channel] + 1) % _max_size;

            if (_tails[channel] == 0)
                _full = true;

            new_value.value_raw = value;
            new_value.index     = _index++;
        }
    }

    auto is_full () const { return _full;  }
    auto is_volts() const { return _volts; }

    auto get_tail(const channel_t channel) const {
        return _buffers[channel][_tails[channel]].value_raw;
    }

    bool get_rms(std::array<double, VOLUME_SIZE>& rms) const
    {
        double sum [CHANNEL_SIZE] { };
        size_t size[CHANNEL_SIZE] { };

        for (auto channel = LEFT; channel < CHANNEL_SIZE; channel++)
            for (size_t k = 0; k < _max_size; ++k)
            {
                auto value = _buffers[channel][k];
                if (isfinite(value.value_raw)) {
                    sum [channel] += value.value_raw;
                    size[channel]++;
                }
            }

        if (size[LEFT] && size[RIGHT] && _full) {
            rms[LEFT   ] = sqrt(sum[LEFT ] / size[LEFT ]);
            rms[RIGHT  ] = sqrt(sum[RIGHT] / size[RIGHT]);
            rms[BALANCE] = abs (sum[LEFT ] - sum[RIGHT]);
            return true;
        }
        else
            return false;
    }

    /** начало цикла извлечения значений буфера, в дальнейшем вызывается circle_::get_next_value
        @param index уникальный идентификатор значения в буфере
    */
    bool get_first_value(const channel_t channel, const size_t size, double& value) {
        if (size == 0 || size > _max_size || _tails[channel] == 0)/* bug: исправить логику */
            return false;

        _it.pointer = _tails[channel] - 1;
        _it.size    = size;
        _it.channel = channel;

        _it.coeff   = std::numeric_limits<double>::quiet_NaN();
        _it.zero    = std::numeric_limits<double>::quiet_NaN();

        size_t cal_index = __opt->get_int(L"calibrations_index");
        if (cal_index != -1 && __opt->get_int(L"calibrate"))
        {
            size_t k = 0;
            if (auto cals = __opt->get_xml(L"calibrations")) {
                forEachXmlChildElement(*cals, el)
                {
                    if (k == cal_index) {
                        if (_it.channel == LEFT)
                            _it.coeff = el->getDoubleAttribute(Identifier(L"left_coeff"));
                        else
                            _it.coeff = el->getDoubleAttribute(Identifier(L"right_coeff"));
                        break;
                    }
                    k++;
                }
            }
        }

        if (__opt->get_int(L"zero"))
            if (_it.channel == LEFT)
                _it.zero = __opt->get_text(L"zero_value_left").getDoubleValue();
            else
                _it.zero = __opt->get_text(L"zero_value_right").getDoubleValue();

        _volts = isfinite(_it.coeff);
        return get_next_value(value);
    }

    /** извлечение значений буфера
        @return о завершении сообщается возвратом false
    */
    bool get_next_value(double& value) {
        value_t result;

        result.value_raw = std::numeric_limits<double>::quiet_NaN();
        result.index     = 0;

        if (_it.pointer >= 0 && _it.size)
        {
            result = _buffers[_it.channel][_it.pointer];
            if (_it.pointer == 0)
                _it.pointer = _max_size - 1;
            else
                _it.pointer--;

            _it.size--;
        };
        if (!isfinite(result.value_raw))
            _it = { };
        else {
            // bug: не менять данные графика, всю работу производить в get_extremes
            if (isfinite(_it.coeff))
                value = result.value_raw * _it.coeff;
            else {
                value = 20 * log10(result.value_raw);
                if (isfinite(_it.zero))
                    value -= _it.zero; // bug: подскакивает граф, не норм
            }
        }

        return isfinite(result.value_raw);
    }

    auto get_extremes(const channel_t channel, const size_t size) {
        auto   result = std::make_unique<double[]>(EXTREMES_SIZE);
        double value;

        if (get_first_value(channel, size, value)) {
            result[MIN] = value;
            result[MAX] = value;
            do {
                if (!isfinite(value)) break;
                result[MIN] = std::min(value, result[MIN]);
                result[MAX] = std::max(value, result[MAX]);
            }
            while (get_next_value(value));
        }
       return result;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(circle_)

};

//=================================================================================================
class signal_ {
//=================================================================================================
private:
    double                   _sample_rate;
    std::mutex               _locker;
    sin_                     _sin;
    std::unique_ptr<circle_> _buff;
    std::unique_ptr<iir<float_type, float_type>>
                             _filters [CHANNEL_SIZE][FILTER_TYPE_SIZE];
    size_t                   _orders                [FILTER_TYPE_SIZE];
    double                   _extremes[VOLUME_SIZE ][EXTREMES_SIZE   ];
    double                   _zeros   [CHANNEL_SIZE];
public:

    signal_()
    {
        _sample_rate       = __opt->get_int(L"sample_rate"    );
        _orders[HIGH_PASS] = __opt->get_int(L"pass_high_order");
        _orders[LOW_PASS ] = __opt->get_int(L"pass_low_order" );

        zero_clear();
    }

    void zero_set(
        double value_left  = std::numeric_limits<double>::quiet_NaN(),
        double value_right = std::numeric_limits<double>::quiet_NaN())
    {
        std::lock_guard<std::mutex> locker(_locker);
        if (isfinite(value_left) && isfinite(value_right)) {
            _zeros[LEFT ] = value_left;
            _zeros[RIGHT] = value_right;
        }
        else {
            std::array<double, VOLUME_SIZE> rms;
            if (_buff->get_rms(rms))
            {
                _zeros[LEFT ] = gain2db(rms[LEFT ]);
                _zeros[RIGHT] = gain2db(rms[RIGHT]);
                __opt->save(L"zero_value_left",  _zeros[LEFT ]);
                __opt->save(L"zero_value_right", _zeros[RIGHT]);
            }
        }
        extremes_clear();
    }
    auto zero_get(const channel_t channel) const {
        return _zeros[channel];
    }
    void zero_clear() {
        for (auto& zero : _zeros) zero = 0.0;
        extremes_clear();
    }

    void extremes_set(const std::vector<double>& rms) {
        for (size_t line = LEFT; line < VOLUME_SIZE; line++)
            if (isfinite(rms.at(line)))
            {
                if (!isfinite(_extremes[line][MIN])) _extremes[line][MIN] = rms.at(line);
                if (!isfinite(_extremes[line][MAX])) _extremes[line][MAX] = rms.at(line);
                _extremes[line][MIN] = std::min(_extremes[line][MIN], rms.at(line));
                _extremes[line][MAX] = std::max(_extremes[line][MAX], rms.at(line));
            }
    }
    auto extremes_get(const size_t channel) const {
        return _extremes[channel];
    }
    void extremes_clear() {
        for (auto& line : _extremes)
            for (auto& column : line)
                column = std::numeric_limits<double>::quiet_NaN();
    }

    static double gain2db(const double gain) {
        return 20 * log10(gain);
    }
    void set_freq(const double freq) {
        _sin.set_freq(static_cast<float>(freq));
        _sin.reset();
    }
    void change_buff_size(const int new_size) {
        std::lock_guard<std::mutex> locker(_locker);
        if (new_size && _sample_rate) {
            auto number_of_samples = static_cast<size_t>(new_size / (1000.0 / _sample_rate));
            _buff = std::make_unique<circle_>(number_of_samples);
        }
    }
    void clear_data() {
        std::lock_guard<std::mutex> locker(_locker);
        if (_buff)
            _buff->clear();
    }
    void set_order(const filter_type_t filter_type, const int new_order) {
        _orders[filter_type] = new_order;
    }
    std::vector<double> get_rms() const {
        std::array<double, VOLUME_SIZE> result;
        if (_buff && _buff->get_rms(result))
        {
            return { result[LEFT], result[RIGHT] };
        };
        return {};
    }

    void filter_init(const filter_type_t filter_type)
    {
        std::lock_guard<std::mutex> locker(_locker);
        for (auto channel = LEFT; channel <= RIGHT; channel++)
        {
            if (filter_type == HIGH_PASS)
            {
                iir_coeff high_pass(_orders[filter_type], filter_type::high);
                butterworth_iir(high_pass, __opt->get_int(L"pass_high_freq") / _sample_rate, 3.0);
                _filters[channel][HIGH_PASS] = std::make_unique<iir<float_type, float_type>>(high_pass);
            }
            if (filter_type == LOW_PASS)
            {
                iir_coeff low_pass(_orders[filter_type], filter_type::low);
                butterworth_iir(low_pass, __opt->get_int(L"pass_low_freq") / _sample_rate, 3.0);
                _filters[channel][LOW_PASS] = std::make_unique<iir<float_type, float_type>>(low_pass);
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
        filter_init(LOW_PASS );
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
