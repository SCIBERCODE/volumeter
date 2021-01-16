#include <JuceHeader.h>
#include <memory>
#include <variant>
#include <spuce/filters/iir.h>
#include <spuce/filters/butterworth_iir.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "gui_theme.h"
#include "main.h"
#include "circular_buffer.h"
#include "signal.h"
#include "gui_filter.h"
#include "gui_calibration.h"
#include "gui_graph.h"
#include "gui.h"

application::application() {
    _theme = std::make_unique<theme::light>();

    PropertiesFile::Options params;
    params.applicationName = L"volumeter";
    params.filenameSuffix  = L".xml";
    params.folderName      = File::getCurrentWorkingDirectory().getFullPathName();
    params.storageFormat   = PropertiesFile::storeAsXML;
    _settings.setStorageParameters(params);
}

application::~application() {
    _theme.reset();
    _settings.closeFiles();
}

theme::light *application::get_theme() const {
    return _theme.get();
}

void application::initialise(const String&)
{
    Desktop::getInstance().setDefaultLookAndFeel(_theme.get());
    _main_window = std::make_unique<main_window>(L"rms volumeter", *this);
}

const String application::getApplicationName()       { return L"rms_volumeter";    }
const String application::getApplicationVersion()    { return __DATE__;            }
bool  application::moreThanOneInstanceAllowed()      { return true;                }
void  application::shutdown()                        { _main_window = nullptr;     }
void  application::main_window::closeButtonPressed() { _app.systemRequestedQuit(); }
      application::main_window::~main_window()       { }

//=================================================================================================
application::main_window::main_window(const String& name, application& app) :
    DocumentWindow(name, app.get_theme()->get_bg_color(), DocumentWindow::allButtons),
    _app(app)
{
    _content = std::make_unique<main_component>(app);

    setUsingNativeTitleBar(false);
    setTitleBarTextCentred(true);
    setResizable(true, false);
    setContentOwned(_content.get(), true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
    toFront(true);
}

//=================================================================================================
const String prefix(const double value, const wchar_t *unit, const size_t numder_of_decimals)
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

const bool is_about_equal(const float a, const float b) {
    return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * FLT_EPSILON);
}

const bool round_flt(const float value) {
    return round(value * 10000.0f) / 10000.0f;
}

START_JUCE_APPLICATION(application)

