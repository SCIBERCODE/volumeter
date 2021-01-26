
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
        uint32_t  pointer;
        channel_t channel;
    };

    std::map<size_t, iterator_t> _sessions; // хэндлы перечисления значений get_first_value/get_next_value
    size_t                       _sessions_count { };
    std::unique_ptr<value_t[]>   _buffers[CHANNEL_SIZE];
    int32_t                      _heads  [CHANNEL_SIZE];
    const size_t                 _max_size;
    uint64_t                     _values_count { };
    bool                         _full { }; // как минимум единажды буфер был заполнен
//=================================================================================================
public:
    static const uint32_t INVALID_HANDLE = static_cast<uint32_t>(-1);

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
        for (auto channel = LEFT; channel < CHANNEL_SIZE; channel++) {
            _heads[channel] = -1;

            if (_buffers[channel])
                for (size_t k = 0; k < _max_size; k++)
                    _buffers[channel][k] = { _NAN<double> };
        }
    }

    void enqueue(const channel_t channel, const double value)
    {
        if (_max_size && isfinite(value) && _buffers[channel])
        {
            _heads[channel] = (_heads[channel] + 1) % _max_size;

            if (static_cast<size_t>(_heads[channel]) == _max_size - 1)
                _full = true;

            auto& new_value     = _buffers[channel][_heads[channel]];
            new_value.index     = _values_count++;
            new_value.value_raw = value;
        }
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

    /** начало цикла извлечения значений буфера, в дальнейшем вызывается get_next_value
        @return хэндл для дальнейшего использования в get_next_value
    */
    uint32_t get_first_value(const channel_t channel, const size_t size, double& value) { // T[35]
        if (size == 0 || size > _max_size || _heads[channel] == -1)
            return false;

        iterator_t new_it;

        auto handle       = _sessions_count++;
        new_it.pointer    = _heads[channel];
        new_it.size       = size;
        new_it.channel    = channel;
        _sessions[handle] = new_it;

        if (get_next_value(handle, value))
            return handle;

        return INVALID_HANDLE;
    }

    /** извлечение значений буфера
        @return о завершении сообщается возвратом false
    */
    bool get_next_value(const uint32_t handle, double& value) {
        auto result = _NAN<double>;

        if (_sessions.count(handle) == 0)
            return false;

        auto& it = _sessions.at(handle);
        if (it.size)
        {
            result = _buffers[it.channel][it.pointer].value_raw;
            it.pointer = it.pointer == 0 ?
                _max_size  - 1 :
                it.pointer - 1;

            it.size--;
        };
        if (isfinite(result))
        {
            value = result;
            return true;
        }
        else {
            _sessions.erase(handle);
            return false;
        }
    }

    auto get_extremes(const channel_t channel, const size_t window_size) {
        auto     result = std::make_unique<double[]>(EXTREMES_SIZE);
        double   value;
        uint32_t handle;

        handle = get_first_value(channel, window_size, value);
        if (handle != INVALID_HANDLE)
        {
            result[MIN] = value;
            result[MAX] = value;
            do {
                if (!isfinite(value)) break;
                result[MIN] = std::min(value, result[MIN]);
                result[MAX] = std::max(value, result[MAX]);
            }
            while (get_next_value(handle, value));
        }
        return result;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(circular_buffer)

};

