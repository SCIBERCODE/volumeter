#include <JuceHeader.h>

using namespace std;

#include "main.h"
#include "gui_theme.h"
#include "gui.h"

unique_ptr<settings_>     __opt;
unique_ptr<theme::light_> __theme;

application_::application_() {
    __opt   = make_unique<settings_>();
    __theme = make_unique<theme::light_>();
}

application_::~application_() {
    __opt  .reset();
    __theme.reset();
}

application_::main_window_::main_window_(const String& name, JUCEApplication& app) :
    DocumentWindow(name, __theme->get_bg_color(), DocumentWindow::allButtons),
    _app(app)
{
    _content = make_unique<main_component_>();

    setUsingNativeTitleBar(false);
    setTitleBarTextCentred(true);
    setResizable(true, false);
    setContentOwned(_content.get(), true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
    toFront(true);
}

void application_::initialise(const String&)
{
    Desktop::getInstance().setDefaultLookAndFeel(__theme.get());
    _main_window = make_unique<main_window_>(L"rms volumeter", *this);
}

const String application_::getApplicationName()        { return L"rms_volumeter";    }
const String application_::getApplicationVersion()     { return __DATE__;            }
bool  application_::moreThanOneInstanceAllowed()       { return true;                }
void  application_::shutdown()                         { _main_window = nullptr;     }
void  application_::main_window_::closeButtonPressed() { _app.systemRequestedQuit(); }

String prefix(const double value, const wchar_t *unit, const size_t numder_of_decimals)
{
    auto   symbol    = String();
    double new_value = value;

    auto exp = static_cast<int>(floor(log10(value) / 3.0) * 3.0);
    if (__prefs.count(exp))
    {
        symbol = __prefs.at(exp);
        new_value = value * pow(10.0, -exp);
        if (new_value >= 1000.0)
        {
            new_value /= 1000.0;
            exp += 3;
        }
    }
    return String(new_value, static_cast<int>(numder_of_decimals)) + L" " + symbol + String(unit);
}

String prefix_v(const double value) {
    return prefix(value, L"V", 5);
}

bool is_about_equal(const float a, const float b) {
    return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * FLT_EPSILON);
}

bool round_flt(const float value) {
    return round(value * 10000.0f) / 10000.0f;
}

START_JUCE_APPLICATION(application_)
