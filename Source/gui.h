#pragma once
#include <JuceHeader.h>
#include "gui_cals.h"

extern unique_ptr<settings_> _opt;

class main_component_ : public AudioAppComponent,
                        public Timer,
                        public ChangeListener
{
protected:

	const vector<float> buff_size_list = { 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 10.0f };
	const vector<int>   tone_list      = { 10, 20, 200, 500, 1000, 5000, 10000, 20000 };
	const vector<int>   order_list     = { 1, 2, 4, 10, 20, 40, 60, 80, 100, 120, 140, 200 };

//=========================================================================================
public:
//=========================================================================================

	main_component_() : calibrations_component(signal)
	{
		addAndMakeVisible (&zero_button);
		zero_button.setButtonText ("Set zero");
		zero_button.onClick = [this]
		{
			signal.set_zero();
		};

		addAndMakeVisible(&reset_zero_button);
		reset_zero_button.setButtonText("Reset zero");
		reset_zero_button.onClick = [this]
		{
			signal.reset_zero();
		};

		addAndMakeVisible(reset_minmax);
		reset_minmax.setButtonText("Reset stat");
		reset_minmax.onClick = [this]
		{
			signal.clear_stat();
		};

		addAndMakeVisible(tone_checkbox);
		addAndMakeVisible(tone_combo);
		tone_checkbox.setToggleState(_opt->load_int("tone", "0"), dontSendNotification);
		tone_checkbox.setLookAndFeel(&theme_right_text);
		tone_checkbox.onClick = [this]
		{
			signal.clear_stat();
			_opt->save("tone", tone_checkbox.getToggleState());
		};
		for (auto const item : tone_list)
			tone_combo.addItem(prefix(item, "Hz", 0), item);

		tone_combo.setSelectedId(_opt->load_int("tone_value"));
		tone_combo.onChange = [this]
		{
			auto val = tone_combo.getSelectedId();
			signal.set_freq(val);
			_opt->save("tone_value", val);
		};

		addAndMakeVisible(buff_size_combo);
		addAndMakeVisible(buff_size_label);
		buff_size_label.attachToComponent(&buff_size_combo, true);
		for (auto const item : buff_size_list)
			buff_size_combo.addItem(prefix(item, "s", 0), (int)(item * 1000));

		buff_size_combo.setSelectedId(_opt->load_int("buff_size"));
		buff_size_combo.onChange = [this]
		{
			auto val = buff_size_combo.getSelectedId();
			signal.change_buff_size(val);
			_opt->save("buff_size", val);
		};		

		addAndMakeVisible(l_label_value); addAndMakeVisible(l_label_value_minmax);
		addAndMakeVisible(l_label);
		l_label.attachToComponent(&l_label_value, true);
		l_label_value.setFont(l_label_value.getFont().boldened());
		l_label_value_minmax.setJustificationType(Justification::right);

		addAndMakeVisible(r_label_value); addAndMakeVisible(r_label_value_minmax);
		addAndMakeVisible(r_label);
		r_label.attachToComponent(&r_label_value, true);
		r_label_value.setFont(r_label_value.getFont().boldened());
		r_label_value_minmax.setJustificationType(Justification::right);

		addAndMakeVisible(b_label_value); addAndMakeVisible(b_label_value_minmax);
		addAndMakeVisible(balance_label);
		balance_label.attachToComponent(&b_label_value, true);
		b_label_value.setFont(b_label_value.getFont().boldened());
		b_label_value_minmax.setJustificationType(Justification::right);

		const OwnedArray<AudioIODeviceType>& types = deviceManager.getAvailableDeviceTypes();

		addAndMakeVisible(types_combo);
		addAndMakeVisible(outputs_combo);
		addAndMakeVisible(rates_combo);
		if (types.size() > 1)
		{
			int index = 0;
			for (int i = 0; i < types.size(); ++i)
			{
				auto name_type = types.getUnchecked(i)->getTypeName();
				types_combo.addItem(name_type, i + 1);
				if (name_type == "ASIO") index = i;
			}

			types_combo.setSelectedItemIndex(index);
			types_combo.onChange = [this]
			{
				AudioIODeviceType *type = deviceManager.getAvailableDeviceTypes()[types_combo.getSelectedId() - 1];
				if (type)
				{
					deviceManager.setCurrentAudioDeviceType(type->getTypeName(), true);
					
					const StringArray devs(type->getDeviceNames());
					outputs_combo.clear(dontSendNotification);

					auto device = _opt->load_str("device");

					for (int i = 0; i < devs.size(); ++i) {
						outputs_combo.addItem(devs[i], i + 1);
						if (device == devs[i])
							outputs_combo.setSelectedItemIndex(i);
					}
					if (device.isEmpty())
						outputs_combo.setSelectedItemIndex(0);
				}
			};
		}
		outputs_combo.onChange = [this]
		{
			signal.clear_data();
			signal.clear_stat();

			_opt->save("device", outputs_combo.getText());
			auto sample_rate = _opt->load_int("sample_rate", "48000");

			auto config                     = deviceManager.getAudioDeviceSetup();
			config.outputDeviceName         = outputs_combo.getText();
			config.inputDeviceName          = config.outputDeviceName;
			config.useDefaultInputChannels  = true;
			config.useDefaultOutputChannels = true;
			config.sampleRate               = sample_rate;
			deviceManager.setAudioDeviceSetup(config, true);

			if (auto* currentDevice = deviceManager.getCurrentAudioDevice()) {
				rates_combo.clear(dontSendNotification);
				for (auto rate : currentDevice->getAvailableSampleRates())
				{
					auto intRate = roundToInt(rate);
					rates_combo.addItem(String(intRate) + " Hz", intRate);
				}
				rates_combo.setSelectedId(sample_rate, dontSendNotification);
			}
		};
		rates_combo.onChange = [this]
		{
			_opt->save("sample_rate", rates_combo.getSelectedId());
			outputs_combo.onChange();
		};

		// === filter ==================================================================

		addAndMakeVisible(bpfH_checkbox);
		addAndMakeVisible(bpfL_checkbox);
		addAndMakeVisible(high_slider);
		addAndMakeVisible(order_combo);

		bpfH_checkbox.setToggleState(_opt->load_int("use_bpfH", "0"), dontSendNotification);
		bpfH_checkbox.setLookAndFeel(&theme_right);

		bpfH_checkbox.onClick = [this] {
			_opt->save("use_bpfH", bpfH_checkbox.getToggleState());
			signal.filter_init();
		};
		bpfL_checkbox.setToggleState(_opt->load_int("use_bpfL", "0"), dontSendNotification);
		bpfL_checkbox.onClick = [this] {
			_opt->save("use_bpfL", bpfL_checkbox.getToggleState());
			signal.filter_init();
		};

		high_slider.setSliderStyle(Slider::LinearHorizontal);
		high_slider.setTextBoxStyle(Slider::TextBoxRight, false, 60, 20);
		high_slider.setRange(20, 20000, 1);
		high_slider.setValue(15000, dontSendNotification);
		high_slider.onDragEnd = [this] {
			signal.filter_init();
		};

		for (auto const item : order_list)
			order_combo.addItem(String(item), item);

		order_combo.setSelectedId(120, dontSendNotification);
		order_combo.setTooltip("Filter Order");
		order_combo.onChange = [this]
		{
			signal.set_order(order_combo.getSelectedId());
			signal.filter_init();
		};

		addAndMakeVisible(calibrations_component);
		calibrations_component.update();

		setSize(420, 670);
		startTimer(100);

		setAudioChannels(2, 2);
	}

	~main_component_() override
	{
		shutdownAudio();
		tone_checkbox.setLookAndFeel(nullptr);
		bpfH_checkbox.setLookAndFeel(nullptr);
	}

	void changeListenerCallback(ChangeBroadcaster* /*source*/) { }
	void releaseResources() override { }

	void prepareToPlay (int /*samples_per_block*/, double sample_rate) override
	{
		buff_size_combo.onChange();
		signal.prepare_to_play(sample_rate);
	}

	void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
	{
		signal.next_audio_block(bufferToFill);
	}

	void resized() override
	{
		auto area = getLocalBounds().reduced(_s::margin * 2);

		area.removeFromBottom(_s::margin);
		calibrations_component.setBounds(area.removeFromBottom(280));
		area.removeFromBottom(_s::height);

		area.removeFromBottom(_s::margin);
		reset_minmax.setBounds(area.removeFromBottom(_s::height));
		area.removeFromBottom(_s::margin);
		reset_zero_button.setBounds(area.removeFromBottom(_s::height));
		area.removeFromBottom(_s::margin);
		zero_button.setBounds(area.removeFromBottom(_s::height));
		area.removeFromBottom(_s::margin);

		types_combo.setBounds(area.removeFromTop(_s::height));
		area.removeFromTop(_s::margin);
		outputs_combo.setBounds(area.removeFromTop(_s::height));
		area.removeFromTop(_s::margin);
		rates_combo.setBounds(area.removeFromTop(_s::height));
		area.removeFromTop(_s::margin);

		auto line = area.removeFromTop(_s::height);
		bpfH_checkbox.setBounds(line.removeFromLeft(60));
		high_slider.setBounds(line.removeFromLeft(line.getWidth() - (60 + _s::label) - _s::margin * 2));
		line.removeFromLeft(_s::margin);
		order_combo.setBounds(line.removeFromLeft(60 + _s::margin));
		line.removeFromLeft(_s::margin);
		bpfL_checkbox.setBounds(line);
		area.removeFromTop(_s::margin);

		area.removeFromTop(_s::height + _s::margin);

		line = area.removeFromTop(_s::height);
		line.removeFromLeft(_s::label);
		buff_size_combo.setBounds(line);
		area.removeFromTop(_s::margin);
		line = area.removeFromTop(_s::height);
		tone_checkbox.setBounds(line.removeFromLeft(_s::label - _s::margin));
		line.removeFromLeft(_s::margin);
		tone_combo.setBounds(line);
		area.removeFromTop(_s::margin);

		area.removeFromLeft(_s::label);
		auto right = area.removeFromRight(getWidth() / 2);
		right.removeFromRight(_s::margin * 2);

		l_label_value.setBounds(area.removeFromTop(_s::height));
		area.removeFromTop(_s::margin);
		r_label_value.setBounds(area.removeFromTop(_s::height));
		area.removeFromTop(_s::margin);
		b_label_value.setBounds(area.removeFromTop(_s::height));

		l_label_value_minmax.setBounds(right.removeFromTop(_s::height));
		right.removeFromTop(_s::margin);
		r_label_value_minmax.setBounds(right.removeFromTop(_s::height));
		right.removeFromTop(_s::margin);
		b_label_value_minmax.setBounds(right.removeFromTop(_s::height));

	}

	void display(Label &lbl, String text) {
		if (text.contains("inf") || text.contains("nan"))
		{
			lbl.setText("--", dontSendNotification);
			lbl.setColour(Label::ColourIds::textColourId, Colours::grey);
		}
		else {
			lbl.setText(text, dontSendNotification);
			lbl.setColour(Label::ColourIds::textColourId, Colours::black);
		}
	}

	auto display(String text) {
		if (text.contains("inf") || text.contains("nan") || text.contains("555.000"))
		{
			return String("--");
		}
		return text;
	}

	void timerCallback() override
	{
		auto lrb = signal.get_lrb();
		function<String(double)> print;

		auto calibrate = calibrations_component.is_active();
		if (calibrate)
		{
			lrb[0] *= calibrations_component.coef()[0]; // bug: при неучтённом префиксе
			lrb[1] *= calibrations_component.coef()[1];
			print = prefix_v;
		}
		else {
			lrb[0] = signal.gain2db(lrb[0]) - signal.stat.zero[0];
			lrb[1] = signal.gain2db(lrb[1]) - signal.stat.zero[1];
			print = [](double value) { return String(value, 3) + " dB"; };
		}
		lrb[2] = abs(lrb[0] - lrb[1]);

		for (size_t k = 0; k < 3; k++) {
			if (display(String(lrb[k], 3)) != "--")
			{
				if (signal.stat.minmax[k][0] == 555) signal.stat.minmax[k][0] = lrb[k];
				if (signal.stat.minmax[k][1] == 555) signal.stat.minmax[k][1] = lrb[k];
				signal.stat.minmax[k][0] = min(signal.stat.minmax[k][0], lrb[k]); // bug: на вольтах при переключении нижняя граница не похожа на правду
				signal.stat.minmax[k][1] = max(signal.stat.minmax[k][1], lrb[k]);
			}
			display(l_label_value, print(lrb[k])); l_label_value_minmax.setText(display(print(signal.stat.minmax[k][0])) + " .. " + display(print(signal.stat.minmax[k][1])), dontSendNotification);
		}

		if (zero_button.isEnabled()       == calibrate) zero_button.setEnabled(!calibrate);
		if (reset_zero_button.isEnabled() == calibrate) reset_zero_button.setEnabled(!calibrate);
	}

//=========================================================================================
private:
//=========================================================================================

	TextButton zero_button, reset_zero_button, reset_minmax;
	ComboBox   types_combo, outputs_combo, rates_combo, buff_size_combo, tone_combo, order_combo;
	Slider     high_slider;

	Label
		buff_size_label { {}, "Buff size"   },
		cal_label       { {}, "Cal"         },
		l_label         { {}, "Left"        }, l_label_value, l_label_value_minmax,
		r_label         { {}, "Right"       }, r_label_value, r_label_value_minmax,
		balance_label   { {}, "Balance"     }, b_label_value, b_label_value_minmax;
	ToggleButton
		tone_checkbox   { "Tone"            },
		bpfH_checkbox   { "bpfH"            }, bpfL_checkbox { "bpfL" };

	theme::checkbox_right_text_lf_ theme_right_text;
	theme::checkbox_right_lf_      theme_right;

	signal_	      			signal;
	calibrations_component_ calibrations_component;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (main_component_)
};