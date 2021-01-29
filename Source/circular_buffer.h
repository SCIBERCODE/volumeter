
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

    std::map<size_t, iterator_t> _sessions;           // value enumeration handles used in get_first_value/get_next_value
    size_t                       _sessions_count { };
    uint64_t                     _values_count   { };
    bool                         _full           { }; // the buffer has been filled at least once
    std::unique_ptr<value_t[]>   _buffers[+channel_t::__size];
    int32_t                      _heads  [+channel_t::__size];
    const size_t                 _max_size;
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
        for (const auto& channel : channel_t()) {
            _heads[+channel] = -1;

            if (_buffers[+channel])
                for (size_t k = 0; k < _max_size; k++)
                    _buffers[+channel][k] = { _NAN<double> };
        }
    }

    void enqueue(const channel_t channel, const double value)
    {
        if (_max_size && isfinite(value) && _buffers[+channel])
        {
            _heads[+channel] = (_heads[+channel] + 1) % _max_size;

            if (static_cast<size_t>(_heads[+channel]) == _max_size - 1)
                _full = true;

            auto& new_value     = _buffers[+channel][_heads[+channel]];
            new_value.index     = _values_count++;
            new_value.value_raw = value;
        }
    }

    bool get_rms(const channel_t channel, double& value) const
    {
        if (!_full) return false;

        double sum  { };
        size_t size { };
        for (size_t k = 0; k < _max_size; ++k)
        {
            const auto& sample = _buffers[+channel][k];
            if (isfinite(sample.value_raw)) {
                sum += sample.value_raw;
                size++;
            }
        }
        if (size) {
            value = sqrt(sum / size);
            return true;
        }
        else
            return false;
    }

    /** retrieving values in reverse order
        @param size the maximum number of values to be read
        @return values enumeration handle used in a subsequent call to get_next_value
    */
    uint32_t get_first_value(const channel_t channel, const size_t size, double& value) { // T[35]
        if (size == 0 || size > _max_size || _heads[+channel] == -1)
            return false;

        iterator_t new_it;

        auto handle       = _sessions_count++;
        new_it.pointer    = _heads[+channel];
        new_it.size       = size;
        new_it.channel    = channel;
        _sessions[handle] = new_it;

        if (get_next_value(handle, value))
            return handle;

        return INVALID_HANDLE;
    }

    /** continues a value retrieving from a previous call to the get_first_value
        @return the function fails when there is no more data left to retrieve
    */
    bool get_next_value(const uint32_t handle, double& value) {
        auto result = _NAN<double>;

        if (_sessions.count(handle) == 0)
            return false;

        auto& it = _sessions.at(handle);
        if (it.size)
        {
            result = _buffers[+it.channel][it.pointer].value_raw;
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
        auto     result = std::make_unique<double[]>(+extremum_t::__size);
        double   value;
        uint32_t handle;

        handle = get_first_value(channel, window_size, value);
        if (handle != INVALID_HANDLE)
        {
            result[+extremum_t::MIN] = value;
            result[+extremum_t::MAX] = value;
            do {
                if (!isfinite(value)) break;
                result[+extremum_t::MIN] = std::min(value, result[+extremum_t::MIN]);
                result[+extremum_t::MAX] = std::max(value, result[+extremum_t::MAX]);
            }
            while (get_next_value(handle, value));
        }
        return result;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(circular_buffer)

};

