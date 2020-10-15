# -*- coding: utf-8 -*-

def fanarttv_albumart(data):
    if 'albums' in data:
        albumdata = {}
        thumbs = []
        extras = []
        discs = {}
        for mbid, art in data['albums'].items():
            if 'albumcover' in art:
                for thumb in art['albumcover']:
                    thumbdata = {}
                    thumbdata['image'] = thumb['url']
                    thumbdata['preview'] = thumb['url'].replace('/fanart/', '/preview/')
                    thumbdata['aspect'] = 'thumb'
                    thumbs.append(thumbdata)
            if 'cdart' in art:
                for cdart in art['cdart']:
                    extradata = {}
                    extradata['image'] = cdart['url']
                    extradata['preview'] = cdart['url'].replace('/fanart/', '/preview/')
                    extradata['aspect'] = 'discart'
                    extras.append(extradata)
                    # support for multi-disc albums
                    multidata = {}
                    num = cdart['disc']
                    multidata['image'] = cdart['url']
                    multidata['preview'] = cdart['url'].replace('/fanart/', '/preview/')
                    multidata['aspect'] = 'discart%s' % num
                    if not num in discs:
                        discs[num] = [multidata]
                    else:
                        discs[num].append(multidata)
        if thumbs:
            albumdata['thumb'] = thumbs
        # only return for multi-discs, not single discs
        if len(discs) > 1:
            for k, v in discs.items():
                for item in v:
                    extras.append(item)
        if extras:
            albumdata['extras'] = extras
        return albumdata
