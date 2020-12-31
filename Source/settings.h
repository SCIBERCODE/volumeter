#pragma once

class settings_ {
protected:
    struct option {
        String xml_name;
        String default_value;
    };

    const map<String, option> options = {
        { L"combo_dev_type",     { L"device_type",        L"ASIO"  } },
        { L"combo_dev_output",   { L"device",             L""      } },
        { L"combo_dev_rate",     { L"sample_rate",        L"44100" } },
        { L"checkbox_high_pass", { L"pass_high",          L"0"     } },
        { L"checkbox_low_pass",  { L"pass_low",           L"0"     } },
        { L"slider_freq_high",   { L"pass_low_value",     L"15000" } },
        { L"combo_order",        { L"order",              L"120"   } },
        { L"combo_buff_size",    { L"buff_size",          L"500"   } },
        { L"checkbox_tone",      { L"tone",               L"0"     } },
        { L"combo_tone",         { L"tone_value",         L"1000"  } },
        // calibrations component
        { L"checkbox_cal",       { L"calibrate",          L"0"     } },
        { L"combo_prefix",       { L"prefix",             L"0"     } },
        { L"table_cals",         { L"calibrations",       L""      } },
        { L"table_cals_row",     { L"calibrations_index", L"-1"    } },

        { L"checkbox_filter",    { L"filter",             L"0"     } }
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
        if (key_exists(component_name)) {
            settings.getUserSettings()->setValue(options.at(get_key(component_name)).xml_name, value);
            settings.saveIfNeeded();
        }
    }

    String get_text(const String& component_name) {
        if (key_exists(component_name))
            return settings.getUserSettings()->getValue
            (
                options.at(get_key(component_name)).xml_name,
                options.at(get_key(component_name)).default_value
            );

        return String();
    }

    int get_int(const String& component_name) {
        return key_exists(component_name) ? get_text(get_key(component_name)).getIntValue() : 0;
    }

    auto get_xml(const String& component_name) {
        if (key_exists(component_name))
            return settings.getUserSettings()->getXmlValue(options.at(get_key(component_name)).xml_name);

        return unique_ptr<XmlElement>{ };
    }

private:
    // ищем по имени контрола, потому как преимущественно настройки запрашивает гуй
    // после глядим в именах опций непосредственно использующихся в файле конфига
    String get_key(const String& request) {
        if (options.count(request) > 0)
            return request;

        auto find = find_if(options.begin(), options.end(),
            [request](const auto& obj) { return obj.second.xml_name == request; });

        if (find != options.end())
            return find->first;

        return String();
    }

    bool key_exists(const String& request) {
        return get_key(request).length() > 0;
    }

    ApplicationProperties settings;
};
