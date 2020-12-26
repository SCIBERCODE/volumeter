#pragma once

class settings_ {
protected:
    struct option {
        String xml_name;
        String default_value;
    };

    const map<String, option> options = {
        { "checkbox_bpfH",    { "use_bpfH",   "0"     } },
        { "checkbox_bpfL",    { "use_bpfL",   "0"     } },
        { "slider_freq_high", { "bpfL_value", "15000" } },
        { "combo_order",      { "order",      "120"   } },
        { "combo_buff_size",  { "buff_size",  "500"   } },
        { "checkbox_tone",    { "tone",       "0"     } },
        { "combo_tone",       { "tone_value", "1000"  } }
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
    void save(const String& key, T value) {
        settings.getUserSettings()->setValue(key, value);
        settings.saveIfNeeded();
    }

    auto load_str(const String& key, const String& default_value = "") {
        return settings.getUserSettings()->getValue(key, default_value);
    }

    auto load_int(const String& key, const String& default_value = "1000") {
        return load_str(key, default_value).getIntValue();
    }

    template <typename T>
    void save_(const String& component_name, T value) {
        if (options.count(component_name) > 0) {
            settings.getUserSettings()->setValue(options.at(component_name).xml_name, value);
            settings.saveIfNeeded();
        }
    }

    String load_text(const String& component_name) {
        if (options.count(component_name) == 0)
            return String();
        else
            return settings.getUserSettings()->getValue(
                options.at(component_name).xml_name,
                options.at(component_name).default_value
            );
    }

    int load_int_(const String& component_name) {
        if (options.count(component_name) == 0)
            return 0;
        else
            return load_text(component_name).getIntValue();
    }

    auto load_xml(const String& key) {
        return settings.getUserSettings()->getXmlValue(key);
    }

private:
    ApplicationProperties settings;
};
