#include <JuceHeader.h>

using namespace std;

#include "main.h"
#include "gui.h"

unique_ptr<settings_> _opt;

application_::application_() {
    _opt = make_unique<settings_>();
}

application_::~application_() {
    _opt.reset();
}

const String application_::getApplicationName()    { return "rms_volumeter"; }
const String application_::getApplicationVersion() { return __DATE__;        }
void application_::shutdown()                      { main_window = nullptr;  }
bool application_::moreThanOneInstanceAllowed()    { return true;            }

void application_::initialise(const String&)
{
    Desktop::getInstance().setDefaultLookAndFeel(&theme);
    main_window = make_unique<main_window_>("rms volumeter", new main_component_(), *this);
}

String prefix(double value, String unit, size_t numder_of_decimals)
{
    auto   symbol    = String();
    double new_value = value;

    auto exp = (int)(floor(log10(value) / 3.0) * 3.0);
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
    return String(new_value, (int)numder_of_decimals) + " " + symbol + unit;
}

String prefix_v(double value) {
    return prefix(value, "V", 5);
}

START_JUCE_APPLICATION(application_)
