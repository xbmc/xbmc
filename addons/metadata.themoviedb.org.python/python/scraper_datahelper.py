import re
try:
    from urlparse import parse_qsl
except ImportError: # py2 / py3
    from urllib.parse import parse_qsl

# get addon params from the plugin path querystring
def get_params(argv):
    result = {'handle': int(argv[0])}
    if len(argv) < 2 or not argv[1]:
        return result

    result.update(parse_qsl(argv[1].lstrip('?')))
    return result

def combine_scraped_details_info_and_ratings(original_details, additional_details):
    def update_or_set(details, key, value):
        if key in details:
            details[key].update(value)
        else:
            details[key] = value

    if additional_details:
        if additional_details.get('info'):
            update_or_set(original_details, 'info', additional_details['info'])
        if additional_details.get('ratings'):
            update_or_set(original_details, 'ratings', additional_details['ratings'])
    return original_details

def combine_scraped_details_available_artwork(original_details, additional_details, language, settings):
    if language:
        # Image languages don't have regional variants
        language = language.split('-')[0]
    if additional_details and additional_details.get('available_art'):
        available_art = additional_details['available_art']
        if not original_details.get('available_art'):
            original_details['available_art'] = {}
        for arttype, artlist in available_art.items():
            artlist = sorted(artlist, key=lambda x:x['lang']==language, reverse=True)
            combinlist = artlist + original_details['available_art'].get(arttype, [])
            original_details['available_art'][arttype] = combinlist

            if not settings.getSettingBool('prioritize_fanarttv_artwork'):
                original_details['available_art'][arttype] = sorted(combinlist, key=lambda x:x['lang']==language, reverse=True)

    return original_details

def find_uniqueids_in_text(input_text):
    result = {}
    res = re.search(r'(themoviedb.org/movie/)([0-9]+)', input_text)
    if (res):
        result['tmdb'] = res.group(2)
    res = re.search(r'imdb....?/title/tt([0-9]+)', input_text)
    if (res):
        result['imdb'] = 'tt' + res.group(1)
    else:
        res = re.search(r'imdb....?/Title\?t{0,2}([0-9]+)', input_text)
        if (res):
            result['imdb'] = 'tt' + res.group(1)
    return result
