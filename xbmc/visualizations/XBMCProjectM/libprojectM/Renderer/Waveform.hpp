/*
 * Waveform.hpp
 *
 *  Created on: Jun 25, 2008
 *      Author: pete
 */

#ifndef WAVEFORM_HPP_
#define WAVEFORM_HPP_

#include "Renderable.hpp"
#include <vector>

class ColoredPoint
{
public:
	float x;
	float y;
	float r;
	float g;
	float b;
	float a;

	ColoredPoint():x(0.5),y(0.5),r(1),g(1),b(1),a(1){};
};

class WaveformContext
{
public:
	float sample;
	int samples;
	int sample_int;
	float left;
	float right;
	BeatDetect *music;

	WaveformContext(int samples, BeatDetect *music):samples(samples),music(music){};
};


class Waveform : public RenderItem
{
public:

    int samples; /* number of samples associated with this wave form. Usually powers of 2 */
    bool spectrum; /* spectrum data or pcm data */
    bool dots; /* draw wave as dots or lines */
    bool thick; /* draw thicker lines */
    bool additive; /* add color values together */

    float scaling; /* scale factor of waveform */
    float smoothing; /* smooth factor of waveform */
    int sep;  /* no idea what this is yet... */

    Waveform(int samples);
    void Draw(RenderContext &context);

private:
	virtual ColoredPoint PerPoint(ColoredPoint p, const WaveformContext context)=0;
	std::vector<ColoredPoint> points;
	std::vector<float> pointContext;

};
#endif /* WAVEFORM_HPP_ */
