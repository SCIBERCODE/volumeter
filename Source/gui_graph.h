#pragma once
#include <JuceHeader.h>

extern unique_ptr<settings_> _opt;

// bug: несколько частот при нахождении одинаковых
//      заливать скорее не нынешнее, а прошлое положение
//      при совпадении границ отображать правый канал
//      начинать график с устоявшихся значений
//      провалы вверх при переключении фильтра

class component_graph_ : public Component
{
public:
    component_graph_ () { reset(); }
    ~component_graph_() {          }

    void reset() {
        auto display_width = 0;
        for (auto display : Desktop::getInstance().getDisplays().displays)
            display_width += display.userArea.getWidth();
        history_stat = make_unique<circle_>(display_width);
    }

    void enqueue(const vector<double>& rms) {
        if (_opt->get_int("graph_paused") == 0) {
            if (isfinite(rms.at(LEFT))) { // todo: syncrochannels
                history_stat->enqueue(LEFT,  static_cast<float>(rms.at(LEFT)));
                history_stat->enqueue(RIGHT, static_cast<float>(rms.at(RIGHT)));
                repaint(); // todo: оптимизировать
            }
        }
    }

    void paint(Graphics& g) override
    {
        auto plot = getLocalBounds();
        g.setColour(Colours::white);
        g.fillRect(plot);

        const float dotted_pattern[2] = { 2.0f, 3.0f };
        auto y_start = plot.getY() + theme::graph_indent;
        auto y_end = plot.getY() + plot.getHeight() - theme::graph_indent;

        g.setColour(Colours::grey);
        g.drawDashedLine(
            Line<float>(static_cast<float>(plot.getX()), y_start, static_cast<float>(plot.getRight()), y_start),
            dotted_pattern, _countof(dotted_pattern)
        );
        g.drawDashedLine(
            Line<float>(static_cast<float>(plot.getX()), y_end, static_cast<float>(plot.getRight()), y_end),
            dotted_pattern, _countof(dotted_pattern)
        );

        float value;
        //=====================================================================================
        for (auto channel = LEFT; channel <= RIGHT; channel++) {
            //=================================================================================
            if (channel == LEFT  && _opt->get_int(L"graph_left")  == 0) continue;
            if (channel == RIGHT && _opt->get_int(L"graph_right") == 0) continue;

            float offset = 0.0f;
            Path path;

            auto minmax = history_stat->get_minmax(channel, plot.getWidth());
            if (history_stat->get_first_value(channel, plot.getWidth(), value)) // bug: устранить мерцание при заполнении буфера
            {
                do {
                    if (!isfinite(value)) break;
                    auto x = plot.getRight() - offset++;
                    auto y = -value * (plot.getHeight() / abs(minmax.at(MIN) - minmax.at(MAX)));
                    if (path.isEmpty())
                        path.startNewSubPath(x, y);

                    path.lineTo(x, y);
                    //=====================================================================================
                    for (auto stat = MIN; stat < STAT_SIZE; stat++) {
                        //=================================================================================
                        //if (round_flt(value) == round_flt(minmax.at(MAX)))
                        //if (is_about_equal(value, minmax.at(MAX)))
                        if (String(value, 3) == String(minmax.at(stat), 3)) // todo: индостан
                        {
                            const auto text = String(value, 3) + L" dB";
                            const auto text_width = g.getCurrentFont().getStringWidth(text);
                            Rectangle<int> rect =
                            {
                                static_cast<int>(plot.getRight() - offset - (text_width / 2)),
                                static_cast<int>(stat == MAX ? y_start - 14 : y_end + 1),
                                text_width,
                                13
                            };
                            int overhung[2] = {
                                rect.getX() - plot.getX(),
                                plot.getRight() - rect.getRight(),
                            };
                            if (overhung[0] < 0) rect.setCentre(rect.getCentreX() - overhung[0], rect.getCentreY());
                            if (overhung[1] < 0) rect.setCentre(rect.getCentreX() + overhung[1], rect.getCentreY());

                            /*g.setColour(Colours::yellow);
                            if (channel == LEFT) {
                                if (stat == MAX) g.fillRect(plot.withBottom(y_start - 1));
                                if (stat == MIN) g.fillRect(plot.withTop(y_end + 1));
                            }
                            else*/
                            g.setColour(Colours::white);
                            g.fillRect(rect);

                            g.setColour(channel == LEFT ? Colours::black : Colours::green);
                            g.drawText(text, rect, Justification::centred, false);
                            break;
                        }
                    }
                }
                //=====================================================================================
                while (history_stat->get_next_value(value));
                if (offset < 2) return;

                path.scaleToFit(
                    plot.getX() + (plot.getWidth() - offset),
                    y_start,
                    offset,
                    plot.getHeight() - theme::graph_indent * 2,
                    false
                );
                g.setColour(channel == LEFT ? Colours::black : Colours::green);
                g.strokePath(path, PathStrokeType(1.0f));

            }
        }
        //=====================================================================================

        /*const size_t line_width = 30;
        Rectangle<int> rect
        (
            plot.getX(),
            static_cast<int>(y_end),
            plot.getWidth() - theme::margin,
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
        g.drawRect(plot);*/
    }

private:
    unique_ptr<circle_> history_stat;

};