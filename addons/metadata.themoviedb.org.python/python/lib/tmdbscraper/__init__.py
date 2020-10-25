
def get_imdb_id(uniqueids):
    imdb_id = uniqueids.get('imdb')
    if not imdb_id or not imdb_id.startswith('tt'):
        return None
    return imdb_id

# example format for scraper results
_ScraperResults = {
    'info',
    'ratings',
    'uniqueids',
    'cast',
    'available_art',
    'error',
    'warning' # not handled
}
