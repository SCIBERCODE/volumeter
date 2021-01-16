
class sine {
//=================================================================================================
private:
    double
        _sample_rate          { 44100.0 },
        _frequency            { },
        _angle_delta          { },
        _angles[CHANNEL_SIZE] { };

public:
    sine () { }
    ~sine() { }

    void set_sample_rate(const double sample_rate) {
        _sample_rate = sample_rate;
    }

    void reset() {
        for (auto& angle : _angles)
            angle = 0.0;
    }

    void set_freq(const float frequency)
    {
        _frequency   = frequency;
        _angle_delta = (frequency / _sample_rate) * 2.0 * M_PI;
    }

    auto sample(const channel_t channel)
    {
        auto new_sample = sin(_angles[channel]);
        _angles[channel] += _angle_delta;

        if (_angles[channel] > 2.0 * M_PI)
            _angles[channel] -= 2.0 * M_PI;

        return static_cast<float>(new_sample);
    }
};

class signal {
//=================================================================================================
private:
    double        _sample_rate;
    std::mutex    _locker;
    sine          _sine;
    std::unique_ptr<circular_buffer>
                  _buff;
    std::unique_ptr<spuce::iir<spuce::float_type, spuce::float_type>>
                  _filters [CHANNEL_SIZE][FILTER_TYPE_SIZE];
    size_t        _orders                [FILTER_TYPE_SIZE];
    double        _extremes[VOLUME_SIZE ][EXTREMES_SIZE   ];
    double        _zeros   [CHANNEL_SIZE];
    application & _app;
//=================================================================================================
public:

    signal(application& app) : _app(app)
    {
        _sample_rate       = _app.get_int(option_t::sample_rate    );
        _orders[HIGH_PASS] = _app.get_int(option_t::pass_high_order);
        _orders[LOW_PASS ] = _app.get_int(option_t::pass_low_order );

        zero_clear();
    }

    ~signal() { }

    void zero_set(
        double value_left  = _NAN<double>,
        double value_right = _NAN<double>)
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
                _app.save(option_t::zero_value_left,  _zeros[LEFT ]);
                _app.save(option_t::zero_value_right, _zeros[RIGHT]);
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
                column = _NAN<double>;
    }

    static double gain2db(const double gain) {
        return 20 * log10(gain);
    }

    void set_freq(const double freq) {
        _sine.set_freq(static_cast<float>(freq));
        _sine.reset();
    }

    void change_buff_size(const int new_size) {
        std::lock_guard<std::mutex> locker(_locker);
        if (new_size && _sample_rate) {
            auto number_of_samples = static_cast<size_t>(new_size / (1000.0 / _sample_rate));
            _buff = std::make_unique<circular_buffer>(number_of_samples);
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
                spuce::iir_coeff high_pass(_orders[filter_type], spuce::filter_type::high);
                butterworth_iir(high_pass, _app.get_int(option_t::pass_high_freq) / _sample_rate, 3.0);
                _filters[channel][HIGH_PASS] = std::make_unique<spuce::iir<spuce::float_type, spuce::float_type>>(high_pass);
            }
            if (filter_type == LOW_PASS)
            {
                spuce::iir_coeff low_pass(_orders[filter_type], spuce::filter_type::low);
                butterworth_iir(low_pass, _app.get_int(option_t::pass_low_freq) / _sample_rate, 3.0);
                _filters[channel][LOW_PASS] = std::make_unique<spuce::iir<spuce::float_type, spuce::float_type>>(low_pass);
            }
        }
    };

    void prepare_to_play(const double sample_rate)
    {
        _sample_rate = sample_rate;

        _sine.set_sample_rate(_sample_rate);
        _sine.set_freq(static_cast<float>(_app.get_int(option_t::tone_value)));
        _sine.reset();

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
                auto use_high_pass = _app.get_int(option_t::pass_high) && _filters[LEFT][HIGH_PASS] && _filters[RIGHT][HIGH_PASS];
                auto use_low_pass  = _app.get_int(option_t::pass_low ) && _filters[LEFT][LOW_PASS ] && _filters[RIGHT][LOW_PASS ];
                auto use_tone      = _app.get_int(option_t::tone     ); // todo: ускорить работу в этой процедуре

                for (auto channel = LEFT; channel <= RIGHT; channel++)
                    for (auto sample_index = 0; sample_index < buffer.numSamples; ++sample_index)
                    {
                        _buff->enqueue(channel, read[channel][sample_index] * read[channel][sample_index]);
                        if (use_high_pass || use_low_pass || use_tone)
                        {
                            spuce::float_type sample = use_tone ? _sine.sample(channel) : read[channel][sample_index];
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(signal)
};

