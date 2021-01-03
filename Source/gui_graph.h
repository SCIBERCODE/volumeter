#pragma once
#include <JuceHeader.h>
#include "signal.h"

extern unique_ptr<settings_> __opt;

// bug: заливать не нынешнее, а прошлое положение надписи
//      при совпадении границ надписи отображать правый канал
//      начинать график с устоявшихся значений
//      провалы вверх при переключении фильтра

class component_graph_ : public Component
{
private:
    Rectangle<int>      _plot;
    Rectangle<int>      _plot_indented;
    unique_ptr<float[]> _extremes;
    unique_ptr<circle_> _graph_data;

public:
    component_graph_ () { reset(); }
    ~component_graph_() {          }

    void reset() {
        auto display_width = 0;
        for (const auto& display : Desktop::getInstance().getDisplays().displays)
            display_width += display.userArea.getWidth();

        _graph_data = make_unique<circle_>(display_width);
    }

    void enqueue(const vector<double>& rms) {
        if (__opt->get_int(L"graph_paused") == 0) {
            if (isfinite(rms.at(LEFT))) { // todo: syncrochannels
                _graph_data->enqueue(LEFT,  static_cast<float>(rms.at(LEFT )));
                _graph_data->enqueue(RIGHT, static_cast<float>(rms.at(RIGHT)));
                repaint(); // todo: оптимизировать
            }
        }
    }

    //=========================================================================================
    void draw_graph_line(const volume_t channel, Graphics& g) {
        //=====================================================================================
        float offset = 0.0f;
        float value;
        Path  path;

        if (!_graph_data->get_first_value(channel, _plot.getWidth(), value)) // bug: устранить мерцание в момент закольцовывания буфера
            return;
        do
        {
            if (!isfinite(value)) break;
            auto x = _plot.getRight() - offset;
            auto y = -value * (_plot.getHeight() / abs(_extremes[MIN] - _extremes[MAX]));
            if (path.isEmpty())
                path.startNewSubPath(x, y);

            path.lineTo(x, y);
            offset++;
        }
        while (_graph_data->get_next_value(value));
        if (path.getLength() == 0.0f)
            return;

        path.scaleToFit(
            _plot_indented.getWidth() - offset,
            static_cast<float>(_plot_indented.getY()),
            offset,
            static_cast<float>(_plot_indented.getHeight()),
            false
        );
        g.setColour(channel == LEFT ? Colours::black : Colours::green);
        g.strokePath(path, PathStrokeType(1.0f));
    }

    // todo: подсветка точки треугольником на линии графика
    //=========================================================================================
    void draw_extremes(const volume_t channel, Graphics& g) {
        //=====================================================================================
        float value;

        if (!_graph_data->get_first_value(channel, _plot.getWidth(), value))
            return;

        float offset = 0.0f;
        for ( ; ; )
        {
            offset++;
            for (auto extremum = MIN; extremum < EXTREMES_SIZE; extremum++)
            {
                //if (round_flt(value) == round_flt(extremes.at(MAX)))
                //if (is_about_equal(value, extremes.at(MAX)))
                if (String(value, 3) == String(_extremes[extremum], 3)) // todo: индостан
                {
                    const auto text = String(value, 3) + L" dB";
                    const auto text_width = g.getCurrentFont().getStringWidth(text);
                    Rectangle<int> rect =
                    {
                        static_cast<int>(_plot.getRight() - offset - (text_width / 2)),
                        static_cast<int>(extremum == MAX ? _plot.getY() + 1 : _plot_indented.getBottom() + 1),
                        text_width,
                        13
                    };
                    int overhung[2] = {
                        rect.getX()     - _plot.getX()     - 2 /* отступ от рамки */,
                        _plot.getRight() - rect.getRight() - 2,
                    };
                    if (overhung[0] < 0) rect.setCentre(rect.getCentreX() - overhung[0], rect.getCentreY()); // надпись упирается слева
                    if (overhung[1] < 0) rect.setCentre(rect.getCentreX() + overhung[1], rect.getCentreY()); // справа

                    /*g.setColour(Colours::yellow);
                    if (channel == LEFT) {
                        if (extremum == MAX) g.fillRect(_plot.withBottom(y_top - 1));
                        if (extremum == MIN) g.fillRect(_plot.withTop(y_bottom + 1));
                    }
                    else*/
                    g.setColour(Colours::white);
                    g.fillRect(rect);

                    g.setColour(channel == LEFT ? Colours::black : Colours::green);
                    g.drawText(text, rect, Justification::centred, false);
                    break; // todo: несколько ключевых частот, в зависимости от свободного места
                }
            }
            if (!_graph_data->get_next_value(value))
                break;
        }
    }

    //=========================================================================================
    void draw_legend(Graphics& g) {
        //=====================================================================================
        const size_t line_width = 30;
        Rectangle<int> rect
        (
            _plot.getX(),
            static_cast<int>(_plot_indented.getBottom()),
            _plot.getWidth() - theme::margin,
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
    }

    void paint(Graphics& g) override
    {
        _plot          = getLocalBounds();
        _plot_indented = getLocalBounds();

        _plot_indented.reduce(0, theme::graph_indent);

        g.setColour(Colours::white);
        g.fillRect(_plot);

        const float dotted_pattern[2] = { 2.0f, 3.0f };
        Line<int> lines[2] = {
            {_plot_indented.getX(), _plot_indented.getY(),      _plot_indented.getRight(), _plot_indented.getY()      },
            {_plot_indented.getX(), _plot_indented.getBottom(), _plot_indented.getRight(), _plot_indented.getBottom() }
        };
        g.setColour(Colours::grey);
        g.drawDashedLine(lines[0].toFloat(), dotted_pattern, _countof(dotted_pattern));
        g.drawDashedLine(lines[1].toFloat(), dotted_pattern, _countof(dotted_pattern));

        for (auto channel = LEFT; channel <= RIGHT; channel++)
        {
            if (channel == LEFT  && __opt->get_int(L"graph_left" ) == 0) continue;
            if (channel == RIGHT && __opt->get_int(L"graph_right") == 0) continue;

            _extremes = _graph_data->get_extremes(channel, _plot.getWidth());
            draw_graph_line(channel, g);
            draw_extremes(channel, g);
            //draw_legend(g);
        }
        g.setColour(Colours::grey);
        g.drawRect(_plot);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(component_graph_)

};
