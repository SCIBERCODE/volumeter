#pragma once
#include <JuceHeader.h>
#include <spuce/filters/iir.h>
#include <spuce/filters/butterworth_iir.h>

#include "main.h"

extern unique_ptr<settings_> _opt;

//=========================================================================================
class sin_
//=========================================================================================
{
public:
	sin_() : sampling_rate(44100.0),
		angle(0.0),
		angle_delta(0.0),
		freq(0.0) { }

	void set_sample_rate(double sample_rate) {
		sampling_rate = sample_rate;
	}
	void reset() {
		angle = 0;
	}
	void set_freq(float frequency) {
		freq = frequency;
		auto cycles_per_sample = frequency / sampling_rate;
		angle_delta = cycles_per_sample * 2.0 * 3.14159265358979323846;
	}
	auto sample() {
		auto current_sample = sin(angle);
		angle += angle_delta;
		return static_cast<float>(current_sample);
	}

private:

	double sampling_rate, freq, angle_delta, angle;
};

//=========================================================================================
template <class T> class circle_ {
//=========================================================================================
private:
	unique_ptr<T[]> buffer;
	size_t          head = 0;
	size_t          tail = 0;
	size_t          max_size;
	T               empty_item;
public:
	void clear() {
		for (size_t k = 0; k < max_size; k++)
			buffer[k] = 555;
	}
	circle_<T>(size_t max_size)
		: buffer(unique_ptr<T[]>(new T[max_size])), max_size(max_size)
	{
		//std::generate(std::begin(buffer), std::end(buffer), [] { return 0; });
		clear();
	};

	void enqueue(T item)
	{
		if (max_size == 0) return;
		buffer[tail] = item * item;
		tail = (tail + 1) % max_size;
	}

	double get_value()
	{
		double sum = 0.0;
		size_t size = 0;

		for (size_t k = 0; k < max_size; ++k)
		{
			auto sample = buffer[k];
			if (sample != 555 && sample) {
				sum += sample;
				size++;
			}
		}
		return sqrt(sum / size);
	}
};

//=========================================================================================
class signal_
//=========================================================================================
{
public:

	signal_() : sample_rate(0.0), order(120)
	{
		buff[0].reset(new circle_<float>(0));
		buff[1].reset(new circle_<float>(0));
	}
	void set_zero() {
		stat.zero[0] = gain2db(buff[0]->get_value());
		stat.zero[1] = gain2db(buff[1]->get_value());
		clear_stat();
	}
	void clear_stat() {
		stat.clear_stat();
	}
	void reset_zero() {
		stat.zero[0] = 0;
		stat.zero[1] = 0;
		clear_stat();
	}
	double gain2db(double gain) {
		return 20 * log10(gain);
	}
	void set_freq(double freq) {
		osc.set_freq(static_cast<float>(freq));
		osc.reset();
	}
	void change_buff_size(int new_size) {
		if (new_size && sample_rate) {
			auto samples = static_cast<size_t>(new_size / (1000.0 / sample_rate));
			buff[0].reset(new circle_<float>(samples));
			buff[1].reset(new circle_<float>(samples));
		}
	}
	void clear_data() {
		buff[0]->clear();
		buff[1]->clear();
	}
	void set_order(int new_order) {
		order = new_order;
	}
	array<double, 3> get_lrb() {
		auto l = buff[0]->get_value();
		auto r = buff[1]->get_value();
		
		array<double, 3> lrb = { l, r, abs(l - r) };
		return lrb;
	}

	void filter_init()
	{
		/*{
			lock_guard<mutex> locker(audio_process);

			if (_opt->load_int("use_bpfH", "0"))
			{
				spuce::iir_coeff bpfH(order, spuce::filter_type::high);
				spuce::butterworth_iir(bpfH, 20.0 / sample_rate, 3.0);
				iir[0].reset(new spuce::iir<spuce::float_type, spuce::float_type>(bpfH));
			}
			if (_opt->load_int("use_bpfL", "0"))
			{
				spuce::iir_coeff bpfL(order, spuce::filter_type::low);
				spuce::butterworth_iir(bpfL, high_slider.getValue() / sample_rate, 3.0);
				iir[1].reset(new spuce::iir<spuce::float_type, spuce::float_type>(bpfL));
			}
		}
		high_slider.repaint();*/
	};

	void prepare_to_play(double sample_rate_)
	{
		sample_rate = sample_rate_;

		osc.set_sample_rate(sample_rate);
		osc.set_freq(static_cast<float>(_opt->load_int("tone_value")));
		osc.reset();

		//filter_init();
	}

	void next_audio_block(const AudioSourceChannelInfo& bufferToFill)
	{
		auto r = bufferToFill.buffer->getArrayOfReadPointers();
		auto w = bufferToFill.buffer->getArrayOfWritePointers();

		for (int sample_idx = 0; sample_idx < bufferToFill.numSamples; ++sample_idx)
		{
			if (r[0][sample_idx]) buff[0]->enqueue(r[0][sample_idx]);
			if (r[1][sample_idx]) buff[1]->enqueue(r[1][sample_idx]);
		}

		if (_opt->load_int("tone"))
		{
			for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
			{
				auto tone_sample = osc.sample();
				w[0][sample] = tone_sample;
				w[1][sample] = tone_sample;
			}
		}
		/*else {
			if (audio_process.try_lock())
			{
				auto use_bpfH = bpfH_checkbox.getToggleState() && iir[0];
				auto use_bpfL = bpfL_checkbox.getToggleState() && iir[1];

				if (use_bpfH || use_bpfL)
				{
					for (size_t ch = 0; ch < 2; ++ch) {
						for (int sample_idx = 0; sample_idx < bufferToFill.numSamples; ++sample_idx)
						{
							spuce::float_type sample = r[ch][sample_idx]; // random.nextFloat();
							if (use_bpfH) sample = iir[0]->clock(sample);
							if (use_bpfL) sample = iir[1]->clock(sample);
							w[ch][sample_idx] = (float)sample;
						}
						if (use_bpfH) iir[0]->reset();
						if (use_bpfL) iir[1]->reset();
					}
				}
				audio_process.unlock();
			}
		}*/
		else bufferToFill.clearActiveBufferRegion();
	}	

protected:

	struct stat_t {
		double zero[2] = { 0, 0 };
		double minmax[3][2] = { { 555, 555 }, { 555, 555 }, { 555, 555 } };

		void clear_stat() {
			for (size_t k = 0; k < 3; k++) {
				minmax[k][0] = 555;
				minmax[k][1] = 555;
			}
		}
	};

public:
	stat_t       stat;
private:
	double       sample_rate;
	size_t       order;
	mutex        audio_process;
	sin_         osc;

	unique_ptr<circle_<float>>                                   buff[2];
	unique_ptr<spuce::iir<spuce::float_type, spuce::float_type>> iir[2];

};