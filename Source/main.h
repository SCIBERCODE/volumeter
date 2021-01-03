#pragma once
#include <JuceHeader.h>
#include <memory>
#include "gui_theme.h"
#include "settings.h"

extern unique_ptr<theme::light_> __theme;

const map<int, String> _prefs = {
    { -15, L"f" },
    { -12, L"p" },
    {  -9, L"n" },
    {  -6, L"μ" },
    {  -3, L"m" },
    {   0, L""  },
    {   3, L"k" },
    {   6, L"M" }
};

enum volume_t : size_t {
    LEFT = 0,
    RIGHT,
    BALANCE,
    VOLUME_SIZE
};

const map<volume_t, String> _stat_captions = {
    { LEFT,    L"Left"    },
    { RIGHT,   L"Right"   },
    { BALANCE, L"Balance" }
};

enum extremum_t : size_t {
    MIN = 0,
    MAX,
    EXTREMES_SIZE
};

enum filter_type_t : size_t {
    HIGH_PASS = 0,
    LOW_PASS,
    FILTER_TYPE_SIZE
};

enum labels_stat_column_t : size_t {
    LABEL = 0,
    VALUE,
    EXTREMES,
    LABELS_STAT_COLUMN_SIZE
};

template <typename T>
void operator ++(T& value, int)
{
    value = static_cast<T>(value + 1);
}

String prefix(double value, const wchar_t *unit, size_t numder_of_decimals);
String prefix_v(double value);

bool is_about_equal(float a, float b);
bool round_flt(float value);

//=========================================================================================
class application_ : public JUCEApplication
{
public:
    application_();
    ~application_() override;

    const String getApplicationName() override;
    const String getApplicationVersion() override;
    void  shutdown() override;
    void  initialise(const String&) override;
    bool  moreThanOneInstanceAllowed() override;

protected:

    class main_window_ : public DocumentWindow
    {
    public:
        main_window_ (const String& name, Component* component, JUCEApplication& a) :
            DocumentWindow(name, __theme->get_bg_color(), DocumentWindow::allButtons),
            app(a)
        {
            setUsingNativeTitleBar(false);
            setTitleBarTextCentred(true);
            setResizable(true, false);
            setContentOwned(component, true);
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

};
