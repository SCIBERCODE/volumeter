
class sine {
//=================================================================================================
private:
    double
        _sample_rate                { 44100.0 },
        _frequency                  { },
        _angle_delta                { },
        _angles[+channel_t::__size] { };

public:
     sine() { }
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
        auto new_sample = sin(_angles[+channel]);
        _angles[+channel] += _angle_delta;

        if (_angles[+channel] > 2.0 * M_PI)
            _angles[+channel] -= 2.0 * M_PI;

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
                  _buff    [+graph_type_t::__size];
    std::unique_ptr<spuce::iir<spuce::float_type, spuce::float_type>>
                  _filters [+channel_t   ::__size][+filter_type_t::__size];
    size_t        _orders                         [+filter_type_t::__size];
    double        _extremes[+volume_t    ::__size][+extremum_t   ::__size];
    double        _zeros   [+channel_t   ::__size];
    application & _app;
//=================================================================================================
public:

    signal(application& app) : _app(app)
    {
        _sample_rate                       = _app.get_int(option_t::sample_rate    );
        _orders[+filter_type_t::HIGH_PASS] = _app.get_int(option_t::pass_high_order);
        _orders[+filter_type_t::LOW_PASS ] = _app.get_int(option_t::pass_low_order );
        zeros_clear();
    }

    ~signal() { }

    void zeros_set(
        double value_left  = _NAN<double>,
        double value_right = _NAN<double>
    ) {
        std::lock_guard<std::mutex> locker(_locker);
        if (isfinite(value_left) && isfinite(value_right)) {
            _zeros[+channel_t::LEFT ] = value_left;
            _zeros[+channel_t::RIGHT] = value_right;
        }
        else {
            double rms;
            for (const auto& channel : channel_t())
                if (_buff[+graph_type_t::FILTERED]->get_rms(channel, rms))
                {
                    _zeros[+channel] = rms;
                    _app.save(
                        channel == channel_t::LEFT ? option_t::zero_value_left : option_t::zero_value_right,
                        _zeros[+channel]
                    );
                }
        }
        extremes_clear();
    }

    auto zeros_get(const channel_t channel) const {
        return _zeros[+channel];
    }

    void zeros_clear() {
        for (auto& zero : _zeros) zero = 0.0;
        extremes_clear();
    }

    void extremes_set(const std::vector<double>& rms) {
        for (auto const& line_type : volume_t())
            if (isfinite(rms.at(+line_type)))
            {
                auto line = +line_type;
                for (auto const& extremum : extremum_t())
                    if (!isfinite(_extremes[line][+extremum])) _extremes[line][+extremum] = rms.at(line);

                _extremes[line][+extremum_t::MIN] = std::min(_extremes[line][+extremum_t::MIN], rms.at(line));
                _extremes[line][+extremum_t::MAX] = std::min(_extremes[line][+extremum_t::MAX], rms.at(line));
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
            _buff[+graph_type_t::FILTERED] = std::make_unique<circular_buffer>(number_of_samples);
            _buff[+graph_type_t::RAW     ] = std::make_unique<circular_buffer>(number_of_samples);
        }
    }

    void clear_data() {
        std::lock_guard<std::mutex> locker(_locker);
        if (_buff[+graph_type_t::FILTERED]) _buff[+graph_type_t::FILTERED]->clear();
        if (_buff[+graph_type_t::RAW     ]) _buff[+graph_type_t::RAW     ]->clear();
    }

    void set_order(const filter_type_t filter_type, const int new_order) {
        _orders[+filter_type] = new_order;
    }

    std::vector<double> get_rms(graph_type_t type) const {
        std::vector<double> result;
        double rms;

        for (auto const& channel : channel_t())
            if (_buff[+type] && _buff[+type]->get_rms(channel, rms))
                result.push_back(rms);

        if (result.size() == +channel_t::__size)
            return result;

        return { };
    }

    void filter_init(const filter_type_t filter_type)
    {
        std::lock_guard<std::mutex> locker(_locker);
        for (auto const& channel : channel_t())
        {
            if (filter_type == filter_type_t::HIGH_PASS)
            {
                spuce::iir_coeff high_pass(_orders[+filter_type], spuce::filter_type::high);
                butterworth_iir(high_pass, _app.get_int(option_t::pass_high_freq) / _sample_rate, 3.0);
                _filters[+channel][+filter_type_t::HIGH_PASS] = std::make_unique<spuce::iir<spuce::float_type, spuce::float_type>>(high_pass);
            }
            if (filter_type == filter_type_t::LOW_PASS)
            {
                spuce::iir_coeff low_pass(_orders[+filter_type], spuce::filter_type::low);
                butterworth_iir(low_pass, _app.get_int(option_t::pass_low_freq) / _sample_rate, 3.0);
                _filters[+channel][+filter_type_t::LOW_PASS] = std::make_unique<spuce::iir<spuce::float_type, spuce::float_type>>(low_pass);
            }
        }
    };

    void prepare_to_play(const double sample_rate)
    {
        _sample_rate = sample_rate;

        _sine.set_sample_rate(_sample_rate);
        _sine.set_freq(static_cast<float>(_app.get_int(option_t::tone_value)));
        _sine.reset();

        filter_init(filter_type_t::HIGH_PASS);
        filter_init(filter_type_t::LOW_PASS );
    }

    void next_audio_block(const AudioSourceChannelInfo& buffer)
    {
        if (_locker.try_lock()) // T[19]
        {
            if (_buff[+graph_type_t::FILTERED] && _buff[+graph_type_t::RAW])
            {
                auto read          = buffer.buffer->getArrayOfReadPointers();
                auto write         = buffer.buffer->getArrayOfWritePointers();
                auto use_high_pass = _app.get_int(option_t::pass_high) && _filters[+channel_t::LEFT][+filter_type_t::HIGH_PASS] && _filters[+channel_t::RIGHT][+filter_type_t::HIGH_PASS];
                auto use_low_pass  = _app.get_int(option_t::pass_low ) && _filters[+channel_t::LEFT][+filter_type_t::LOW_PASS ] && _filters[+channel_t::RIGHT][+filter_type_t::LOW_PASS ];
                auto use_tone      = static_cast<bool>(_app.get_int(option_t::tone)); // T[20]

                for (auto const& channel : channel_t()) {
                    for (auto sample_index = 0; sample_index < buffer.numSamples; ++sample_index)
                    {
                        spuce::float_type sample_in = read[+channel][sample_index];
                        auto mag = sample_in * sample_in;

                        _buff[+graph_type_t::RAW]->enqueue(channel, mag);
                        if (use_high_pass) sample_in = _filters[+channel][+filter_type_t::HIGH_PASS]->clock(sample_in);
                        if (use_low_pass ) sample_in = _filters[+channel][+filter_type_t::LOW_PASS ]->clock(sample_in);

                        _buff[+graph_type_t::FILTERED]->enqueue(channel, 
                            use_high_pass || use_low_pass ? (sample_in * sample_in) : mag);

                        if (use_tone)
                            write[+channel][sample_index] = _sine.sample(channel);
                    }
                }
                if (!use_tone)
                    buffer.clearActiveBufferRegion();
            }
            _locker.unlock();
        }
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(signal)
};

