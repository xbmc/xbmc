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

class Preset;
class PresetInputs;
class PresetOutputs;

class PresetLoader {
	public:
		static const std::string PROJECTM_FILE_EXTENSION;
		static const std::string MILKDROP_FILE_EXTENSION;
		
		/** Initializes the preset loader with the target directory specified */
		PresetLoader(std::string dirname = std::string());
		
		/** Destructor will remove all alllocated presets */
		~PresetLoader();
	
		/** Load a preset by specifying a filename of the directory (that is, NOT full path) */
		/** Autopointers: when you take it, I leave it */		
		std::unique_ptr<Preset> loadPreset(unsigned int index, PresetInputs & presetInputs, 
			PresetOutputs & presetOutputs) const;
		
		/// Add a preset to the loader's collection.
		/// \param url an url referencing the preset
		/// \param presetName a name for the preset
		/// \param rating an integer representing the goodness of the preset
		/// \returns The unique index assigned to the preset in the collection. Used with loadPreset
		unsigned int addPresetURL ( const std::string & url, const std::string & presetName, int rating);
	
		
		/// Add a preset to the loader's collection.
		/// \param index insertion index
		/// \param url an url referencing the preset
		/// \param presetName a name for the preset
		/// \param rating an integer representing the goodness of the preset
		void insertPresetURL (unsigned int index, const std::string & url, const std::string & presetName, int rating);
	
		/// Clears all presets from the collection
		inline void clear() { m_entries.clear(); m_presetNames.clear(); m_ratings.clear(); m_ratingsSum = 0; }
		
		const std::vector<int> & getPresetRatings() const;
		int getPresetRatingsSum() const;
		
		void removePreset(unsigned int index);

		void setRating(unsigned int index, int rating);
		
		/// Get a preset rating given an index
		 int getPresetRating ( unsigned int index) const;
		
		/// Get a preset url given an index
		const std::string & getPresetURL ( unsigned int index) const;
		
		/// Get a preset name given an index
		const std::string & getPresetName ( unsigned int index) const;
		
		/** Returns the number of presets in the active directory */
		inline std::size_t getNumPresets() const {
			return m_entries.size();
		}
					
		/** Sets the directory where the loader will search for files */	
		void setScanDirectory(std::string pathname);

		/// Returns the directory path associated with this preset chooser
		inline const std::string & directoryName() const {
			return m_dirname;
		}

		/** Rescans the active preset directory */
		void rescan();

	private:
		void handleDirectoryError();
		std::string m_dirname;
		DIR * m_dir;

		int m_ratingsSum;
		
		// vector chosen for speed, but not great for reverse index lookups
		std::vector<std::string> m_entries;
		std::vector<std::string> m_presetNames;
		std::vector<int> m_ratings;

};

#endif
