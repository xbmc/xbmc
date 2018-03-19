def get_timezones():

	''' Returns a dictionary or regions, which hold lists of countries within those regions. '''

	with open('/usr/share/zoneinfo/zone.tab', 'r') as f:

		lines = f.readlines()

		lines = [line for line in lines if line and not line.startswith('#') and '/' in line]

	tmp = []
	timezones = {'UTC': ['UTC']}

	for line in lines:

		columns = line.split('\t')

		try:
			tz_raw = columns[2].replace('\n','')
		except:
			continue

		tmp.append(tz_raw)

	tmp.sort()

	for tz_raw in tmp:

		tz_region, tz_country = tz_raw[:tz_raw.index('/')], tz_raw[tz_raw.index('/')+1:]

		t = timezones.get(tz_region, [])

		t.append(tz_country)

		timezones[tz_region] = t

	return timezones

# tz = get_timezones()

# import pprint

# pprint.pprint(tz)

# z = tz.keys()
# z.sort()

# for x in z:

# 	print x, '   ', len(tz[x])
