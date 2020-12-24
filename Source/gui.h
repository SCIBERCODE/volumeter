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

		addAndMakeVisible(checkbox_bpfH);
		addAndMakeVisible(checkbox_bpfL);
		addAndMakeVisible(slider_freq_high);
		addAndMakeVisible(combo_order);

		checkbox_bpfH.setToggleState(_opt->load_int_("checkbox_bpfH"), dontSendNotification);
		checkbox_bpfH.setLookAndFeel(&theme_right);

		checkbox_bpfH.onClick = [this] {
			_opt->save_("checkbox_bpfH", checkbox_bpfH.getToggleState());
			signal.filter_init();
		};
		checkbox_bpfL.setToggleState(_opt->load_int_("checkbox_bpfL"), dontSendNotification);
		checkbox_bpfL.onClick = [this] {
			_opt->save_("checkbox_bpfL", checkbox_bpfL.getToggleState());
			signal.filter_init();
		};

		slider_freq_high.setSliderStyle(Slider::LinearHorizontal);
		slider_freq_high.setTextBoxStyle(Slider::TextBoxRight, false, 60, 20);
		slider_freq_high.setRange(20, 20000, 1);
		slider_freq_high.setValue(_opt->load_int_("slider_freq_high"), dontSendNotification);
		slider_freq_high.onDragEnd = [this] {
			_opt->save_("slider_freq_high", slider_freq_high.getValue());
			signal.filter_init();
		};

		for (auto const item : order_list)
			combo_order.addItem(String(item), item);

		combo_order.setSelectedId(_opt->load_int_("combo_order"), dontSendNotification);
		combo_order.setTooltip("Filter Order");
		combo_order.onChange = [this]
		{
			auto value = combo_order.getSelectedId();
			_opt->save_("combo_order", value);
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

		combo_buff_size.setSelectedId(_opt->load_int_("combo_buff_size"));
		combo_buff_size.onChange = [this]
		{
			auto value = combo_buff_size.getSelectedId();
			signal.change_buff_size(value);
			_opt->save_("combo_buff_size", value);
		};

		checkbox_tone.setToggleState(_opt->load_int_("checkbox_tone"), dontSendNotification);
		checkbox_tone.setLookAndFeel(&theme_right_text);
		checkbox_tone.onClick = [this]
		{
			signal.minmax_clear();
			_opt->save_("checkbox_tone", checkbox_tone.getToggleState());
		};
		for (auto const item : tone_list)
			combo_tone.addItem(prefix(item, "Hz", 0), item);

		combo_tone.setSelectedId(_opt->load_int_("combo_tone"));
		combo_tone.onChange = [this]
		{
			auto value = combo_tone.getSelectedId();
			signal.set_freq(value);
			_opt->save_("combo_tone", value);
		};

		// === stat =======================================================================

		const array<String, 3> stat_captions = { "Left", "Right", "Balance" };
		for (size_t line = 0; line < 3; line++)	{
			for (size_t col = 0; col < 3; col++)
			{
                labels_stat.at(line).at(col) = make_unique<Label>(String(), col == 0 ? stat_captions.at(line) : String());
				addAndMakeVisible(labels_stat.at(line).at(col).get());
			}
			labels_stat.at(line).at(0)->attachToComponent(labels_stat.at(line).at(1).get(), true);
			labels_stat.at(line).at(1)->setFont(labels_stat.at(line).at(1)->getFont().boldened());
			labels_stat.at(line).at(2)->setJustificationType(Justification::right);
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

		setSize(420, 670);
		startTimer(100);

		setAudioChannels(2, 2);
	}

	~main_component_() override
	{
		shutdownAudio();
		checkbox_tone.setLookAndFeel(nullptr);
		checkbox_bpfH.setLookAndFeel(nullptr);
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
				if (name_type == "ASIO") index = i;
			}

			combo_dev_types.setSelectedItemIndex(index);
			combo_dev_types.onChange = [this]
			{
				AudioIODeviceType *type = deviceManager.getAvailableDeviceTypes()[combo_dev_types.getSelectedId() - 1];
				if (type)
				{
					deviceManager.setCurrentAudioDeviceType(type->getTypeName(), true);
					const StringArray devs(type->getDeviceNames());
					combo_dev_outputs.clear(dontSendNotification);

					auto device = _opt->load_text("device");

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

			_opt->save("device", combo_dev_outputs.getText());
			auto sample_rate = _opt->load_int("sample_rate", "48000");

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
			_opt->save("sample_rate", combo_dev_rates.getSelectedId());
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
		auto area = getLocalBounds().reduced(_magic::margin * 2);

		combo_dev_types.setBounds(area.removeFromTop(_magic::height));
		area.removeFromTop(_magic::margin);
		combo_dev_outputs.setBounds(area.removeFromTop(_magic::height));
		area.removeFromTop(_magic::margin);
		combo_dev_rates.setBounds(area.removeFromTop(_magic::height));
		area.removeFromTop(_magic::margin);

		auto line = area.removeFromTop(_magic::height);
		checkbox_bpfH.setBounds(line.removeFromLeft(60));
		slider_freq_high.setBounds(line.removeFromLeft(line.getWidth() - (60 + _magic::label) - _magic::margin * 2));
		line.removeFromLeft(_magic::margin);
		combo_order.setBounds(line.removeFromLeft(60 + _magic::margin));
		line.removeFromLeft(_magic::margin);
		checkbox_bpfL.setBounds(line);
		area.removeFromTop(_magic::margin * 2 + _magic::height);

		line = area.removeFromTop(_magic::height);
		line.removeFromLeft(_magic::label);
		combo_buff_size.setBounds(line);
		area.removeFromTop(_magic::margin);
		line = area.removeFromTop(_magic::height);
		checkbox_tone.setBounds(line.removeFromLeft(_magic::label - _magic::margin));
		line.removeFromLeft(_magic::margin);
		combo_tone.setBounds(line);
		area.removeFromTop(_magic::margin);

		area.removeFromBottom(_magic::margin);
		calibrations_component.setBounds(area.removeFromBottom(280));

		area.removeFromBottom(_magic::height + _magic::margin);
		button_stat_reset.setBounds(area.removeFromBottom(_magic::height));
		area.removeFromBottom(_magic::margin);
		button_zero_reset.setBounds(area.removeFromBottom(_magic::height));
		area.removeFromBottom(_magic::margin);
		button_zero.setBounds(area.removeFromBottom(_magic::height));
		area.removeFromBottom(_magic::margin);

		area.removeFromLeft(_magic::label);
		auto right = area.removeFromRight(getWidth() / 2);
		right.removeFromRight(_magic::margin * 2);

		for (size_t k = 0; k < 3; k++) {
			labels_stat.at(k).at(1)->setBounds(area.removeFromTop(_magic::height));
			if (k < 2) area.removeFromTop(_magic::margin);
		}
		for (size_t k = 0; k < 3; k++) {
			labels_stat.at(k).at(2)->setBounds(right.removeFromTop(_magic::height));
			if (k < 2) right.removeFromTop(_magic::margin);
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
            rms.at(0) *= calibrations_component.get_coeff(0); // bug: при неучтённом префиксе
            rms.at(1) *= calibrations_component.get_coeff(1);
			print = prefix_v;
		}
		else {
            rms.at(0) = signal.gain2db(rms.at(0)) - signal.zero_get(0);
            rms.at(1) = signal.gain2db(rms.at(1)) - signal.zero_get(1);
			print = [](double value) { return String(value, 3) + " dB"; };
		}
        rms.push_back(abs(rms.at(0) - rms.at(1)));
        signal.minmax_set(rms);

        array<String, 3> printed;
		for (size_t k = 0; k < 3; k++) 
        {
            printed.fill("--");
            auto minmax = signal.minmax_get(k);

            if (isfinite(rms.at(k)))    printed.at(0) = print(rms.at(k));
            if (isfinite(minmax.at(0))) printed.at(1) = print(minmax.at(0));
            if (isfinite(minmax.at(1))) printed.at(2) = print(minmax.at(1));

            labels_stat.at(k).at(1)->setText(printed.at(0), dontSendNotification);
            labels_stat.at(k).at(2)->setText(printed.at(1) + " .. " + printed.at(2), dontSendNotification);
            labels_stat.at(k).at(1)->setColour(Label::textColourId, isfinite(rms.at(k)) ? Colours::black : Colours::grey);
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

	array <array<unique_ptr<Label>, 3>, 3> labels_stat;

	Label
		label_buff_size { {}, "Buff size" },
		cal_label       { {}, "Cal"       };
	ToggleButton
		checkbox_tone   { "Tone"          },
		checkbox_bpfH   { "bpfH"          },
		checkbox_bpfL   { "bpfL"          };

	theme::checkbox_right_text_lf_ theme_right_text;
	theme::checkbox_right_lf_      theme_right;

	signal_                 signal;
	calibrations_component_ calibrations_component;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (main_component_)
};