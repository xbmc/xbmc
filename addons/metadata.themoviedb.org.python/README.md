## The Movie Database Python scraper for Kodi

This is early work on a Python movie scraper for Kodi.

### Manual search by IMDB / TMDB ID
When manually searching you can enter an IMDB or TMDB ID to pull up an exact movie result.
To search by TMDB enter "tmdb/" then the ID, like "tmdb/11". To search by IMDB ID enter it directly.

## Development info

### How to run unit tests

`python3 -m unittest discover -v` from the main **metadata.themoviedb.org.python** directory.

Set env variable `TEST_E2E` to enable the single IMDB end-to-end test, `TEST_E2E=true python3 -m unittest discover -v`.
Not for a pipeline, but may be helpful to run now and then.
