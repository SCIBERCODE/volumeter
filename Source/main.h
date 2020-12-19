#pragma once
#include <JuceHeader.h>
#include <memory>
#include "theme.h"

namespace _s {
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
public:
	settings_()
	{
		PropertiesFile::Options options;
		options.applicationName = "volumeter";
		options.filenameSuffix  = ".xml";
		options.folderName      = File::getCurrentWorkingDirectory().getFullPathName();
		options.storageFormat   = PropertiesFile::storeAsXML;
		settings.setStorageParameters(options);
	}
	void save(const String& key, const var& value) {
		settings.getUserSettings()->setValue(key, value);
		settings.saveIfNeeded();
	}

	auto load_str(const String& key, const String& default_value = "") {
		return settings.getUserSettings()->getValue(key, default_value);
	}

	auto load_int(const String& key, const String& default_value = "1000") {
		return load_str(key, default_value).getIntValue();
	}

	auto load_xml(const String& key) {
		return settings.getUserSettings()->getXmlValue(key);
	}
private:
	ApplicationProperties settings;
};

