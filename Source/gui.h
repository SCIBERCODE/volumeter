#pragma once
#include <JuceHeader.h>
#include "gui_filter.h"
#include "gui_calibration.h"

extern unique_ptr<settings_> _opt;

class main_component_ : public AudioAppComponent,
                        public Timer
{
protected:

    const vector<float> buff_size_list = { 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 10.0f, 30.0f };
    const vector<int>   tone_list      = { 10, 20, 200, 500, 1000, 5000, 10000, 20000 };

//=========================================================================================
public:
//=========================================================================================
    main_component_() :
        calibrations_component(signal),
        filter_component      (signal)
    {
        clear_graph();
        load_devices();

        // === middle settings ============================================================
        addAndMakeVisible(combo_buff_size);
        addAndMakeVisible(label_buff_size);
        addAndMakeVisible(checkbox_tone);
        addAndMakeVisible(combo_tone);

        label_buff_size.attachToComponent(&combo_buff_size, true);
        for (auto const item : buff_size_list)
            combo_buff_size.addItem(prefix(item, L"s", 0), static_cast<int>(item * 1000));

        combo_buff_size.setSelectedId(_opt->get_int(L"combo_buff_size"));
        combo_buff_size.onChange = [this]
        {
            auto value = combo_buff_size.getSelectedId();
            signal.change_buff_size(value);
            _opt->save(L"combo_buff_size", value);
        };

        checkbox_tone.setToggleState(_opt->get_int(L"checkbox_tone"), dontSendNotification);
        checkbox_tone.setLookAndFeel(&theme_right_text);
        checkbox_tone.onClick = [this]
        {
            signal.minmax_clear();
            _opt->save(L"checkbox_tone", checkbox_tone.getToggleState());
        };
        for (auto const item : tone_list)
            combo_tone.addItem(prefix(item, L"Hz", 0), item);

        combo_tone.setSelectedId(_opt->get_int(L"combo_tone"));
        combo_tone.onChange = [this]
        {
            auto value = combo_tone.getSelectedId();
            signal.set_freq(value);
            _opt->save(L"combo_tone", value);
        };

        // === stat ======================================================================= // todo: центровать цифры по точке
        for (auto line = LEFT; line < LEVEL_SIZE; line++) { // todo: заменить таблицей
            for (auto column = LABEL; column < LABELS_STAT_COLUMN_SIZE; column++)
            {
                if (column == LABEL)
                {
                    auto checkbox = make_unique<ToggleButton>();
                    addAndMakeVisible(checkbox.get());
                    checkbox->setButtonText(_stat_captions.at(line));
                    checkbox->setLookAndFeel(&theme_right_text);
                    if (line != BALANCE) checkbox->setTooltip("Show " + _stat_captions.at(line).toLowerCase() + " channel on the graph");
                    switch (line) {
                    case LEFT   : checkbox->setToggleState(_opt->get_int    (L"graph_left"),  dontSendNotification);
                                  checkbox->onClick = [&, line] { _opt->save(L"graph_left", check_stat.at(line)->getToggleState()); }; break;
                    case RIGHT  : checkbox->setToggleState (_opt->get_int   (L"graph_right"), dontSendNotification);
                                  checkbox->onClick = [&, line] { _opt->save(L"graph_right", check_stat.at(line)->getToggleState()); }; break;
                    case BALANCE: checkbox->getProperties().set  (Identifier(L"dont_show_tick"), true); break;
                    }
                    check_stat.at(line) = move(checkbox);
                }
                else {
                    auto label = make_unique<Label>(); // todo: сработка соответствующих галочек при нажатии на цифры показометра
                    addAndMakeVisible(label.get());
                    label_stat.at(line).at(column - 1) = move(label);
                }
            }
            auto label_value = label_stat.at(line).at(VALUE - 1).get();
            label_stat.at(line).at(VALUE  - 1) ->setFont(label_value->getFont().boldened());
            label_stat.at(line).at(MINMAX - 1)->setJustificationType(Justification::right);
        }

        addAndMakeVisible(button_zero);
        addAndMakeVisible(button_stat_reset);
        addAndMakeVisible(button_pause_graph);

        button_stat_reset.setButtonText(L"Reset stat");
        button_zero.setButtonText(L"Set zero");
        button_pause_graph.setButtonText(L"Pause graph");

        button_pause_graph.setClickingTogglesState(true);
        button_zero.setClickingTogglesState(true);

        button_zero.onClick = [this]
        {
            auto value = button_zero.getToggleState();
            _opt->save(L"button_zero", value);
            if (value)
                signal.zero_set();
            else
                signal.zero_clear();
        };
        if (_opt->get_int(L"button_zero")) {
            button_zero.setToggleState(true, dontSendNotification);
            signal.zero_set(
                _opt->get_text(L"zero_value_left" ).getDoubleValue(),
                _opt->get_text(L"zero_value_right").getDoubleValue()
            );
        }

        button_pause_graph.setToggleState(_opt->get_int(L"button_pause_graph"), dontSendNotification);
        button_pause_graph.onClick = [this]
        {
            auto value = button_pause_graph.getToggleState();
            button_pause_graph.setButtonText(value ? L"Resume graph" : L"Pause graph");
            _opt->save(L"button_pause_graph", value);
        };

        button_stat_reset.onClick = [this]
        {
            signal.minmax_clear();
            clear_graph();
        };

        addAndMakeVisible(filter_component);
        addAndMakeVisible(calibrations_component);
        calibrations_component.update();

        setSize(430, 790);
        startTimer(100);

        setAudioChannels(2, 2);
    }

    ~main_component_() override
    {
        shutdownAudio();

        checkbox_tone.setLookAndFeel(nullptr);
        for (auto line = LEFT; line < LEVEL_SIZE; line++)
            check_stat.at(line)->setLookAndFeel(nullptr);
    }

    void clear_graph() {
        auto display_width = 0;
        for (auto display : Desktop::getInstance().getDisplays().displays)
            display_width += display.userArea.getWidth();
        history_stat = make_unique<circle_>(display_width);
    }

    //=====================================================================================
    void load_devices()
    {
        addAndMakeVisible(combo_dev_types);
        addAndMakeVisible(combo_dev_outputs);
        addAndMakeVisible(combo_dev_rates);

        addAndMakeVisible(label_device_type);
        addAndMakeVisible(label_device);
        addAndMakeVisible(label_sample_rate);

        label_device_type.attachToComponent(&combo_dev_types,   true);
        label_device     .attachToComponent(&combo_dev_outputs, true);
        label_sample_rate.attachToComponent(&combo_dev_rates,   true);

        const OwnedArray<AudioIODeviceType>& types = deviceManager.getAvailableDeviceTypes();
        if (types.size() > 1)
        {
            auto index = 0;
            for (auto i = 0; i < types.size(); ++i)
            {
                auto name_type = types.getUnchecked(i)->getTypeName();
                combo_dev_types.addItem(name_type, i + 1);

                if (name_type == _opt->get_text(L"combo_dev_type"))
                    index = i;
            }

            combo_dev_types.setSelectedItemIndex(index);
            combo_dev_types.onChange = [this]
            {
                _opt->save(L"combo_dev_type", combo_dev_types.getText());
                auto type = deviceManager.getAvailableDeviceTypes()[combo_dev_types.getSelectedId() - 1];
                if (type)
                {
                    deviceManager.setCurrentAudioDeviceType(type->getTypeName(), true);
                    const StringArray devs(type->getDeviceNames());
                    combo_dev_outputs.clear(dontSendNotification);

                    auto device = _opt->get_text(L"combo_dev_output");

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

            _opt->save(L"combo_dev_output", combo_dev_outputs.getText());
            auto sample_rate = _opt->get_int(L"combo_dev_rate");

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
                    combo_dev_rates.addItem(String(intRate) + L" Hz", intRate);
                }
                combo_dev_rates.setSelectedId(sample_rate, dontSendNotification);
            }
        };
        combo_dev_rates.onChange = [this]
        {
            _opt->save(L"combo_dev_rate", combo_dev_rates.getSelectedId());
            combo_dev_outputs.onChange();
        };
    }

    //=====================================================================================
    void releaseResources() override { }

    void prepareToPlay(int /*samples_per_block*/, double sample_rate) override
    {
        combo_buff_size.onChange();
        signal.prepare_to_play(sample_rate);
        filter_component.prepare_to_play(sample_rate);
    }

    void getNextAudioBlock(const AudioSourceChannelInfo& buffer) override
    {
        signal.next_audio_block(buffer);
    }

    void resized() override // todo: при достаточной ширине менять вёрстку
    {
        auto area = getLocalBounds().reduced(theme::margin * 2);
        auto combo_with_label = [&](ComboBox& combo)
        {
            auto line = area.removeFromTop(theme::height);
            line.removeFromLeft(theme::label_width);
            combo.setBounds(line);
            area.removeFromTop(theme::margin);
        };
        // последовательное размещение компонентов сверху вниз
        {
            // девайсы
            combo_with_label(combo_dev_types);
            combo_with_label(combo_dev_outputs);
            combo_with_label(combo_dev_rates);

            // middle settings
            auto line = area.removeFromTop(theme::height);
            line.removeFromLeft(theme::label_width);
            combo_buff_size.setBounds(line);
            area.removeFromTop(theme::margin);
            line = area.removeFromTop(theme::height);
            checkbox_tone.setBounds(line.removeFromLeft(theme::label_width - theme::margin));
            line.removeFromLeft(theme::margin);
            combo_tone.setBounds(line);
            area.removeFromTop(theme::margin);
        }
        // снизу вверх
        {
            area.removeFromBottom(theme::margin);
            // фильтр
            filter_component.setBounds(area.removeFromBottom(95));
            area.removeFromBottom(theme::margin);
            calibrations_component.setBounds(area.removeFromBottom(250));

            // stat
            area.removeFromBottom(theme::margin * 2);
            auto line = area.removeFromBottom(theme::height);
            button_zero.setBounds(line.removeFromRight(theme::button_width));
            line.removeFromRight(theme::margin);
            button_pause_graph.setBounds(line.removeFromRight(theme::button_width));
            line.removeFromRight(theme::margin);
            button_stat_reset.setBounds(line.removeFromRight(theme::button_width));

            auto remain = area;
            auto left   = area.removeFromLeft(theme::label_width);
            auto right  = area.removeFromRight(getWidth() / 2);
            right.removeFromRight(theme::margin * 2);

            for (auto line_index = LEFT; line_index < LEVEL_SIZE; line_index++)
            {
                check_stat.at(line_index)->setBounds(left.removeFromTop(theme::height).withTrimmedRight(theme::margin));
                label_stat.at(line_index).at(VALUE  - 1)->setBounds(area.removeFromTop (theme::height));
                label_stat.at(line_index).at(MINMAX - 1)->setBounds(right.removeFromTop(theme::height));
                remain.removeFromTop(theme::height);

                if (line_index < level_t::BALANCE) {
                    area  .removeFromTop(theme::margin);
                    right .removeFromTop(theme::margin);
                    left  .removeFromTop(theme::margin);
                    remain.removeFromTop(theme::margin);
                }
            }
            remain.removeFromTop(theme::margin);
            remain.removeFromBottom(theme::margin);
            history_plot = remain;
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
            print = [](double value) { return String(value, 3) + L" dB"; };
        }
        rms.push_back(abs(rms.at(LEFT) - rms.at(RIGHT)));
        signal.minmax_set(rms);

        if (!button_pause_graph.getToggleState()) {
            if (isfinite(rms.at(LEFT))) { // todo: syncrochannels
                history_stat->enqueue(LEFT,  static_cast<float>(rms.at(LEFT)));
                history_stat->enqueue(RIGHT, static_cast<float>(rms.at(RIGHT)));
                repaint(); // todo: оптимизировать
            }
        }

        array<String, 3> printed;
        for (auto line = LEFT; line < LEVEL_SIZE; line++)
        {
            printed.fill(theme::empty);
            auto minmax = signal.minmax_get(line);

            if (isfinite(rms.at(line)))   printed.at(0) = print(rms.at(line));
            if (isfinite(minmax.at(MIN))) printed.at(1) = print(minmax.at(MIN));
            if (isfinite(minmax.at(MAX))) printed.at(2) = print(minmax.at(MAX));

            label_stat.at(line).at(0)->setText(printed.at(0), dontSendNotification);
            label_stat.at(line).at(1)->setText(printed.at(1) + L" .. " + printed.at(2), dontSendNotification);
            label_stat.at(line).at(0)->setColour(Label::textColourId, isfinite(rms.at(line)) ? Colours::black : Colours::grey);
        }

        if (button_zero.isEnabled() == calibrate) { // todo: логику без этого
            button_zero.setToggleState(false, sendNotificationSync);
            button_zero.setEnabled(!calibrate);
        }
    }

    bool is_about_equal(float a, float b) {
        return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * (FLT_EPSILON / 10000));
    }

    bool round_flt(float value) {
        return round(value * 10000.0f) / 10000.0f;
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colours::white);
        g.fillRect(history_plot);

        const float dotted_pattern[2] = { 2.0f, 3.0f };
        auto y_start = history_plot.getY() + theme::graph_indent;
        auto y_end   = history_plot.getY() + history_plot.getHeight() - theme::graph_indent;

        g.setColour(Colours::grey);
        g.drawDashedLine(
            Line<float>(static_cast<float>(history_plot.getX()), y_start, static_cast<float>(history_plot.getRight()), y_start),
            dotted_pattern, _countof(dotted_pattern)
        );
        g.drawDashedLine(
            Line<float>(static_cast<float>(history_plot.getX()), y_end, static_cast<float>(history_plot.getRight()), y_end),
            dotted_pattern, _countof(dotted_pattern)
        );

        for (auto channel = LEFT; channel <= RIGHT; channel++)
        {
            if (channel == LEFT  && _opt->get_int(L"graph_left" ) == 0) continue;
            if (channel == RIGHT && _opt->get_int(L"graph_right") == 0) continue;

            auto  minmax = history_stat->get_minmax(channel, history_plot.getWidth());
            float offset = 0.0f;
            float value;
            //=====================================================================================
            if (history_stat->get_first_value(channel, history_plot.getWidth(), value)) // bug: устранить мерцание при заполнении буфера
            {
                Path path;
                do {
                    if (!isfinite(value)) break;
                    auto x = history_plot.getRight() - offset++;
                    auto y = -value * (history_plot.getHeight() / abs(minmax.at(MIN) - minmax.at(MAX)));
                    if (path.isEmpty())
                        path.startNewSubPath(x, y);

                    path.lineTo(x, y);
                } while (history_stat->get_next_value(value));
                if (offset < 2) return;

                path.scaleToFit(
                    history_plot.getX() + (history_plot.getWidth() - offset),
                    y_start,
                    offset,
                    history_plot.getHeight() - theme::graph_indent * 2,
                    false
                );
                g.setColour(channel == LEFT ? Colours::black : Colours::green);
                g.strokePath(path, PathStrokeType(1.0f));
            }
            //=====================================================================================
            offset = 0.0f;
            if (history_stat->get_first_value(channel, history_plot.getWidth(), value))
                do {
                    for (auto stat = MIN; stat < STAT_SIZE; stat++)
                    {
                        //if (round_flt(value) == round_flt(minmax.at(MAX)))
                        //if (is_about_equal(value, minmax.at(MAX)))
                        if (String(value, 3) == String(minmax.at(stat), 3))
                        {
                            const auto text = String(value, 3) + L" dB";
                            const auto text_width = g.getCurrentFont().getStringWidth(text);
                            Rectangle<int> rect =
                            {
                                static_cast<int>(history_plot.getRight() - offset - (text_width / 2)),
                                static_cast<int>(stat == MAX ? y_start - 14 : y_end + 1),
                                text_width,
                                13
                            };
                            int overhung[2] = {
                                rect.getX() - history_plot.getX(),
                                history_plot.getRight() - rect.getRight(),
                            };
                            if (overhung[0] < 0) rect.setCentre(rect.getCentreX() - overhung[0], rect.getCentreY());
                            if (overhung[1] < 0) rect.setCentre(rect.getCentreX() + overhung[1], rect.getCentreY());

                            g.setColour(Colours::white);
                            if (stat == MAX) g.fillRect(history_plot.withBottom(y_start - 1));
                            if (stat == MIN) g.fillRect(history_plot.withTop(y_end + 1));

                            g.setColour(channel == LEFT ? Colours::black : Colours::green);
                            g.drawText(text, rect, Justification::centred, false);
                            break;
                        }
                    }
                    offset++;
                } while (history_stat->get_next_value(value));
        }
        /*const size_t line_width = 30;
        Rectangle<int> rect
        (
            history_plot.getX(),
            static_cast<int>(y_end),
            history_plot.getWidth() - theme::margin,
            13
        );
        for (auto channel = LEFT; channel <= RIGHT; channel++)
        {
            const auto text = _stat_captions.at(channel);
            const auto text_width = g.getCurrentFont().getStringWidth(text);
            g.setColour(channel == LEFT ? Colours::black : Colours::green);
            g.drawFittedText(text, rect.removeFromRight(text_width + theme::margin), Justification::centredRight, 1);
            g.fillRect(rect.removeFromRight(line_width).withY(rect.getY() + 1).reduced(0, 6));
            rect.removeFromRight(theme::margin);
        }
        g.setColour(Colours::grey);
        g.drawRect(history_plot);*/
    }

//=========================================================================================
private:
//=========================================================================================

    array<unique_ptr<ToggleButton>, LEVEL_SIZE>    check_stat;  // [LEFT>RIGHT>BALANCE]
    array<array<unique_ptr<Label>, 2>, LEVEL_SIZE> label_stat;  // [LEFT>RIGHT>BALANCE][VALUE>MINMAX]
    array<array<Rectangle<int>, 2>, 2>             rect_minmax;
    unique_ptr<circle_>                            history_stat;
    signal_                                        signal;
    calibrations_component_                        calibrations_component;
    filter_component_                              filter_component;
    Rectangle<int>                                 history_plot;

    Label         label_device_type  { {}, L"Type"        },
                  label_device       { {}, L"Device"      },
                  label_sample_rate  { {}, L"Sample rate" },
                  label_buff_size    { {}, L"Buff size"   };
    ToggleButton  checkbox_tone      {     L"Tone"        };
    TooltipWindow hint               { this               };

    TextButton                     button_zero, button_stat_reset, button_pause_graph;
    ComboBox                       combo_dev_types, combo_dev_outputs, combo_dev_rates, combo_buff_size, combo_tone;
    theme::checkbox_right_text_lf_ theme_right_text;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (main_component_)
};
