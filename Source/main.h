#pragma once
#include <JuceHeader.h>
#include <memory>
#include "theme.h"
#include "settings.h"

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

enum level_t : size_t {
    LEFT = 0,
    RIGHT,
    BALANCE,
    LEVEL_SIZE
};

enum stat_t : size_t {
    MIN = 0,
    MAX,
    STAT_SIZE
};

enum filter_type_t : size_t {
    HIGH_PASS = 0,
    LOW_PASS
};

enum labels_stat_column_t : size_t {
    LABEL = 0,
    VALUE,
    MINMAX,
    LABELS_STAT_COLUMN_SIZE
};

template <typename T>
void operator ++(T& value, int)
{ 
    value = static_cast<T>(value + 1);
}

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
