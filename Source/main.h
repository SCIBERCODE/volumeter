
const float __buff_size_list_sec[] { 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 30.0f };
const int   __tone_list         [] { 10, 20, 200, 500, 1000, 2000, 5000, 10000, 20000 };

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
protected:
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

    struct option_t
    {
        String xml_name;
        String default_value;
    };

    const std::multimap<String, option_t> options {
        { L"combo_dev_type",     { L"device_type",        L"ASIO"  } },
        { L"combo_dev_output",   { L"device",             { }      } },
        { L"combo_dev_rate",     { L"sample_rate",        L"44100" } },
        { L"combo_buff_size",    { L"buff_size",          L"500"   } },
        { L"checkbox_tone",      { L"tone",               L"0"     } },
        { L"combo_tone",         { L"tone_value",         L"1000"  } },
        { L"button_zero",        { L"zero",               L"0"     } },
        { { },                   { L"zero_value_left",    L"0"     } },
        { { },                   { L"zero_value_right",   L"0"     } },
        // график
        { L"button_pause_graph", { L"graph_paused",       L"0"     } },
        { { },                   { L"graph_left",         L"1"     } },
        { { },                   { L"graph_right",        L"0"     } },
        // калибровка
        { L"checkbox_cal",       { L"calibrate",          L"0"     } },
        { L"combo_prefix",       { L"prefix",             L"0"     } },
        { { },                   { L"calibrations",       { }      } },
        { { },                   { L"calibrations_index", L"-1"    } },
        // фильтры
        { { },                   { L"pass_high",          L"0"     } }, // todo: объединить в файле конфигурации
        { { },                   { L"pass_low",           L"0"     } },
        { { },                   { L"pass_high_freq",     { }      } },
        { { },                   { L"pass_low_freq",      { }      } },
        { { },                   { L"pass_high_order",    L"120"   } },
        { { },                   { L"pass_low_order",     L"120"   } },
    };

private:
    std::unique_ptr<main_window_>  _main_window;
    std::unique_ptr<theme::light_> _theme;
    ApplicationProperties          _settings;

    /** поиск как по ключам, так и по строкам конфига, иначе говоря,
        первые два столбца settings_::options имеют одинаковый вес при поиске,
        но в порядке очерёдности
    */
    const option_t* get_option(const String& request) const {
        auto find_key = options.find(request);

        if (find_key != options.end() && find_key->first.isNotEmpty())
            return &find_key->second;

        auto find = find_if(options.begin(), options.end(),
            [request](const auto& obj) { return obj.second.xml_name == request; });

        if (find != options.end())
            return &find->second;

        return { };
    }
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
    void save(const String& component_name, T value) {
        if (auto option = get_option(component_name)) {
            _settings.getUserSettings()->setValue(option->xml_name, value);
            _settings.saveIfNeeded();
        }
    }

    const String get_text(const String& component_name) {
        if (auto option = get_option(component_name))
            return _settings.getUserSettings()->getValue
            (
                option->xml_name,
                option->default_value
            );

        return String();
    }

    const int get_int(const String& component_name) {
        auto str_value = get_text(component_name);
        return str_value.isNotEmpty() ? str_value.getIntValue() : 0;
    }

    const auto get_xml(const String& component_name) {
        if (auto option = get_option(component_name))
            return _settings.getUserSettings()->getXmlValue(option->xml_name);

        return std::unique_ptr<XmlElement>{ };
    }
};

