#include <JuceHeader.h>

using namespace std;

#include "main.h"
#include "gui.h"

unique_ptr<settings_>     _opt;
unique_ptr<theme::light_> _theme;

application_::application_() {
    _opt   = make_unique<settings_>();
    _theme = make_unique<theme::light_>();
}

application_::~application_() {
    _opt  .reset();
    _theme.reset();
}

const String application_::getApplicationName()    { return L"rms_volumeter"; }
const String application_::getApplicationVersion() { return __DATE__;         }
void application_::shutdown()                      { main_window = nullptr;   }
bool application_::moreThanOneInstanceAllowed()    { return true;             }

void application_::initialise(const String&)
{
    Desktop::getInstance().setDefaultLookAndFeel(_theme.get());
    main_window = make_unique<main_window_>(L"rms volumeter", new main_component_(), *this);
}

String prefix(double value, String unit, size_t numder_of_decimals)
{
    auto   symbol    = String();
    double new_value = value;

    auto exp = static_cast<int>(floor(log10(value) / 3.0) * 3.0);
    if (_prefs.count(exp))
    {
        symbol = _prefs.at(exp);
        new_value = value * pow(10.0, -exp);
        if (new_value >= 1000.0)
        {
            new_value /= 1000.0;
            exp += 3;
        }
    }
    return String(new_value, static_cast<int>(numder_of_decimals)) + L" " + symbol + unit;
}

String prefix_v(double value) {
    return prefix(value, L"V", 5);
}

bool is_about_equal(float a, float b) {
    return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * FLT_EPSILON);
}

bool round_flt(float value) {
    return round(value * 10000.0f) / 10000.0f;
}

START_JUCE_APPLICATION(application_)
