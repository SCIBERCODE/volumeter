
using controls_t = std::variant<std::shared_ptr<Label>, std::shared_ptr<ToggleButton>>;

template <typename T>
std::shared_ptr<T> __get(controls_t var)
{
    return *std::get_if<std::shared_ptr<T>>(&var);
}

//=================================================================================================
class main_component_ : public AudioAppComponent,
                        public Timer {
//=================================================================================================
private:
    Label                      label_device_type { { }, L"Type"        },
                               label_device      { { }, L"Device"      },
                               label_sample_rate { { }, L"Sample rate" },
                               label_buff_size   { { }, L"Buff size"   };
    ToggleButton               checkbox_tone     {      L"Tone"        };
    TooltipWindow              hint              { this                };

    component_calibration_     component_calibration;
    component_filter_          component_filter;
    component_graph_           component_graph;
    theme::checkbox_left_tick_ theme_left_tick; // должно быть объявлено раньше stat_controls
    TextButton                 button_stat_reset, button_pause_graph, button_zero;
    ComboBox                   combo_dev_types, combo_dev_outputs, combo_dev_rates, combo_buff_size, combo_tone;
    controls_t                 stat_controls[VOLUME_SIZE][LABELS_STAT_COLUMN_SIZE];

    signal_                    _signal;
    application_             & _app;
//=================================================================================================
public:
    main_component_(application_& app) :
        component_calibration(app, _signal),
        component_filter     (app, _signal),
        component_graph      (app, _signal),
        _signal              (app         ),
        _app                 (app         )
    {
        load_devices();

        // middle settings
        addAndMakeVisible(combo_buff_size);
        addAndMakeVisible(label_buff_size);
        addAndMakeVisible(checkbox_tone);
        addAndMakeVisible(combo_tone);

        label_buff_size.attachToComponent(&combo_buff_size, true);
        for (const auto item : __buff_size_list_sec)
            combo_buff_size.addItem(prefix(item, L"s", 0), static_cast<int>(item * 1000));

        combo_buff_size.setSelectedId(_app.get_int(L"combo_buff_size"));
        combo_buff_size.onChange = [this]
        {
            auto value = combo_buff_size.getSelectedId();
            _signal.change_buff_size(value);
            _app.save(L"combo_buff_size", value);
            component_graph.start_waiting(buffer_fill, value);
        };

        checkbox_tone.setToggleState(_app.get_int(L"checkbox_tone"), dontSendNotification);
        checkbox_tone.setLookAndFeel(&theme_left_tick);
        checkbox_tone.onClick = [this]
        {
            _signal.extremes_clear();
            _app.save(L"checkbox_tone", checkbox_tone.getToggleState());
        };
        for (const auto item : __tone_list)
            combo_tone.addItem(prefix(item, L"Hz", 0), item);

        combo_tone.setSelectedId(_app.get_int(L"combo_tone"));
        combo_tone.onChange = [this]
        {
            auto value = combo_tone.getSelectedId();
            _signal.set_freq(value);
            _app.save(L"combo_tone", value);
        };

        /** =======================================================================================
            статистика
        */
        for (size_t line = LEFT; line < VOLUME_SIZE; line++) {
            for (auto column = LABEL; column < LABELS_STAT_COLUMN_SIZE; column++)
                if (column == LABEL)
                {
                    // создание галочек выбора каналов для отображения на графике
                    if (line == LEFT || line == RIGHT)
                    {
                        auto       checkbox     = std::make_shared<ToggleButton>();
                        auto&      props        = checkbox->getProperties();
                        const auto channel_name = __channel_name.at(line).toLowerCase();
                        const auto option_name = L"graph_" + channel_name;

                        checkbox->setButtonText(__channel_name.at(line));
                        checkbox->setLookAndFeel(&theme_left_tick);
                        checkbox->setTooltip(L"Show " + channel_name + L" channel on the graph");

                        checkbox->setToggleState (_app.get_int(option_name), dontSendNotification);
                        checkbox->onClick = [=]
                        {
                            if (auto control = __get<ToggleButton>(stat_controls[line][column]))
                                _app.save(option_name, control->getToggleState());
                        };

                        if (line == RIGHT)
                            props.set(Identifier(L"color"), static_cast<int>(Colours::green.getARGB()));

                        stat_controls[line][column] = checkbox;
                        addAndMakeVisible(*checkbox);
                    }
                    if (line == BALANCE) {
                        auto label = std::make_shared<Label>();
                        label->setText(L"Balance", dontSendNotification);
                        label->setJustificationType(Justification::centredRight);
                        stat_controls[line][column] = label;
                        addAndMakeVisible(*label);
                    }
                }
                else // создание лабелов с данными и экстремумами
                { // todo: сработка соответствующих галочек при нажатии на цифры показометра
                    auto label = std::make_shared<Label>();

                    if (column == VALUE)
                        label->setFont(label->getFont().boldened());
                    if (column == EXTREMES)
                        label->setJustificationType(Justification::right);

                    label->setText(theme::empty, dontSendNotification);
                    stat_controls[line][column] = label;
                    addAndMakeVisible(*label);
                }
        }

        addAndMakeVisible(button_stat_reset);
        addAndMakeVisible(button_pause_graph);
        addAndMakeVisible(button_zero);

        button_stat_reset .setConnectedEdges(ToggleButton::ConnectedOnRight);
        button_pause_graph.setConnectedEdges(ToggleButton::ConnectedOnRight | ToggleButton::ConnectedOnLeft);
        button_zero       .setConnectedEdges(                                 ToggleButton::ConnectedOnLeft);

        button_stat_reset .setButtonText(L"Reset stat" );
        button_pause_graph.setButtonText(L"Pause graph");
        button_zero       .setButtonText(L"Set zero"   );

        button_pause_graph.setClickingTogglesState(true);
        button_zero.setClickingTogglesState(true);

        button_stat_reset.onClick = [this]
        {
            _signal.extremes_clear();
            component_graph.reset();
        };

        button_pause_graph.setToggleState(_app.get_int(L"button_pause_graph"), dontSendNotification);
        button_pause_graph.onClick = [this]
        {
            auto value = button_pause_graph.getToggleState();
            button_pause_graph.setButtonText(value ? L"Resume graph" : L"Pause graph");
            _app.save(L"button_pause_graph", value);
        };

        button_zero.onClick = [this]
        {
            auto value = button_zero.getToggleState();
            _app.save(L"button_zero", value);
            if (value)
                _signal.zero_set();
            else
                _signal.zero_clear();
        };
        if (_app.get_int(L"button_zero")) {
            button_zero.setToggleState(true, dontSendNotification);
            _signal.zero_set(
                _app.get_text(L"zero_value_left" ).getDoubleValue(),
                _app.get_text(L"zero_value_right").getDoubleValue()
            );
        }

        addAndMakeVisible(component_graph);
        addAndMakeVisible(component_filter);
        addAndMakeVisible(component_calibration);
        component_calibration.update();

        setSize(430, 790);
        startTimer(100);

        setAudioChannels(2, 2);
    }

    ~main_component_() override
    {
        shutdownAudio();
        checkbox_tone.setLookAndFeel(nullptr);
    }

    void load_devices() {
        //=========================================================================================
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
        if (types.size() > 1) // bug: при смене типа и возврате назад, из настроек подтягивается, но не отображается выбранным
        {
            auto index = 0;
            for (auto i = 0; i < types.size(); ++i)
            {
                auto name_type = types.getUnchecked(i)->getTypeName();
                combo_dev_types.addItem(name_type, i + 1);

                if (name_type == _app.get_text(L"combo_dev_type"))
                    index = i;
            }

            combo_dev_types.setSelectedItemIndex(index);
            combo_dev_types.onChange = [this]
            {
                _app.save(L"combo_dev_type", combo_dev_types.getText());
                auto type = deviceManager.getAvailableDeviceTypes()[combo_dev_types.getSelectedId() - 1];
                if (type)
                {
                    deviceManager.setCurrentAudioDeviceType(type->getTypeName(), true);
                    const StringArray devs(type->getDeviceNames());
                    combo_dev_outputs.clear(dontSendNotification);

                    auto device = _app.get_text(L"combo_dev_output");

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
            _signal.clear_data();
            _signal.extremes_clear();

            _app.save(L"combo_dev_output", combo_dev_outputs.getText());
            auto sample_rate = _app.get_int(L"combo_dev_rate");

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
            _app.save(L"combo_dev_rate", combo_dev_rates.getSelectedId());
            combo_dev_outputs.onChange();
        };
    }

    //=============================================================================================
    // переопределённые методы

    void releaseResources() override { }

    void prepareToPlay(const int /*samples_per_block*/, const double sample_rate) override
    {
        combo_buff_size.onChange();
        _signal.prepare_to_play(sample_rate);
        component_filter.prepare_to_play(sample_rate);
        component_graph.start_waiting(buffer_fill, _app.get_int(L"buff_size"));
    }

    void getNextAudioBlock(const AudioSourceChannelInfo& buffer) override
    {
        _signal.next_audio_block(buffer);
    }

    void resized() override // todo: широкое окно
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
            component_filter.setBounds(area.removeFromBottom(95));
            area.removeFromBottom(theme::margin);
            component_calibration.setBounds(area.removeFromBottom(250));

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

            for (size_t line_index = LEFT; line_index < VOLUME_SIZE; line_index++)
            {
                if (line_index == LEFT || line_index == RIGHT) {
                    if (auto checkbox = __get<ToggleButton>(stat_controls[line_index][LABEL]))
                        checkbox->setBounds(left.removeFromTop(theme::height).withTrimmedRight(theme::margin));
                }
                else {
                    if (auto label_balance = __get<Label>(stat_controls[line_index][LABEL]))
                        label_balance->setBounds(left.removeFromTop(theme::height).withTrimmedRight(theme::margin - 3)); // todo: расположить в ряд без подгонки
                }

                if (auto label = __get<Label>(stat_controls[line_index][VALUE]))
                    label->setBounds(area.removeFromTop(theme::height));

                if (auto label = __get<Label>(stat_controls[line_index][EXTREMES]))
                    label->setBounds(right.removeFromTop(theme::height));

                remain.removeFromTop(theme::height);

                if (line_index < BALANCE) {
                    area  .removeFromTop(theme::margin);
                    right .removeFromTop(theme::margin);
                    left  .removeFromTop(theme::margin);
                    remain.removeFromTop(theme::margin);
                }
            }
            remain.removeFromTop(theme::margin);
            remain.removeFromBottom(theme::margin);
            component_graph.setBounds(remain);
        }
    }

    void timerCallback() override
    {
        std::function<String(double)> print; // todo: центровать цифры по точке
        auto rms = _signal.get_rms();
        if (rms.size() == 0) return;

        if (component_graph.is_waiting())
            component_graph.stop_waiting();

        component_graph.enqueue(LEFT,  rms.at(LEFT ));
        component_graph.enqueue(RIGHT, rms.at(RIGHT));

        auto calibrate = component_calibration.is_active();
        if (calibrate)
        {
            rms.at(LEFT ) *= component_calibration.get_coeff(LEFT ); // bug: при неучтённом префиксе
            rms.at(RIGHT) *= component_calibration.get_coeff(RIGHT);
            print = prefix_v;
        }
        else {
            rms.at(LEFT ) = _signal.gain2db(rms.at(LEFT )) - _signal.zero_get(LEFT );
            rms.at(RIGHT) = _signal.gain2db(rms.at(RIGHT)) - _signal.zero_get(RIGHT);
            print = [ ](double value) { return String(value, 3) + L" dB"; };
        }
        rms.push_back(abs(rms.at(LEFT) - rms.at(RIGHT)));
        _signal.extremes_set(rms);

        String printed[3];
        for (size_t line = LEFT; line < VOLUME_SIZE; line++)
        {
            std::fill_n(printed, _countof(printed), theme::empty);
            auto extremes = _signal.extremes_get(line);

            if (isfinite(rms.at(line)))  printed[0] = print(rms.at(line));
            if (isfinite(extremes[MIN])) printed[1] = print(extremes[MIN]);
            if (isfinite(extremes[MAX])) printed[2] = print(extremes[MAX]);

            if (auto label = __get<Label>(stat_controls[line][VALUE])) {
                label->setText(printed[0], dontSendNotification);
                label->setColour(Label::textColourId, isfinite(rms.at(line)) ? Colours::black : Colours::grey);
            }
            if (auto label = __get<Label>(stat_controls[line][EXTREMES]))
                label->setText(printed[1] + L" .. " + printed[2], dontSendNotification);
        }

        if (button_zero.isEnabled() == calibrate) {
            // todo: ноль на вольтах
            button_zero.setToggleState(false, sendNotificationSync);
            button_zero.setEnabled(!calibrate);
        }
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (main_component_)
};

