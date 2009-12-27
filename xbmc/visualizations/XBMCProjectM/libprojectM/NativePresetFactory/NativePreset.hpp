/*
 * Preset.hpp
 *
 *  Created on: Aug 5, 2008
 *      Author: carm
 */

#ifndef __NATIVE_PRESET_HPP_
#define __NATIVE_PRESET_HPP_

#include <string>

#include "BeatDetect.hpp"
#include "Pipeline.hpp"
#include "PipelineContext.hpp"
#include "Preset.hpp"

/// A templated preset class to build different various hard coded presets and 
/// compile them into object files to be loaded into a playlist
template <class PipelineT>
class NativePreset : public Preset {
public:

	inline NativePreset(const std::string & name=std::string(),
		const std::string & author = std::string()) : Preset(name, author) {}

	virtual ~NativePreset() {}

	inline PipelineT & pipeline() { return _pipeline; }
	inline virtual void Render(const BeatDetect &music, const PipelineContext &context) {
		_pipeline.Render(music, context);
	}

private:
	PipelineT _pipeline;
};

#endif 
