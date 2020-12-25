#pragma once
#include <JuceHeader.h>
#include <spuce/filters/iir.h>
#include <spuce/filters/butterworth_iir.h>
#include "main.h"

using namespace spuce;

extern unique_ptr<settings_> _opt;

//=========================================================================================
class sin_
//=========================================================================================
{
public:
	sin_() : sampling_rate(44100.0),
			 angle        (0.0),
			 angle_delta  (0.0),
			 freq         (0.0)
	{ }
	void set_sample_rate(double sample_rate) {
		sampling_rate = sample_rate;
	}
	void reset() {
		angle = 0.0;
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
class circle_
//=========================================================================================
{
private:
	unique_ptr<float[]> buffer;
	size_t              head;
	size_t              tail;
	size_t              max_size;
public:
	circle_(size_t max_size) : buffer(make_unique<float[]>(max_size)),
							   max_size(max_size),
							   head(0),
							   tail(0)
	{
		clear();
	};

	void clear()
	{
		fill_n(buffer.get(), max_size, numeric_limits<float>::quiet_NaN());
	}

	void enqueue(float item)
	{
		if (max_size && item)
		{
			buffer[tail] = item * item;
			tail = (tail + 1) % max_size;
		}
	}

	double get_rms()
	{
		double sum  = 0.0;
		size_t size = 0;

		for (size_t k = 0; k < max_size; ++k)
		{
			auto sample = buffer[k];
			if (isfinite(sample) && sample) {
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
		zero.fill(0.0);
		minmax_clear();
	}

	void zero_set() {
		lock_guard<mutex> locker(audio_process);
		zero.at(0) = gain2db(buff.at(0)->get_rms());
		zero.at(1) = gain2db(buff.at(1)->get_rms());
		minmax_clear();
	}
	auto zero_get(size_t ch) {
		return zero.at(ch);
	}
	void zero_clear() {
		zero.fill(0.0);
		minmax_clear();
	}

	auto minmax_set(const vector<double>& rms) {
		for (size_t k = 0; k < 3; k++) // bug: на вольтах при переключении нижн€€ граница не похожа на правду            
			if (isfinite(rms.at(k)))
			{
				if (!isfinite(minmax.at(k).at(0))) minmax.at(k).at(0) = rms.at(k);
				if (!isfinite(minmax.at(k).at(1))) minmax.at(k).at(1) = rms.at(k);
				minmax.at(k).at(0) = min(minmax.at(k).at(0), rms.at(k));
				minmax.at(k).at(1) = max(minmax.at(k).at(1), rms.at(k));
			}
	}
	auto minmax_get(size_t ch) {
		return minmax.at(ch);
	}
	void minmax_clear() {
		for (auto& sub : minmax)
			sub.fill(numeric_limits<float>::quiet_NaN());
	}

	double gain2db(double gain) {
		return 20 * log10(gain);
	}
	void set_freq(double freq) {
		osc.set_freq(static_cast<float>(freq));
		osc.reset();
	}
	void change_buff_size(int new_size) {
		lock_guard<mutex> locker(audio_process);
		if (new_size && sample_rate) {
			auto number_of_samples = static_cast<size_t>(new_size / (1000.0 / sample_rate));
			buff.at(0) = make_unique<circle_>(number_of_samples);
			buff.at(1) = make_unique<circle_>(number_of_samples);
		}
	}
	void clear_data() {
		lock_guard<mutex> locker(audio_process);
		if (buff.at(0)) buff.at(0)->clear();
		if (buff.at(1)) buff.at(1)->clear();
	}
	void set_order(int new_order) {
		order = new_order;
	}
	vector<double> get_rms() {
		vector<double> result;
		if (buff.at(0) && buff.at(1))
		{
			result.push_back(buff.at(0)->get_rms());
			result.push_back(buff.at(1)->get_rms());
		};
		return result;
	}

	void filter_init()
	{
		lock_guard<mutex> locker(audio_process);
		for (size_t ch = 0; ch < 2; ++ch)
		{
			if (_opt->load_int("use_bpfH", "0"))
			{
				iir_coeff bpfH(order, filter_type::high);
				butterworth_iir(bpfH, 20.0 / sample_rate, 3.0);
				iir.at(ch).at(0) = make_unique<spuce::iir<float_type, float_type>>(bpfH);
			}
			if (_opt->load_int("use_bpfL", "0"))
			{
				iir_coeff bpfL(order, filter_type::low);
				butterworth_iir(bpfL, _opt->load_int("bpfL_value", "15000") / sample_rate, 3.0);
				iir.at(ch).at(1) = make_unique<spuce::iir<float_type, float_type>>(bpfL);
			}
		}
	};

	void prepare_to_play(double sample_rate_)
	{
		sample_rate = sample_rate_;

		osc.set_sample_rate(sample_rate);
		osc.set_freq(static_cast<float>(_opt->load_int("tone_value")));
		osc.reset();

		filter_init();
	}

	void next_audio_block(const AudioSourceChannelInfo& buffer)
	{
		if (audio_process.try_lock())
		{
			if (buff.at(0) && buff.at(1))
			{
				auto r = buffer.buffer->getArrayOfReadPointers();
				auto w = buffer.buffer->getArrayOfWritePointers();

				for (auto k = 0; k < buffer.numSamples; ++k)
				{
					buff.at(0)->enqueue(r[0][k]);
					buff.at(1)->enqueue(r[1][k]);
				}

				if (_opt->load_int("tone", "0"))
				{
					for (auto sample = 0; sample < buffer.numSamples; ++sample)
					{
						auto tone_sample = osc.sample();
						w[0][sample] = tone_sample;
						w[1][sample] = tone_sample;
					}
				}
				else {
					auto use_bpfH = _opt->load_int("use_bpfH", "0") && iir.at(0).at(0);
					auto use_bpfL = _opt->load_int("use_bpfL", "0") && iir.at(0).at(1);

					if (use_bpfH || use_bpfL)
						for (size_t ch = 0; ch < 2; ++ch)
							for (auto sample_idx = 0; sample_idx < buffer.numSamples; ++sample_idx)
							{
								float_type sample = r[ch][sample_idx];
								if (use_bpfH) sample = iir.at(ch).at(0)->clock(sample);
								if (use_bpfL) sample = iir.at(ch).at(1)->clock(sample);
								w[ch][sample_idx] = static_cast<float>(sample);
							}

					if (!use_bpfH && !use_bpfL)
						buffer.clearActiveBufferRegion();
				}
			}
			audio_process.unlock();
		}
	}	

private:
	double sample_rate;
	size_t order;
	mutex  audio_process;
	sin_   osc;

	array<double, 2>                                            zero;   // [ch]
	array<array<double, 2>, 3>                                  minmax; // [ch, balance][min, max]
	array<unique_ptr<circle_>, 2>                               buff;   // [ch]
	array<array<unique_ptr<iir<float_type, float_type>>, 2>, 2> iir;    // [ch][use_bpfH, use_bpfL]
};