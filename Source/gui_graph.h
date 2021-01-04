#pragma once
#include <JuceHeader.h>
#include "signal.h"

extern unique_ptr<settings_> __opt;

// bug: начинать график с устоявшихся значений
//      провалы вверх при переключении фильтра

class component_graph_ : public Component
{
private:
    Rectangle<int>      _plot;
    Rectangle<int>      _plot_indented;
    unique_ptr<float[]> _extremes;
    unique_ptr<circle_> _graph_data;
    RectangleList<int>  _placed_extremes;

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
    /** построение линии сигнала, масштабирование и отрисовка
    */
    void draw_graph_line(const volume_t channel, Graphics& g) {
        //=====================================================================================
        auto  offset = 0.0f;
        float value;
        Path  path;

        if (_plot_indented.isEmpty())
            return;

        if (!_graph_data->get_first_value(channel, _plot.getWidth(), value)) // bug: устранить мерцание в момент закольцовывания буфера
            return;

        for ( ; ; offset++)
        {
            if (!isfinite(value)) break;
            auto x = _plot.getRight() - offset;
            auto y = -value * (_plot_indented.getHeight() / abs(_extremes[MIN] - _extremes[MAX]));
            if (path.isEmpty())
                path.startNewSubPath(x, y);

            path.lineTo(x, y);

            if (!_graph_data->get_next_value(value))
                break;
        }
        if (path.getLength() == 0.0f) // bug: как минимум единажды эта проверка оказалось недостаточной
            return;

        const auto subpixel_correction = 0.2f;

        path.scaleToFit(
            _plot_indented.getWidth()  - offset,
            _plot_indented.getY()      - subpixel_correction,
            offset,
            _plot_indented.getHeight() + subpixel_correction * 2,
            false
        );
        g.setColour(channel == LEFT ? Colours::black : Colours::green);
        g.strokePath(path, PathStrokeType(1.0f + subpixel_correction));
    }

    // todo: подсветка точки треугольником на линии графика
    //=========================================================================================
    /** отрисовка экстремумов (правый канал перекрывает левый)
    */
    void draw_extremes(const volume_t channel, Graphics& g) {
        //=====================================================================================
        float value;
        for (auto extremum = MIN; extremum < EXTREMES_SIZE; extremum++)
        {
            size_t offset = 0;
            if (!_graph_data->get_first_value(channel, _plot.getWidth(), value))
                continue;

            for ( ; ; offset++)
            {
                //if (round_flt(value) == round_flt(extremes.at(MAX)))
                //if (is_about_equal(value, extremes.at(MAX)))
                if (String(value, 3) == String(_extremes[extremum], 3)) // todo: индостан
                {
                    const auto text = String(value, 3) + L" dB";
                    const auto text_width = g.getCurrentFont().getStringWidth(text);
                    Rectangle<int> rect
                    (
                        _plot.getRight() - offset - (text_width / 2),
                        extremum == MAX ? _plot.getY() + 1 : _plot_indented.getBottom() + 1,
                        text_width,
                        13
                    );
                    int overhung[2] = {
                        rect.getX() - _plot.getX() - 2 /* отступ от рамки */,
                        _plot.getRight() - rect.getRight() - 2,
                    };
                    if (overhung[0] < 0) rect.setCentre(rect.getCentreX() - overhung[0], rect.getCentreY()); // надпись упирается слева
                    if (overhung[1] < 0) rect.setCentre(rect.getCentreX() + overhung[1], rect.getCentreY()); // справа

                    // правый график превалирует над левым, экстремумы левого не отображаются вовсе в случае
                    // даже частичного перекрытия значениями правого
                    if (channel == RIGHT || (channel == LEFT && !_placed_extremes.intersectsRectangle(rect)))
                    {
                        g.setColour(Colours::white);
                        g.fillRect(rect);
                        g.setColour(channel == LEFT ? Colours::black : Colours::green);
                        g.drawText(text, rect, Justification::centred, false);
                    }
                    if (channel == RIGHT)
                        _placed_extremes.add(rect);

                    break; // todo: несколько ключевых частот, в зависимости от свободного места
                }
                if (!_graph_data->get_next_value(value))
                    break;

                if (!isfinite(value))
                    break;
            }
        }
    }

    //=========================================================================================
    /** расшифровка цветов графика
    */
    void draw_legend(Graphics& g) {
        //=====================================================================================
        const size_t line_width = 25;
        Rectangle<int> rect
        (
            _plot_indented.getX() + 4,
            _plot_indented.getBottom(),
            _plot_indented.getWidth(),
            13
        );
        for (auto channel = LEFT; channel <= RIGHT; channel++)
        {
            const auto text = __stat_captions.at(channel);
            const auto text_width = g.getCurrentFont().getStringWidth(text);
            g.setColour(channel == LEFT ? Colours::black : Colours::green);
            g.drawFittedText(text, rect.removeFromLeft(text_width + theme::margin), Justification::centredLeft, 1);
            auto rect_float = rect.removeFromLeft(line_width);
            g.fillRect(rect_float.toFloat().withY(rect.getY() + 1.0f).reduced(0, 5.6f));
            rect.removeFromLeft(theme::margin);
        }
    }

    //=========================================================================================
    /** рисование линий, заливок и прочей индикации
    */
    void draw_indications(Graphics& g) {
        //=====================================================================================
        const float dotted_pattern[2] = { 2.0f, 3.0f };
        Line<int> lines[2] = {
            {_plot_indented.getX(), _plot_indented.getY(),      _plot_indented.getRight(), _plot_indented.getY()      },
            {_plot_indented.getX(), _plot_indented.getBottom(), _plot_indented.getRight(), _plot_indented.getBottom() }
        };
        g.setColour(Colours::grey);
        g.drawDashedLine(lines[0].toFloat(), dotted_pattern, _countof(dotted_pattern));
        g.drawDashedLine(lines[1].toFloat(), dotted_pattern, _countof(dotted_pattern));
    }

    void paint(Graphics& g) override
    {
        _plot          = getLocalBounds();
        _plot_indented = getLocalBounds();

        _plot_indented.reduce(0, theme::graph_indent);
        g.setColour(Colours::white);
        g.fillRect(_plot);

        draw_indications(g);
        //draw_legend(g);

        _placed_extremes.clear();
        for (auto channel = RIGHT; channel != VOLUME_SIZE; channel--)
        {
            if (channel == LEFT  && __opt->get_int(L"graph_left" ) == 0) continue;
            if (channel == RIGHT && __opt->get_int(L"graph_right") == 0) continue;

            _extremes = _graph_data->get_extremes(channel, _plot.getWidth());

            draw_graph_line(channel, g);
            draw_extremes  (channel, g);
        }
        g.setColour(Colours::grey);
        g.drawRect(_plot);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(component_graph_)

};
