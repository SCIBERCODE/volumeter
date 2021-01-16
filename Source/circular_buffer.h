
class circular_buffer {
//=================================================================================================
private:
    struct value_t
    {
        double   value_raw;
        uint64_t index;
    };

    struct iterator_t
    {
        size_t    size;
        size_t    pointer;
        channel_t channel;
    };

    std::unique_ptr<value_t[]> _buffers[CHANNEL_SIZE];
    size_t                     _tails  [CHANNEL_SIZE];
    const size_t               _max_size;
    iterator_t                 _it;
    uint64_t                   _index { };
    bool                       _full  { }; // как минимум единажды буфер был заполнен
//=================================================================================================
public:
    circular_buffer(const size_t max_size) :
        _max_size(max_size)
    {
        for (auto& buffer : _buffers)
            buffer = std::make_unique<value_t[]>(_max_size);

        clear();
    }

    ~circular_buffer() { }

    void clear()
    {
        for (auto line = LEFT; line < CHANNEL_SIZE; line++) {
            if (_buffers[line])
                for (size_t k = 0; k < _max_size; k++)
                {
                    _buffers[line][k].value_raw = _NAN<double>;
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

    auto is_full () const {
        return _full;
    }

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

    /** начало цикла извлечения значений буфера, в дальнейшем вызывается circular_buffer::get_next_value
        @param index уникальный идентификатор значения в буфере
    */
    bool get_first_value(const channel_t channel, const size_t size, double& value) { // todo: объединить функции в одну
        if (size == 0 || size > _max_size || /* bug: исправить логику */ _tails[channel] == 0)
            return false;

        _it.pointer = _tails[channel] - 1;
        _it.size    = size;
        _it.channel = channel;
        return get_next_value(value);
    }

    /** извлечение значений буфера
        @return о завершении сообщается возвратом false
    */
    bool get_next_value(double& value) {
        value = _NAN<double>;
        if (_it.pointer >= 0 && _it.size)
        {
            value = _buffers[_it.channel][_it.pointer].value_raw;
            if (_it.pointer == 0)
                _it.pointer = _max_size - 1;
            else
                _it.pointer--;

            _it.size--;
        };
        if (!isfinite(value))
            _it = { };

        return isfinite(value);
    }

    auto get_extremes(const channel_t channel, const size_t window_size) {
        auto   result = std::make_unique<double[]>(EXTREMES_SIZE);
        double value;

        if (get_first_value(channel, window_size, value)) {
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(circular_buffer)

};

