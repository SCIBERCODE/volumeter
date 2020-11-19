#include <JuceHeader.h>
#include "main.h"
#include "theme.h"

unique_ptr<ApplicationProperties> _settings;

class Application : public juce::JUCEApplication
{
public:
	Application()
	{
		_settings.reset(new ApplicationProperties());
		PropertiesFile::Options options;
		options.applicationName = "volumeter";
		options.filenameSuffix  = ".xml";
		options.folderName      = File::getCurrentWorkingDirectory().getFullPathName();
		options.storageFormat   = PropertiesFile::storeAsXML;
		_settings->setStorageParameters(options);
	}
	~Application() override
	{
		_settings.reset();
	}

	const juce::String getApplicationName() override    { return "volumeter";   }
	const juce::String getApplicationVersion() override { return "1.0.0";       }
	void shutdown() override                            { mainWindow = nullptr; }

	void initialise (const juce::String&) override
	{
		Desktop::getInstance().setDefaultLookAndFeel(&theme);
		mainWindow.reset(new MainWindow ("volumeter", new MainContentComponent, *this));
	}

private:
	class MainWindow : public juce::DocumentWindow
	{
	public:
		MainWindow (const juce::String& name, juce::Component* c, JUCEApplication& a)
			: DocumentWindow(name, theme::grey_level(239), juce::DocumentWindow::allButtons), app(a)
		{
			setUsingNativeTitleBar(false);
			setTitleBarTextCentred(true);
			setResizable(true, false);
			setContentOwned (c, true);
			setResizeLimits (300, 250, 10000, 10000);
			centreWithSize (getWidth(), getHeight());
			setVisible (true);
			toFront(true);
		}

		void closeButtonPressed() override
		{
			app.systemRequestedQuit();
		}

	private:
		JUCEApplication& app;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
	};
	std::unique_ptr<MainWindow> mainWindow;
	theme::light theme;
};

START_JUCE_APPLICATION (Application)