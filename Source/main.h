
enum waiting_event_t
{
    device_init,
    buffer_fill
};

const std::map<int, String> __prefs {
    { -15, L"f"      },
    { -12, L"p"      },
    {  -9, L"n"      },
    {  -6, L"\u00b5" },
    {  -3, L"m"      },
    {   0, L""       },
    {   3, L"k"      },
    {   6, L"M"      }
};

enum channel_t : size_t {
    LEFT,
    RIGHT,
    CHANNEL_SIZE
};

enum volume_t : size_t {
    BALANCE = CHANNEL_SIZE,
    VOLUME_SIZE
};

const std::map<size_t, String> __channel_name {
    { LEFT,  L"Left"  },
    { RIGHT, L"Right" }
};

enum extremum_t : size_t {
    MIN,
    MAX,
    EXTREMES_SIZE
};

enum filter_type_t : size_t {
    HIGH_PASS,
    LOW_PASS,
    FILTER_TYPE_SIZE
};

enum labels_stat_column_t : size_t {
    LABEL,
    VALUE,
    EXTREMES,
    LABELS_STAT_COLUMN_SIZE
};

enum class option_t {
    device_type,
    device_name,
    sample_rate,
    buff_size,
    tone,
    tone_value,
    zero,
    zero_value_left,
    zero_value_right,
    graph_paused,
    graph_left,
    graph_right,
    calibrate,
    prefix,
    calibrations,
    calibrations_index,
    pass_high,
    pass_low,
    pass_high_freq,
    pass_low_freq,
    pass_high_order,
    pass_low_order,
};

template <typename T>
void operator ++(T& value, int) // todo: проверить на вызовы с другими типами помимо перечислений, описанных выше
{
    value = static_cast<T>(value + 1);
}

void operator --(channel_t& value, int)
{
    value = value == 0 ? CHANNEL_SIZE : static_cast<channel_t>(value - 1);
}

//=================================================================================================
const String prefix  (double value, const wchar_t *unit, size_t numder_of_decimals);
const String prefix_v(double value);

const bool is_about_equal(float a, float b);
const bool round_flt     (float value);

class main_component_;

//=================================================================================================
class application_ : public JUCEApplication {
//=================================================================================================
private:
    class main_window_ : public DocumentWindow
    {
    public:
        main_window_(const String& name, application_& app);
        void closeButtonPressed() override;

    private:
        application_                   & _app;
        std::unique_ptr<main_component_> _content;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(main_window_)
    };

    struct option_names_t
    {
        const String& xml_name;
        const String& default_value;
    };

    const std::map<option_t, option_names_t> _options
    {
        { option_t::device_type,        { L"device_type",        L"ASIO"  } }, // combo_dev_type
        { option_t::device_name,        { L"device",             { }      } }, // combo_dev_output
        { option_t::sample_rate,        { L"sample_rate",        L"44100" } }, // combo_dev_rate
        { option_t::buff_size,          { L"buff_size",          L"500"   } }, // combo_buff_size
        { option_t::tone,               { L"tone",               L"0"     } }, // checkbox_tone
        { option_t::tone_value,         { L"tone_value",         L"1000"  } }, // combo_tone
        { option_t::zero,               { L"zero",               L"0"     } }, // button_zero
        { option_t::zero_value_left,    { L"zero_value_left",    L"0"     } },
        { option_t::zero_value_right,   { L"zero_value_right",   L"0"     } },
        // график
        { option_t::graph_paused,       { L"graph_paused",       L"0"     } }, // button_pause_graph
        { option_t::graph_left,         { L"graph_left",         L"1"     } },
        { option_t::graph_right,        { L"graph_right",        L"0"     } },
        // калибровка
        { option_t::calibrate,          { L"calibrate",          L"0"     } }, // checkbox_cal
        { option_t::prefix,             { L"prefix",             L"0"     } }, // combo_prefix
        { option_t::calibrations,       { L"calibrations",       { }      } },
        { option_t::calibrations_index, { L"calibrations_index", L"-1"    } },
        // фильтры
        { option_t::pass_high,          { L"pass_high",          L"0"     } }, // todo: объединить в файле конфигурации
        { option_t::pass_low,           { L"pass_low",           L"0"     } },
        { option_t::pass_high_freq,     { L"pass_high_freq",     { }      } },
        { option_t::pass_low_freq,      { L"pass_low_freq",      { }      } },
        { option_t::pass_high_order,    { L"pass_high_order",    L"120"   } },
        { option_t::pass_low_order,     { L"pass_low_order",     L"120"   } },
    };

    std::unique_ptr<main_window_>  _main_window;
    std::unique_ptr<theme::light_> _theme;
    ApplicationProperties          _settings;

    const option_names_t* find_xml_name(const option_t request) const
    {
        if (_options.count(request))
            return &_options.at(request);

        return nullptr;
    }
//=================================================================================================
public:
    application_();
    ~application_() override;

    const String getApplicationName() override;
    const String getApplicationVersion() override;
    void  shutdown() override;
    void  initialise(const String&) override;
    bool  moreThanOneInstanceAllowed() override;

    theme::light_ *get_theme() const;

    template <typename T>
    void save(const option_t option_name, T value) {
        if (auto option = find_xml_name(option_name))
            _settings.getUserSettings()->setValue(option->xml_name, value);
    }

    const String get_text(const option_t option_name) {
        if (auto option = find_xml_name(option_name))
            return _settings.getUserSettings()->getValue
            (
                option->xml_name,
                option->default_value
            );

        return String();
    }

    const int get_int(const option_t option_name) {
        auto text_value = get_text(option_name);
        return text_value.isNotEmpty() ? text_value.getIntValue() : 0;
    }

    auto get_xml(const option_t option_name) {
        if (auto option = find_xml_name(option_name))
            return _settings.getUserSettings()->getXmlValue(option->xml_name);

        return std::unique_ptr<XmlElement>{ };
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(application_)
};

