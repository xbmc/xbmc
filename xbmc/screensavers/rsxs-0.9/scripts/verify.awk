BEGIN {
	if (skip !~ /cyclone/)
		verify["rs-cyclone"] = 1
	
	if (skip !~ /euphoria/) {
		verify["\\(regular\\)\" rs-euphoria"] = 1
		verify["\\(grid\\)\" rs-euphoria"] = 1
		verify["\\(cubism\\)\" rs-euphoria"] = 1
		verify["\\(bad math\\)\" rs-euphoria"] = 1
		verify["\\(M-theory\\)\" rs-euphoria"] = 1
		verify["\\(UHF TEM\\)\" rs-euphoria"] = 1
		verify["\\(nowhere\\)\" rs-euphoria"] = 1
		verify["\\(echo\\)\" rs-euphoria"] = 1
		verify["\\(kaleidoscope\\)\" rs-euphoria"] = 1
	}
	
	if (skip !~ /fieldlines/)
		verify["rs-fieldlines"] = 1
	
	if (skip !~ /flocks/)
		verify["rs-flocks"] = 1
	
	if (skip !~ /flux/) {
		verify["\\(regular\\)\" rs-flux"] = 1
		verify["\\(hypnotic\\)\" rs-flux"] = 1
		verify["\\(insane\\)\" rs-flux"] = 1
		verify["\\(sparklers\\)\" rs-flux"] = 1
		verify["\\(paradigm\\)\" rs-flux"] = 1
		verify["\\(fusion\\)\" rs-flux"] = 1
	}
	
	if (skip !~ /helios/)
		verify["rs-helios"] = 1
	
	if (skip !~ /hyperspace/)
		verify["rs-helios"] = 1
	
	if (skip !~ /lattice/) {
		verify["\\(regular\\)\" rs-lattice"] = 1
		verify["\\(chainmail\\)\" rs-lattice"] = 1
		verify["\\(brass mesh\\)\" rs-lattice"] = 1
		verify["\\(computer\\)\" rs-lattice"] = 1
		verify["\\(slick\\)\" rs-lattice"] = 1
		verify["\\(tasty\\)\" rs-lattice"] = 1
	}
	
	if (skip !~ /plasma/)
		verify["rs-plasma"] = 1
	
	if (skip !~ /skyrocket/)
		verify["rs-skyrocket"] = 1
	
	if (skip !~ /solarwinds/) {
		verify["\\(regular\\)\" rs-solarwinds"] = 1
		verify["\\(cosmic strings\\)\" rs-solarwinds"] = 1
		verify["\\(cold pricklies\\)\" rs-solarwinds"] = 1
		verify["\\(space fur\\)\" rs-solarwinds"] = 1
		verify["\\(jiggly\\)\" rs-solarwinds"] = 1
		verify["\\(undertow\\)\" rs-solarwinds"] = 1
	}
}

/^[ \t]*\*programs:/ {
	output = ""
	do {
		getline
		doRemove = ""
		for (name in verify)
			if ($0 ~ name)
				doVerify = name
		if (doVerify)
			delete verify[doVerify]
		output = output $0 ORS
	} while ($0 ~ /\\$/)
	next
}

END {
	ORS="\n"
	count = 0
	for (name in verify) {
		++count
		print name
	}
	exit count
}
