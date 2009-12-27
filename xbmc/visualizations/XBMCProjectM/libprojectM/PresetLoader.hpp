#ifndef __PRESET_LOADER_HPP
#define __PRESET_LOADER_HPP

#include <string> // used for path / filename stuff
#include <memory> // for auto pointers
#include <sys/types.h>

#ifdef WIN32
#include "win32-dirent.h"
#endif

#ifdef LINUX
#include <dirent.h>
#endif

#ifdef MACOS
#include <dirent.h>
#endif

#include <vector>
#include <map>
#include "PresetFactoryManager.hpp"

class Preset;
class PresetFactory;


class PresetLoader {
	public:
		
		
		/// Initializes the preset loader with the target directory specified 
		PresetLoader(int gx, int gy, std::string dirname);
				
		~PresetLoader();
	
		/// Load a preset by specifying it's unique identifier given when the preset url
		/// was added to this loader	
		std::auto_ptr<Preset> loadPreset(unsigned int index) const;
		std::auto_ptr<Preset> loadPreset ( const std::string & url )  const;
		/// Add a preset to the loader's collection.
		/// \param url an url referencing the preset
		/// \param presetName a name for the preset
		/// \param rating an integer representing the goodness of the preset
		/// \returns The unique index assigned to the preset in the collection. Used with loadPreset
		unsigned int addPresetURL ( const std::string & url, const std::string & presetName, const RatingList & ratings);
			
		/// Add a preset to the loader's collection.
		/// \param index insertion index
		/// \param url an url referencing the preset
		/// \param presetName a name for the preset
		/// \param rating an integer representing the goodness of the preset
		void insertPresetURL (unsigned int index, const std::string & url, const std::string & presetName, const RatingList & ratings);
	
		/// Clears all presets from the collection
		inline void clear() { 
			_entries.clear(); _presetNames.clear(); 
			_ratings = std::vector<RatingList>(TOTAL_RATING_TYPES, RatingList());
			clearRatingsSum();
 		}

		inline void clearRatingsSum() {
			_ratingsSums = std::vector<int>(TOTAL_RATING_TYPES, 0);
		}
		
		const std::vector<RatingList> & getPresetRatings() const;		
		const std::vector<int> & getPresetRatingsSums() const;

		/// Removes a preset from the loader
		/// \param index the unique identifier of the preset url to be removed
		void removePreset(unsigned int index);

		/// Sets the rating of a preset to a new value
		void setRating(unsigned int index, int rating, const PresetRatingType ratingType);
		
		/// Get a preset rating given an index
		int getPresetRating ( unsigned int index, const PresetRatingType ratingType) const;
		
		/// Get a preset url given an index
		const std::string & getPresetURL ( unsigned int index) const;
		
		/// Get a preset name given an index
		const std::string & getPresetName ( unsigned int index) const;
		
		/// Returns the number of presets in the active directory 
		inline std::size_t size() const {
			return _entries.size();
		}
					
		/// Sets the directory where the loader will search for files 
		void setScanDirectory(std::string pathname);

		/// Returns the directory path associated with this preset chooser
		inline const std::string & directoryName() const {
			return _dirname;
		}
		
		/// Rescans the active preset directory
		void rescan();
		void setPresetName(unsigned int index, std::string name);
	private:
		void handleDirectoryError();
		std::string _dirname;
		DIR * _dir;
		std::vector<int> _ratingsSums;
		mutable PresetFactoryManager _presetFactoryManager;

		// vector chosen for speed, but not great for reverse index lookups
		std::vector<std::string> _entries;
		std::vector<std::string> _presetNames;

		// Indexed by ratingType, preset position.
		std::vector<RatingList> _ratings;
		

};

#endif
