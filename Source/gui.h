
// T[00]

class main_component : public AudioAppComponent,
                       public Timer {
//=================================================================================================
private:
    const float _buff_size_list_sec[8] { 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 30.0f };
    const int   _tone_list[9]          { 10, 20, 200, 500, 1000, 2000, 5000, 10000, 20000 };

    using controls_t = std::variant<std::shared_ptr<Label>, std::shared_ptr<ToggleButton>>;

    template <typename T>
    std::shared_ptr<T> _get(controls_t var)
    {
        return *std::get_if<std::shared_ptr<T>>(&var);
    }

    signal                    _signal;
    application             & _app;
    component_calibration     _component_calibration;
    component_filter          _component_filter;
    component_graph           _component_graph;
    theme::checkbox_left_tick _theme_left_tick; // должно быть объявлено раньше stat_controls

    Label                      label_device_type { { }, L"Type"        },
                               label_device      { { }, L"Device"      },
                               label_sample_rate { { }, L"Sample rate" },
                               label_buff_size   { { }, L"Buff size"   },
                               label_graph_type  { { }, L"Graph for"   };
    ToggleButton               checkbox_tone     {      L"Tone"        };
    TooltipWindow              hint              { this, 500           };
    TextButton                 button_stat_reset, button_pause_graph, button_zero;
    ComboBox                   combo_dev_types, combo_dev_outputs, combo_dev_rates, combo_buff_size, combo_tone;
    controls_t                 stat_controls        [VOLUME_SIZE    ][LABELS_STAT_COLUMN_SIZE];
    ToggleButton               checkboxes_graph_type[GRAPH_TYPE_SIZE];
//=================================================================================================
public:
    main_component(application& app) :
        _component_calibration(app, _signal),
        _component_filter     (app, _signal),
        _component_graph      (app, _signal),
        _signal               (app         ),
        _app                  (app         )
    {
        load_devices();

        // middle settings
        addAndMakeVisible(combo_buff_size);
        addAndMakeVisible(label_buff_size);
        addAndMakeVisible(checkbox_tone);
        addAndMakeVisible(combo_tone);

        label_buff_size.attachToComponent(&combo_buff_size, true);
        for (const auto item : _buff_size_list_sec)
            combo_buff_size.addItem(prefix(item, L"s", 0), static_cast<int>(item * 1000));

        combo_buff_size.setSelectedId(_app.get_int(option_t::buff_size));
        combo_buff_size.onChange = [this]
        {
            auto value = combo_buff_size.getSelectedId();
            _signal.change_buff_size(value);
            _app.save(option_t::buff_size, value);
            _component_graph.start_waiting(waiting_event_t::buffer_fill, value);
        };

        checkbox_tone.setToggleState(_app.get_int(option_t::tone), dontSendNotification);
        checkbox_tone.setLookAndFeel(&_theme_left_tick);
        checkbox_tone.onClick = [this]
        {
            _signal.extremes_clear();
            _app.save(option_t::tone, checkbox_tone.getToggleState());
        };
        for (const auto item : _tone_list)
            combo_tone.addItem(prefix(item, L"Hz", 0), item);

        combo_tone.setSelectedId(_app.get_int(option_t::tone_value));
        combo_tone.onChange = [this]
        {
            auto value = combo_tone.getSelectedId();
            _signal.set_freq(value);
            _app.save(option_t::tone_value, value);
        };

        /** =======================================================================================
            statistics
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
                        const auto channel_name = __channels_name.at(line).toLowerCase();
                        const auto option_name = line == LEFT ? option_t::graph_left : option_t::graph_right;

                        checkbox->setButtonText(__channels_name.at(line));
                        checkbox->setLookAndFeel(&_theme_left_tick);
                        checkbox->setTooltip(L"Show the " + channel_name + L" channel on the graph");

                        checkbox->setToggleState (_app.get_int(option_name), dontSendNotification);
                        checkbox->onClick = [=]
                        {
                            if (auto control = _get<ToggleButton>(stat_controls[line][column]))
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
                else // создание надписей с данными и экстремумами
                { 
                    // T[01]
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

        /** =======================================================================================
            above the graph
        */
        addAndMakeVisible(label_graph_type);
        label_graph_type.setJustificationType(Justification::right);
        for (size_t type = INPUT; type < GRAPH_TYPE_SIZE; type++)
        {
            addAndMakeVisible(checkboxes_graph_type[type]);
            checkboxes_graph_type[type].setRadioGroupId(1);
            checkboxes_graph_type[type].onClick = [this, type] {
                _app.save(option_t::graph_type, static_cast<int>(type));
            };
        }

        checkboxes_graph_type[INPUT ].setButtonText(L"Input Buffer");
        checkboxes_graph_type[OUTPUT].setButtonText(L"Output Buffer");

        auto type_loaded = static_cast<graph_type_t>(_app.get_int(option_t::graph_type));
        checkboxes_graph_type[type_loaded].setToggleState(true, dontSendNotification);

        /** =======================================================================================
            bellow the graph
        */
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
            _component_graph.reset();
        };

        button_pause_graph.setToggleState(_app.get_int(option_t::graph_paused), dontSendNotification);
        button_pause_graph.onClick = [this]
        {
            auto value = button_pause_graph.getToggleState();
            button_pause_graph.setButtonText(value ? L"Resume graph" : L"Pause graph");
            _app.save(option_t::graph_paused, value);
        };

        button_zero.onClick = [this]
        {
            auto value = button_zero.getToggleState();
            _app.save(option_t::zero, value);
            if (value)
                _signal.zeros_set();
            else
                _signal.zeros_clear();
        };
        if (_app.get_int(option_t::zero)) {
            button_zero.setToggleState(true, dontSendNotification);
            _signal.zeros_set(
                _app.get_text(option_t::zero_value_left ).getDoubleValue(),
                _app.get_text(option_t::zero_value_right).getDoubleValue()
            );
        }

        addAndMakeVisible(_component_graph);
        addAndMakeVisible(_component_filter);
        addAndMakeVisible(_component_calibration);
        _component_calibration.update();

        setSize(430, 825);
        startTimer(100);

        setAudioChannels(2, 2);
    }

    ~main_component() override
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
        if (types.size() > 1) // B[11]
        {
            auto index = 0;
            for (auto i = 0; i < types.size(); ++i)
            {
                auto name_type = types.getUnchecked(i)->getTypeName();
                combo_dev_types.addItem(name_type, i + 1);

                if (name_type == _app.get_text(option_t::device_type))
                    index = i;
            }

            combo_dev_types.setSelectedItemIndex(index);
            combo_dev_types.onChange = [this]
            {
                _app.save(option_t::device_type, combo_dev_types.getText());
                auto type = deviceManager.getAvailableDeviceTypes()[combo_dev_types.getSelectedId() - 1];
                if (type)
                {
                    deviceManager.setCurrentAudioDeviceType(type->getTypeName(), true);
                    const StringArray devs(type->getDeviceNames());
                    combo_dev_outputs.clear(dontSendNotification);

                    auto device = _app.get_text(option_t::device_name);

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

            _app.save(option_t::device_name, combo_dev_outputs.getText());
            auto sample_rate = _app.get_int(option_t::sample_rate);

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
            _app.save(option_t::sample_rate, combo_dev_rates.getSelectedId());
            combo_dev_outputs.onChange();
        };
    }

    //=============================================================================================
    // juce callbacks

    void releaseResources() override {
        //=========================================================================================
    }

    void prepareToPlay(const int /*samples_per_block*/, const double sample_rate) override {
        //=========================================================================================
        combo_buff_size.onChange();
        _signal.prepare_to_play(sample_rate);
        _component_filter.prepare_to_play(sample_rate);
        _component_graph.start_waiting(waiting_event_t::buffer_fill, _app.get_int(option_t::buff_size));
    }

    void getNextAudioBlock(const AudioSourceChannelInfo& buffer) override {
        //=========================================================================================
        _signal.next_audio_block(buffer);
    }

    void resized() override { // T[02]
        //=========================================================================================
        auto area = getLocalBounds().reduced(theme::margin * 2);
        auto combo_with_label = [&](ComboBox& combo)
        {
            auto line = area.removeFromTop(theme::height);
            line.removeFromLeft(theme::label_width);
            combo.setBounds(line);
            area.removeFromTop(theme::margin);
        };
        // top to bottom
        {
            // audio devices
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
        Rectangle<int> remain;
        {
            area.removeFromBottom(theme::margin);
            _component_filter.setBounds(area.removeFromBottom(95));
            area.removeFromBottom(theme::margin);
            _component_calibration.setBounds(area.removeFromBottom(250));

            // bellow the graph, bottom to top
            area.removeFromBottom(theme::margin * 2);
            auto line = area.removeFromBottom(theme::height);
            button_zero.setBounds(line.removeFromRight(theme::button_width));
            line.removeFromRight(theme::margin);
            button_pause_graph.setBounds(line.removeFromRight(theme::button_width));
            line.removeFromRight(theme::margin);
            button_stat_reset.setBounds(line.removeFromRight(theme::button_width));

            // above the graph, top to bottom
            remain = area;
            auto left  = area.removeFromLeft(theme::label_width);
            auto right = area.removeFromRight(getWidth() / 2);
            right.removeFromRight(theme::margin * 2);

            for (size_t line_index = LEFT; line_index < VOLUME_SIZE; line_index++)
            {
                if (line_index == LEFT || line_index == RIGHT) {
                    if (auto checkbox = _get<ToggleButton>(stat_controls[line_index][LABEL]))
                        checkbox->setBounds(left.removeFromTop(theme::height).withTrimmedRight(theme::margin));
                }
                else {
                    if (auto label_balance = _get<Label>(stat_controls[line_index][LABEL]))
                        label_balance->setBounds(left.removeFromTop(theme::height).withTrimmedRight(theme::margin - 3)); // T[03]
                }

                if (auto label = _get<Label>(stat_controls[line_index][VALUE]))
                    label->setBounds(area.removeFromTop(theme::height));

                if (auto label = _get<Label>(stat_controls[line_index][EXTREMES]))
                    label->setBounds(right.removeFromTop(theme::height));

                remain.removeFromTop(theme::height);

                auto top_margin = theme::margin;
                if (line_index == BALANCE) top_margin *= 3; // for the graph_type
                area.removeFromTop  (top_margin);
                right.removeFromTop (top_margin);
                left.removeFromTop  (top_margin);
                remain.removeFromTop(top_margin);
            }            
            label_graph_type.setBounds(left.removeFromTop(theme::height).withTrimmedRight(theme::margin));
            checkboxes_graph_type[INPUT ].setBounds(area.removeFromTop(theme::height));
            checkboxes_graph_type[OUTPUT].setBounds(right.removeFromTop(theme::height));
            remain.removeFromTop(theme::height);
        }
        remain.removeFromTop(theme::margin);
        remain.removeFromBottom(theme::margin);
        _component_graph.setBounds(remain);
    }

    void timerCallback() override {
        //=========================================================================================
        std::vector<double> rms[GRAPH_TYPE_SIZE] = { _signal.get_rms(INPUT), _signal.get_rms(OUTPUT) };
        if (rms[INPUT].size() == 0 || rms[OUTPUT].size() == 0) return;

        if (_component_graph.is_waiting())
            _component_graph.stop_waiting();

        for (auto type = INPUT; type < GRAPH_TYPE_SIZE; type ++)
            for (auto channel = LEFT; channel < CHANNEL_SIZE; channel++)
            {
                _component_graph.enqueue(type, channel, rms[type].at(channel));
                if (type == INPUT)
                    rms[type].at(channel) = _app.do_corrections(channel, rms[type].at(channel));
            }

        rms[INPUT].push_back(abs(rms[INPUT].at(LEFT) - rms[INPUT].at(RIGHT))); // баланс вычисляется после всех корректировок
        _signal.extremes_set(rms[INPUT]);

        String printed[3];
        for (size_t line = LEFT; line < VOLUME_SIZE; line++)
        {
            std::fill_n(printed, _countof(printed), theme::empty);
            auto extremes = _signal.extremes_get(line);

            if (isfinite(rms[INPUT].at(line))) printed[0] = _app.print(rms[INPUT].at(line));
            if (isfinite(extremes[MIN]))       printed[1] = _app.print(extremes[MIN]);
            if (isfinite(extremes[MAX]))       printed[2] = _app.print(extremes[MAX]);

            if (auto label = _get<Label>(stat_controls[line][VALUE])) {
                label->setText(printed[0], dontSendNotification);
                label->setColour(Label::textColourId, isfinite(rms[INPUT].at(line)) ? Colours::black : Colours::grey);
            }
            if (auto label = _get<Label>(stat_controls[line][EXTREMES]))
                label->setText(printed[1] + L" .. " + printed[2], dontSendNotification);
        }
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (main_component)
};

