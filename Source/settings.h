#pragma once

class settings_ {
protected:
    struct option {
        String xml_name;
        String default_value;
    };

    const map<String, option> options = {
        { "combo_dev_type",     { "device_type",    "ASIO"  } },
        { "combo_dev_output",   { "device",         ""      } },
        { "combo_dev_rate",     { "sample_rate",    "48000" } },
        { "checkbox_high_pass", { "use_pass_high",  "0"     } },
        { "checkbox_low_pass",  { "use_pass_low",   "0"     } },
        { "slider_freq_high",   { "pass_low_value", "15000" } },
        { "combo_order",        { "order",          "120"   } },
        { "combo_buff_size",    { "buff_size",      "500"   } },
        { "checkbox_tone",      { "tone",           "0"     } },
        { "combo_tone",         { "tone_value",     "1000"  } },
        // calibrations component
        { "checkbox_cal",       { "calibrate",      "0"     } },
        { "combo_prefix",       { "prefix",         "0"     } },
        { "table_cals",         { "calibrations",   ""      } },
        { "table_cals_row",     { "cal_index",      "-1"    } }
    };

public:
    settings_()
    {
        PropertiesFile::Options params;
        params.applicationName = "volumeter";
        params.filenameSuffix  = ".xml";
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

    String load_text(const String& component_name) {
        if (key_exists(component_name))
            return settings.getUserSettings()->getValue
            (
                options.at(get_key(component_name)).xml_name,
                options.at(get_key(component_name)).default_value
            );

        return String();
    }

    int load_int(const String& component_name) {
        return key_exists(component_name) ? load_text(get_key(component_name)).getIntValue() : 0;
    }

    auto load_xml(const String& component_name) {
        if (key_exists(component_name))
            return settings.getUserSettings()->getXmlValue(options.at(get_key(component_name)).xml_name);

        return unique_ptr<XmlElement>{ };
    }

private:
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
