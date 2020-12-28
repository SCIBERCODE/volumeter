#pragma once
#include <JuceHeader.h>
#include "gui_cals.h"

extern unique_ptr<settings_> _opt;

class main_component_ : public AudioAppComponent,
                        public Timer
{
protected:

    const vector<float> buff_size_list = { 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 10.0f, 30.0f };
    const vector<int>   tone_list      = { 10, 20, 200, 500, 1000, 5000, 10000, 20000 };
    const vector<int>   order_list     = { 1, 2, 4, 10, 20, 40, 60, 80, 100, 120, 140, 200 };

//=========================================================================================
public:
//=========================================================================================

    main_component_() : calibrations_component(signal)
    {
        load_devices();

        // === filter =====================================================================
        addAndMakeVisible(checkbox_high_pass);
        addAndMakeVisible(checkbox_low_pass);
        addAndMakeVisible(slider_freq_high);
        addAndMakeVisible(combo_order);

        checkbox_high_pass.setToggleState(_opt->load_int("checkbox_high_pass"), dontSendNotification);
        checkbox_low_pass. setToggleState(_opt->load_int("checkbox_low_pass" ), dontSendNotification);
        checkbox_high_pass.setLookAndFeel(&theme_right);

        checkbox_high_pass.onClick = [this] {
            _opt->save("checkbox_high_pass", checkbox_high_pass.getToggleState());
            signal.filter_init();
        };
        checkbox_low_pass.onClick = [this] {
            _opt->save("checkbox_low_pass", checkbox_low_pass.getToggleState());
            signal.filter_init();
        };

        slider_freq_high.setSliderStyle(Slider::LinearHorizontal);
        slider_freq_high.setTextBoxStyle(Slider::TextBoxRight, false, 60, 20);
        slider_freq_high.setRange(20, 20000, 1);
        slider_freq_high.setValue(_opt->load_int("slider_freq_high"), dontSendNotification);
        slider_freq_high.onDragEnd = [this] {
            _opt->save("slider_freq_high", slider_freq_high.getValue());
            signal.filter_init();
        };

        for (auto const item : order_list)
            combo_order.addItem(String(item), item);

        combo_order.setSelectedId(_opt->load_int("combo_order"), dontSendNotification);
        combo_order.setTooltip("Filter Order");
        combo_order.onChange = [this]
        {
            auto value = combo_order.getSelectedId();
            _opt->save("combo_order", value);
            signal.set_order(value);
            signal.filter_init();
        };

        // === middle settings ============================================================
        addAndMakeVisible(combo_buff_size);
        addAndMakeVisible(label_buff_size);
        addAndMakeVisible(checkbox_tone);
        addAndMakeVisible(combo_tone);

        label_buff_size.attachToComponent(&combo_buff_size, true);
        for (auto const item : buff_size_list)
            combo_buff_size.addItem(prefix(item, "s", 0), static_cast<int>(item * 1000));

        combo_buff_size.setSelectedId(_opt->load_int("combo_buff_size"));
        combo_buff_size.onChange = [this]
        {
            auto value = combo_buff_size.getSelectedId();
            signal.change_buff_size(value);
            _opt->save("combo_buff_size", value);
        };

        checkbox_tone.setToggleState(_opt->load_int("checkbox_tone"), dontSendNotification);
        checkbox_tone.setLookAndFeel(&theme_right_text);
        checkbox_tone.onClick = [this]
        {
            signal.minmax_clear();
            _opt->save("checkbox_tone", checkbox_tone.getToggleState());
        };
        for (auto const item : tone_list)
            combo_tone.addItem(prefix(item, "Hz", 0), item);

        combo_tone.setSelectedId(_opt->load_int("combo_tone"));
        combo_tone.onChange = [this]
        {
            auto value = combo_tone.getSelectedId();
            signal.set_freq(value);
            _opt->save("combo_tone", value);
        };

        // === stat =======================================================================
        const array<String, 3> stat_captions = { "Left", "Right", "Balance" }; // todo: хорошо бы по точке центровать цифры
        for (auto line = LEFT; line < LEVEL_SIZE; line++) {
            for (auto column = LABEL; column < LABELS_STAT_COLUMN_SIZE; column++)
            {
                auto label = make_unique<Label>(String(), column == LABEL ? stat_captions.at(line) : String());
                addAndMakeVisible(label.get());
                labels_stat.at(line).at(column) = move(label);
                
            }
            auto label_value = labels_stat.at(line).at(VALUE).get();
            labels_stat.at(line).at(LABEL) ->attachToComponent(label_value, true);
            labels_stat.at(line).at(VALUE) ->setFont(label_value->getFont().boldened());
            labels_stat.at(line).at(MINMAX)->setJustificationType(Justification::right);
        }

        addAndMakeVisible(button_zero);
        addAndMakeVisible(button_zero_reset);
        addAndMakeVisible(button_stat_reset);

        button_zero.setButtonText      ("Set zero"  );
        button_zero_reset.setButtonText("Reset zero");
        button_stat_reset.setButtonText("Reset stat");

        button_zero.onClick       = [this] { signal.zero_set();     };
        button_zero_reset.onClick = [this] { signal.zero_clear();   };
        button_stat_reset.onClick = [this] { signal.minmax_clear(); };

        addAndMakeVisible(calibrations_component);
        calibrations_component.update();

        setSize(430, 670);
        startTimer(100);

        setAudioChannels(2, 2);
    }

    ~main_component_() override
    {
        shutdownAudio();
        checkbox_tone.setLookAndFeel(nullptr);
        checkbox_high_pass.setLookAndFeel(nullptr);
    }

    //=====================================================================================
    void load_devices()
    {
        addAndMakeVisible(combo_dev_types);
        addAndMakeVisible(combo_dev_outputs);
        addAndMakeVisible(combo_dev_rates);

        const OwnedArray<AudioIODeviceType>& types = deviceManager.getAvailableDeviceTypes();
        if (types.size() > 1)
        {
            auto index = 0;
            for (auto i = 0; i < types.size(); ++i)
            {
                auto name_type = types.getUnchecked(i)->getTypeName();
                combo_dev_types.addItem(name_type, i + 1);

                if (name_type == _opt->load_text("combo_dev_type"))
                    index = i;
            }

            combo_dev_types.setSelectedItemIndex(index);
            combo_dev_types.onChange = [this]
            {
                _opt->save("combo_dev_type", combo_dev_types.getText());
                auto type = deviceManager.getAvailableDeviceTypes()[combo_dev_types.getSelectedId() - 1];
                if (type)
                {
                    deviceManager.setCurrentAudioDeviceType(type->getTypeName(), true);
                    const StringArray devs(type->getDeviceNames());
                    combo_dev_outputs.clear(dontSendNotification);

                    auto device = _opt->load_text("combo_dev_output");

                    for (int i = 0; i < devs.size(); ++i) {
                        combo_dev_outputs.addItem(devs[i], i + 1);
                        if (device == devs[i])
                            combo_dev_outputs.setSelectedItemIndex(i);
                    }
                    if (device.isEmpty())
                        combo_dev_outputs.setSelectedItemIndex(0);
                }
            };
        }
        combo_dev_outputs.onChange = [this]
        {
            signal.clear_data();
            signal.minmax_clear();

            _opt->save("combo_dev_output", combo_dev_outputs.getText());
            auto sample_rate = _opt->load_int("combo_dev_rate");

            auto config                     = deviceManager.getAudioDeviceSetup();
            config.outputDeviceName         = combo_dev_outputs.getText();
            config.inputDeviceName          = config.outputDeviceName;
            config.useDefaultInputChannels  = true;
            config.useDefaultOutputChannels = true;
            config.sampleRate               = sample_rate;
            deviceManager.setAudioDeviceSetup(config, true);

            if (auto* currentDevice = deviceManager.getCurrentAudioDevice()) {
                combo_dev_rates.clear(dontSendNotification);
                for (auto rate : currentDevice->getAvailableSampleRates())
                {
                    auto intRate = roundToInt(rate);
                    combo_dev_rates.addItem(String(intRate) + " Hz", intRate);
                }
                combo_dev_rates.setSelectedId(sample_rate, dontSendNotification);
            }
        };
        combo_dev_rates.onChange = [this]
        {
            _opt->save("combo_dev_rate", combo_dev_rates.getSelectedId());
            combo_dev_outputs.onChange();
        };
    }

    //=====================================================================================
    void releaseResources() override { }

    void prepareToPlay (int /*samples_per_block*/, double sample_rate) override
    {
        combo_buff_size.onChange();
        signal.prepare_to_play(sample_rate);

        slider_freq_high.setRange(20, sample_rate / 2, 1);
        slider_freq_high.repaint();
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& buffer) override
    {
        signal.next_audio_block(buffer);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(theme::margin * 2);

        // === devices ====================================================================
        combo_dev_types.setBounds(area.removeFromTop(theme::height));
        area.removeFromTop(theme::margin);
        combo_dev_outputs.setBounds(area.removeFromTop(theme::height));
        area.removeFromTop(theme::margin);
        combo_dev_rates.setBounds(area.removeFromTop(theme::height));
        area.removeFromTop(theme::margin);

        // === filter =====================================================================
        auto line = area.removeFromTop(theme::height);
        checkbox_high_pass.setBounds(line.removeFromLeft(theme::label + theme::margin * 3)); // todo: wtf with margins
        slider_freq_high.setBounds(line.removeFromLeft(line.getWidth() - (60 + theme::label) - theme::margin * 6));
        line.removeFromLeft(theme::margin);
        combo_order.setBounds(line.removeFromLeft(60 + theme::margin));
        line.removeFromLeft(theme::margin);
        checkbox_low_pass.setBounds(line);
        area.removeFromTop(theme::margin * 2 + theme::height);

        // === middle settings ============================================================
        line = area.removeFromTop(theme::height);
        line.removeFromLeft(theme::label);
        combo_buff_size.setBounds(line);
        area.removeFromTop(theme::margin);
        line = area.removeFromTop(theme::height);
        checkbox_tone.setBounds(line.removeFromLeft(theme::label - theme::margin));
        line.removeFromLeft(theme::margin);
        combo_tone.setBounds(line);
        area.removeFromTop(theme::margin);

        // from bottom
        area.removeFromBottom(theme::margin);
        calibrations_component.setBounds(area.removeFromBottom(280));

        // === stat =======================================================================
        area.removeFromBottom(theme::height + theme::margin);
        button_stat_reset.setBounds(area.removeFromBottom(theme::height));
        area.removeFromBottom(theme::margin);
        button_zero_reset.setBounds(area.removeFromBottom(theme::height));
        area.removeFromBottom(theme::margin);
        button_zero.setBounds(area.removeFromBottom(theme::height));
        area.removeFromBottom(theme::margin);

        area.removeFromLeft(theme::label);
        auto right = area.removeFromRight(getWidth() / 2);
        right.removeFromRight(theme::margin * 2);

        for (auto line_index = LEFT; line_index < LEVEL_SIZE; line_index++)
        {
            labels_stat.at(line_index).at(labels_stat_column_t::VALUE) ->setBounds(area.removeFromTop (theme::height));
            labels_stat.at(line_index).at(labels_stat_column_t::MINMAX)->setBounds(right.removeFromTop(theme::height));

            if (line_index < level_t::BALANCE) {
                area .removeFromTop(theme::margin);
                right.removeFromTop(theme::margin);
            }
        }
    }

    void timerCallback() override
    {
        auto rms = signal.get_rms();
        if (rms.size() == 0) return;

        function<String(double)> print;

        auto calibrate = calibrations_component.is_active();
        if (calibrate)
        {
            rms.at(LEFT)  *= calibrations_component.get_coeff(LEFT); // bug: при неучтённом префиксе
            rms.at(RIGHT) *= calibrations_component.get_coeff(RIGHT);
            print = prefix_v;
        }
        else {
            rms.at(LEFT)  = signal.gain2db(rms.at(LEFT))  - signal.zero_get(LEFT);
            rms.at(RIGHT) = signal.gain2db(rms.at(RIGHT)) - signal.zero_get(RIGHT);
            print = [](double value) { return String(value, 3) + " dB"; };
        }
        rms.push_back(abs(rms.at(LEFT) - rms.at(RIGHT)));
        signal.minmax_set(rms);

        array<String, 3> printed;
        for (auto line = LEFT; line < LEVEL_SIZE; line++)
        {
            printed.fill("--");
            auto minmax = signal.minmax_get(line);

            if (isfinite(rms.at(line)))   printed.at(0) = print(rms.at(line));
            if (isfinite(minmax.at(MIN))) printed.at(1) = print(minmax.at(MIN));
            if (isfinite(minmax.at(MAX))) printed.at(2) = print(minmax.at(MAX));

            labels_stat.at(line).at(1)->setText(printed.at(0), dontSendNotification);
            labels_stat.at(line).at(2)->setText(printed.at(1) + " .. " + printed.at(2), dontSendNotification);
            labels_stat.at(line).at(1)->setColour(Label::textColourId, isfinite(rms.at(line)) ? Colours::black : Colours::grey);
        }

        if (button_zero.isEnabled()       == calibrate) button_zero.setEnabled      (!calibrate);
        if (button_zero_reset.isEnabled() == calibrate) button_zero_reset.setEnabled(!calibrate);
    }

//=========================================================================================
private:
//=========================================================================================

    TextButton button_zero, button_zero_reset, button_stat_reset;
    ComboBox   combo_dev_types, combo_dev_outputs, combo_dev_rates, combo_buff_size, combo_tone, combo_order;
    Slider     slider_freq_high;

    array <array<unique_ptr<Label>, 3>, 3> labels_stat; // [LEFT>RIGHT>BALANCE][MIN>MAX]

    Label        label_buff_size    { {}, "Buff size" };
    ToggleButton checkbox_tone      { "Tone"          },
                 checkbox_high_pass { "High pass"     },
                 checkbox_low_pass  { "Low pass"      };

    theme::checkbox_right_text_lf_ theme_right_text;
    theme::checkbox_right_lf_      theme_right;

    signal_                 signal;
    calibrations_component_ calibrations_component;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (main_component_)
};
