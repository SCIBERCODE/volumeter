#pragma once

class settings_ {
protected:
    struct option_t {
        String xml_name;
        String default_value;
    };

    const multimap<String, option_t> options = {
        { L"combo_dev_type",     { L"device_type",        L"ASIO"  } },
        { L"combo_dev_output",   { L"device",             { }      } },
        { L"combo_dev_rate",     { L"sample_rate",        L"44100" } },
        { L"combo_buff_size",    { L"buff_size",          L"500"   } },
        { L"checkbox_tone",      { L"tone",               L"0"     } },
        { L"combo_tone",         { L"tone_value",         L"1000"  } },
        { L"button_pause_graph", { L"graph_paused",       L"0"     } },
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

public:
    settings_()
    {
        PropertiesFile::Options params;
        params.applicationName = L"volumeter";
        params.filenameSuffix  = L".xml";
        params.folderName      = File::getCurrentWorkingDirectory().getFullPathName();
        params.storageFormat   = PropertiesFile::storeAsXML;
        settings.setStorageParameters(params);
    }

    template <typename T>
    void save(const String& component_name, T value) {
        if (auto option = get_option(component_name)) {
            settings.getUserSettings()->setValue(option->xml_name, value);
            settings.saveIfNeeded();
        }
    }

    String get_text(const String& component_name) {
        if (auto option = get_option(component_name))
            return settings.getUserSettings()->getValue
            (
                option->xml_name,
                option->default_value
            );

        return String();
    }

    int get_int(const String& component_name) {
        auto str_value = get_text(component_name);
        return str_value.isNotEmpty() ? str_value.getIntValue() : 0;
    }

    auto get_xml(const String& component_name) {
        if (auto option = get_option(component_name))
            return settings.getUserSettings()->getXmlValue(option->xml_name);

        return unique_ptr<XmlElement>{ };
    }

private:
    // поиск как по ключам, так и по строкам конфига
    const option_t* get_option(const String& request) {
        auto find_key = options.find(request);

        if (find_key != options.end() && find_key->first.isNotEmpty())
            return &find_key->second;

        auto find = find_if(options.begin(), options.end(),
            [request](const auto& obj) { return obj.second.xml_name == request; });

        if (find != options.end())
            return &find->second;

        return { };
    }

    ApplicationProperties settings;
};
