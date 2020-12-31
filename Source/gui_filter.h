#pragma once
#include <JuceHeader.h>
#include "signal.h"

extern unique_ptr<settings_> _opt;

class filter_component_ : public Component
{
protected:
    const vector<int> order_list = { 1, 2, 4, 10, 20, 40, 60, 80, 100, 120, 140, 200, 300, 500 };

public:

    filter_component_(signal_& signal) : signal(signal)
    // todo: filter_component_(shared_ptr<signal_> signal) : signal(move(signal))
    {
        addAndMakeVisible(checkbox_filter);
        checkbox_filter.setToggleState(_opt->get_int(L"checkbox_filter"), dontSendNotification);
        checkbox_filter.onClick = [this]
        {
            _opt->save(L"checkbox_filter", checkbox_filter.getToggleState());
        };

        addAndMakeVisible(checkbox_high_pass);
        addAndMakeVisible(checkbox_low_pass);
        addAndMakeVisible(slider_freq_high);
        addAndMakeVisible(combo_order);

        checkbox_high_pass.setToggleState(_opt->get_int(L"checkbox_high_pass"), dontSendNotification);
        checkbox_low_pass .setToggleState(_opt->get_int(L"checkbox_low_pass"),  dontSendNotification);
        checkbox_high_pass.setLookAndFeel(&theme_right);

        checkbox_high_pass.onClick = [&] {
            _opt->save(L"checkbox_high_pass", checkbox_high_pass.getToggleState());
            signal.filter_init();
        };
        checkbox_low_pass.onClick = [&] {
            _opt->save(L"checkbox_low_pass", checkbox_low_pass.getToggleState());
            signal.filter_init();
        };

        slider_freq_high.setSliderStyle(Slider::LinearHorizontal);
        slider_freq_high.setTextBoxStyle(Slider::TextBoxRight, false, 60, 20);
        slider_freq_high.setRange(20, 20000, 1);
        slider_freq_high.setValue(_opt->get_int(L"slider_freq_high"), dontSendNotification);
        slider_freq_high.onDragEnd = [&] {
            _opt->save(L"slider_freq_high", slider_freq_high.getValue());
            signal.filter_init();
        };

        for (auto const item : order_list)
            combo_order.addItem(String(item), item);

        combo_order.setSelectedId(_opt->get_int(L"combo_order"), dontSendNotification);
        combo_order.setTooltip(L"Filter Order");
        combo_order.onChange = [&]
        {
            auto value = combo_order.getSelectedId();
            _opt->save(L"combo_order", value);
            signal.set_order(value);
            signal.filter_init();
        };
    }

    ~filter_component_() {
        checkbox_high_pass.setLookAndFeel(nullptr);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        checkbox_high_pass.setBounds(area.removeFromLeft(theme::label_width + theme::margin * 3)); // todo: wtf with margins
        slider_freq_high.setBounds(area.removeFromLeft(area.getWidth() - (60 + theme::label_width) - theme::margin * 6));
        area.removeFromLeft(theme::margin);
        combo_order.setBounds(area.removeFromLeft(60 + theme::margin));
        area.removeFromLeft(theme::margin);
        checkbox_low_pass.setBounds(area);
        area.removeFromTop(theme::margin * 2 + theme::height);
    }

    void prepare_to_play(double sample_rate)
    {
        slider_freq_high.setRange(20, sample_rate / 2, 1);
        slider_freq_high.repaint();
    }

private:
    signal_                 & signal;
    theme::checkbox_right_lf_ theme_right;
    ComboBox                  combo_order;
    Slider                    slider_freq_high;
    ToggleButton              checkbox_high_pass { L"High pass"  },
                              checkbox_low_pass  { L"Low pass"   },
                              checkbox_filter    { L"Use filter" };

};
