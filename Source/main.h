#pragma once
#include <JuceHeader.h>
#include <memory>
#include "theme.h"

namespace _magic {
	const size_t height = 20;
	const size_t margin = 5;
	const size_t label  = 75;
}

const map<int, String> _prefs = {
	{ -15,  "f" },
	{ -12,  "p" },
	{  -9,  "n" },
	{  -6, L"μ" },
	{  -3,  "m" },
	{   0,  ""  },
	{   3,  "k" },
	{   6,  "M" }
};

String prefix(double value, String unit, size_t numder_of_decimals);
String prefix_v(double value);

//=========================================================================================
class application_ : public JUCEApplication
{
public:
	application_();
	~application_() override;

	const String getApplicationName() override;
	const String getApplicationVersion() override;
	void shutdown() override;
	void initialise(const String&) override;
	bool moreThanOneInstanceAllowed() override;

protected:

	class main_window_ : public DocumentWindow
	{
	public:
		main_window_ (const String& name, Component* c, JUCEApplication& a)
			: DocumentWindow(name, theme::grey_level(239), DocumentWindow::allButtons), app(a)
		{
			setUsingNativeTitleBar(false);
			setTitleBarTextCentred(true);
			setResizable(true, false);
			setContentOwned(c, true);
			setResizeLimits(300, 250, 10000, 10000);
			centreWithSize(getWidth(), getHeight());
			setVisible(true);
			toFront(true);
		}

		void closeButtonPressed() override 
		{
			app.systemRequestedQuit();
		}

	private:
		JUCEApplication& app;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (main_window_)
	};

private:

	unique_ptr<main_window_> main_window;
	theme::light_            theme;
};

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