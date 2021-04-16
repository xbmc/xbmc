def configure_scraped_details(details, settings):
    details = _configure_rating_prefix(details, settings)
    details = _configure_keeporiginaltitle(details, settings)
    details = _configure_trailer(details, settings)
    details = _configure_multiple_studios(details, settings)
    details = _configure_default_rating(details, settings)
    details = _configure_tags(details, settings)
    return details

def configure_tmdb_artwork(details, settings):
    if 'available_art' not in details:
        return details

    art = details['available_art']
    fanart_enabled = settings.getSettingBool('fanart')
    if not fanart_enabled:
        if 'fanart' in art:
            del art['fanart']
        if 'set.fanart' in art:
            del art['set.fanart']
    if not settings.getSettingBool('landscape'):
        if 'landscape' in art:
            if fanart_enabled:
                art['fanart'] = art.get('fanart', []) + art['landscape']
            del art['landscape']
        if 'set.landscape' in art:
            if fanart_enabled:
                art['set.fanart'] = art.get('set.fanart', []) + art['set.landscape']
            del art['set.landscape']

    return details

def is_fanarttv_configured(settings):
    return settings.getSettingBool('enable_fanarttv_artwork')

def _configure_rating_prefix(details, settings):
    if details['info'].get('mpaa'):
        details['info']['mpaa'] = settings.getSettingString('certprefix') + details['info']['mpaa']
    return details

def _configure_keeporiginaltitle(details, settings):
    if settings.getSettingBool('keeporiginaltitle'):
        details['info']['title'] = details['info']['originaltitle']
    return details

def _configure_trailer(details, settings):
    if details['info'].get('trailer') and not settings.getSettingBool('trailer'):
        del details['info']['trailer']
    return details

def _configure_multiple_studios(details, settings):
    if not settings.getSettingBool('multiple_studios'):
        details['info']['studio'] = details['info']['studio'][:1]
    return details

def _configure_default_rating(details, settings):
    imdb_default = bool(details['ratings'].get('imdb')) and settings.getSettingString('RatingS') == 'IMDb'
    trakt_default = bool(details['ratings'].get('trakt')) and settings.getSettingString('RatingS') == 'Trakt'
    default_rating = 'themoviedb'
    if imdb_default:
        default_rating = 'imdb'
    elif trakt_default:
        default_rating = 'trakt'
    if default_rating not in details['ratings']:
        default_rating = list(details['ratings'].keys())[0] if details['ratings'] else None
    for rating_type in details['ratings'].keys():
        details['ratings'][rating_type]['default'] = rating_type == default_rating
    return details

def _configure_tags(details, settings):
    if not settings.getSettingBool('add_tags'):
        del details['info']['tag']
    return details

# pylint: disable=invalid-name
try:
    basestring
except NameError: # py2 / py3
    basestring = str

#pylint: disable=redefined-builtin
class PathSpecificSettings(object):
    # read-only shim for typed `xbmcaddon.Addon().getSetting*` methods
    def __init__(self, settings_dict, log_fn):
        self.data = settings_dict
        self.log = log_fn

    def getSettingBool(self, id):
        return self._inner_get_setting(id, bool, False)

    def getSettingInt(self, id):
        return self._inner_get_setting(id, int, 0)

    def getSettingNumber(self, id):
        return self._inner_get_setting(id, float, 0.0)

    def getSettingString(self, id):
        return self._inner_get_setting(id, basestring, '')

    def _inner_get_setting(self, setting_id, setting_type, default):
        value = self.data.get(setting_id)
        if isinstance(value, setting_type):
            return value
        self._log_bad_value(value, setting_id)
        return default

    def _log_bad_value(self, value, setting_id):
        if value is None:
            self.log("requested setting ({0}) was not found.".format(setting_id))
        else:
            self.log('failed to load value "{0}" for setting {1}'.format(value, setting_id))
