
/// @idea Weighted random based on user stats

#ifndef PRESET_CHOOSER_HPP
#define PRESET_CHOOSER_HPP

#include "Preset.hpp"

#include "PresetLoader.hpp"
#include "RandomNumberGenerators.hpp"
#include <cassert>
#include <memory>
#include <iostream>
class PresetChooser;

///  A simple iterator class to traverse back and forth a preset directory
class PresetIterator {

public:
    PresetIterator()  {}

    /// Instantiate a preset iterator at the given starting position 
    PresetIterator(std::size_t start);

    /// Move iterator forward
    void operator++();

    /// Move iterator backword
    void operator--() ;
 
    /// Not equal comparator
    bool operator !=(const PresetIterator & presetPos) const ;

    /// Equality comparator
    bool operator ==(const PresetIterator & presetPos) const ;

    /// Returns an integer value representing the iterator position
    /// @bug might become internal
    /// \brief Returns the indexing value used by the current iterator.
    std::size_t operator*() const;

    ///  Allocate a new preset given this iterator's associated preset name
    /// \param presetInputs the preset inputs to associate with the preset upon construction
    /// \param presetOutputs the preset outputs to associate with the preset upon construction
    /// \returns an autopointer of the newly allocated preset
    std::unique_ptr<Preset> allocate( PresetInputs & presetInputs, PresetOutputs & presetOutputs);

    ///  Set the chooser asocciated with this iterator
    void setChooser(const PresetChooser & chooser);

private:
    std::size_t m_currentIndex;
    const PresetChooser * m_presetChooser;

};

/// Provides functions and iterators to select presets. Requires a preset loader upon construction
class PresetChooser {

public:
    typedef PresetIterator iterator;

    /// Initializes a chooser with an established preset loader.
    /// \param presetLoader an initialized preset loader to choose presets from
    /// \note The preset loader is refreshed via events or otherwise outside this class's scope
    PresetChooser(const PresetLoader & presetLoader);

    /// Choose a preset via the passed in index. Must be between 0 and num valid presets in directory
    /// \param index An index lying in the interval [0, this->getNumPresets())
    /// \param presetInputs the preset inputs to associate with the preset upon construction
    /// \param presetOutputs the preset outputs to associate with the preset upon construction
    /// \returns an auto pointer of the newly allocated preset
    std::unique_ptr<Preset> directoryIndex(std::size_t index, PresetInputs & presetInputs,
                                         PresetOutputs & presetOutputs) const;

    /// Gets the number of presets last believed to exist in the preset loader's filename collection
    /// \returns the number of presets in the collection
    std::size_t getNumPresets() const;

   
    /// An STL-esque iterator to begin traversing presets from a directory
    /// \param index the index to begin iterating at. Assumed valid between [0, num presets)
    /// \returns the position of the first preset in the collection
    PresetIterator begin(unsigned int index) const;

    /// An STL-esque iterator to begin traversing presets from a directory
    /// \returns the position of the first preset in the collection
    PresetIterator begin();

    /// An STL-esque iterator to retrieve an end position from a directory
    /// \returns the end position of the collection
    PresetIterator end() const;

    /// Perform a weighted sample to select a preset (uses preset rating values)
    /// \returns an iterator to the randomly selected preset
    iterator weightedRandom() const;

    /// True if no presets in directory 
    bool empty() const;

    
    inline void nextPreset(PresetIterator & presetPos);

private:

    const PresetLoader * m_presetLoader;
};


inline PresetChooser::PresetChooser(const PresetLoader & presetLoader):m_presetLoader(&presetLoader) {}

inline std::size_t PresetChooser::getNumPresets() const {
    return m_presetLoader->getNumPresets();
}

inline void PresetIterator::setChooser(const PresetChooser & chooser) {
    m_presetChooser = &chooser;
}

inline std::size_t PresetIterator::operator*() const {
    return m_currentIndex;
}

inline PresetIterator::PresetIterator(std::size_t start):m_currentIndex(start) {}

inline void PresetIterator::operator++() {
    assert(m_currentIndex < m_presetChooser->getNumPresets());
    m_currentIndex++;
}

inline void PresetIterator::operator--() {
    assert(m_currentIndex > 0);
    m_currentIndex--;
}

inline bool PresetIterator::operator !=(const PresetIterator & presetPos) const {
    return (*presetPos != **this);
}


inline bool PresetIterator::operator ==(const PresetIterator & presetPos) const {
    return (*presetPos == **this);
}

inline std::unique_ptr<Preset> PresetIterator::allocate( PresetInputs & presetInputs, PresetOutputs & presetOutputs) {
    return m_presetChooser->directoryIndex(m_currentIndex, presetInputs, presetOutputs);
}

inline void PresetChooser::nextPreset(PresetIterator & presetPos) {

		if (this->empty()) {
			return;
		}
		
		// Case: idle preset currently running, selected first preset of chooser
		else if (presetPos == this->end()) 
			presetPos = this->begin();		 
		else
			++(presetPos);

		// Case: already at last preset, loop to beginning
		if (((presetPos) == this->end())) {
			presetPos = this->begin();
		}
		
}

inline PresetIterator PresetChooser::begin() {
    PresetIterator pos(0);
    pos.setChooser(*this);
    return pos;
}

inline PresetIterator PresetChooser::begin(unsigned int index) const{
    PresetIterator pos(index);
    pos.setChooser(*this);
    return pos;
}

inline PresetIterator PresetChooser::end() const {
    PresetIterator pos(m_presetLoader->getNumPresets());
    pos.setChooser(*this);
    return pos;
}


inline bool PresetChooser::empty() const {
	return m_presetLoader->getNumPresets() == 0;
}

inline std::unique_ptr<Preset> PresetChooser::directoryIndex(std::size_t index, PresetInputs & presetInputs,
                                         PresetOutputs & presetOutputs) const {

	return m_presetLoader->loadPreset(index, presetInputs, presetOutputs);
}


inline PresetChooser::iterator PresetChooser::weightedRandom() const {
	std::size_t index = RandomNumberGenerators::weightedRandom
		(m_presetLoader->getPresetRatings(), m_presetLoader->getPresetRatingsSum());
	return begin(index);
}

#endif
