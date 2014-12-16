BEGIN {
	if (skip !~ /cyclone/)
		remove["rs-cyclone"] = 1
	
	if (skip !~ /euphoria/) {
		remove["\\(regular\\)\" rs-euphoria"] = 1
		remove["\\(grid\\)\" rs-euphoria"] = 1
		remove["\\(cubism\\)\" rs-euphoria"] = 1
		remove["\\(bad math\\)\" rs-euphoria"] = 1
		remove["\\(M-theory\\)\" rs-euphoria"] = 1
		remove["\\(UHF TEM\\)\" rs-euphoria"] = 1
		remove["\\(nowhere\\)\" rs-euphoria"] = 1
		remove["\\(echo\\)\" rs-euphoria"] = 1
		remove["\\(kaleidoscope\\)\" rs-euphoria"] = 1
	}
	
	if (skip !~ /fieldlines/)
		remove["rs-fieldlines"] = 1
	
	if (skip !~ /flocks/)
		remove["rs-flocks"] = 1
	
	if (skip !~ /flux/) {
		remove["\\(regular\\)\" rs-flux"] = 1
		remove["\\(hypnotic\\)\" rs-flux"] = 1
		remove["\\(insane\\)\" rs-flux"] = 1
		remove["\\(sparklers\\)\" rs-flux"] = 1
		remove["\\(paradigm\\)\" rs-flux"] = 1
		remove["\\(fusion\\)\" rs-flux"] = 1
	}
	
	if (skip !~ /helios/)
		remove["rs-helios"] = 1
	
	if (skip !~ /hyperspace/)
		remove["rs-hyperspace"] = 1
	
	if (skip !~ /lattice/) {
		remove["\\(regular\\)\" rs-lattice"] = 1
		remove["\\(chainmail\\)\" rs-lattice"] = 1
		remove["\\(brass mesh\\)\" rs-lattice"] = 1
		remove["\\(computer\\)\" rs-lattice"] = 1
		remove["\\(slick\\)\" rs-lattice"] = 1
		remove["\\(tasty\\)\" rs-lattice"] = 1
	}
	
	if (skip !~ /plasma/)
		remove["rs-plasma"] = 1
	
	if (skip !~ /skyrocket/)
		remove["rs-skyrocket"] = 1
	
	if (skip !~ /solarwinds/) {
		remove["\\(regular\\)\" rs-solarwinds"] = 1
		remove["\\(cosmic strings\\)\" rs-solarwinds"] = 1
		remove["\\(cold pricklies\\)\" rs-solarwinds"] = 1
		remove["\\(space fur\\)\" rs-solarwinds"] = 1
		remove["\\(jiggly\\)\" rs-solarwinds"] = 1
		remove["\\(undertow\\)\" rs-solarwinds"] = 1
	}
}

/^[ \t]*\*programs:/ {
	print
	output = ""
	do {
		getline
		doRemove = ""
		for (name in remove)
			if ($0 ~ name)
				doRemove = name
		if (doRemove)
			delete remove[doRemove]
		else
			output = output $0 ORS
	} while ($0 ~ /\\$/)
	ORS=""
	print output
	next
}

! /^[ \t]*\*programs:/ {
	ORS="\n"
	print
}
