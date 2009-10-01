BEGIN {
	if (skip !~ /cyclone/)
		add["rs-cyclone"] =                           "           GL:  \"Really Slick Cyclones\" rs-cyclone -root                    \\n\\"
	
	if (skip !~ /euphoria/) {
		add["\\(regular\\)\" rs-euphoria"] =          "           GL:  \"Really Slick Euphoria (regular)\" rs-euphoria -stringy -root \\n\\"
		add["\\(grid\\)\" rs-euphoria"] =             "           GL:  \"Really Slick Euphoria (grid)\" rs-euphoria             -wisps 4  -background 1 -density 25 -visibility 70 -speed 15 -wireframe -root \\n\\"
		add["\\(cubism\\)\" rs-euphoria"] =           "           GL:  \"Really Slick Euphoria (cubism)\" rs-euphoria           -wisps 15 -background 0 -density 4  -visibility 15 -speed 10 -root \\n\\"
		add["\\(bad math\\)\" rs-euphoria"] =         "           GL:  \"Really Slick Euphoria (bad math)\" rs-euphoria         -wisps 2  -background 2 -density 20 -visibility 35 -speed 30 -feedback 40 -feedbacksize 5 -feedbacksize 8 -wireframe -lines -root \\n\\"
		add["\\(M-theory\\)\" rs-euphoria"] =         "           GL:  \"Really Slick Euphoria (M-theory)\" rs-euphoria         -wisps 3  -background 0 -density 25 -visibility 35 -speed 20 -feedback 40 -feedbackspeed 20 -feedbacksize 8 -root \\n\\"
		add["\\(UHF TEM\\)\" rs-euphoria"] =          "           GL:  \"Really Slick Euphoria (UHF TEM)\" rs-euphoria          -wisps 0  -background 3 -density 35 -visibility 5  -speed 50 -root \\n\\"
		add["\\(nowhere\\)\" rs-euphoria"] =          "           GL:  \"Really Slick Euphoria (nowhere)\" rs-euphoria          -wisps 0  -background 3 -density 30 -visibility 40 -speed 20 -feedback 80 -feedbackspeed 10 -feedbacksize 8 -wireframe -random -root \\n\\"
		add["\\(echo\\)\" rs-euphoria"] =             "           GL:  \"Really Slick Euphoria (echo)\" rs-euphoria             -wisps 3  -background 0 -density 25 -visibility 30 -speed 20 -feedback 85 -feedbackspeed 30 -feedbacksize 8 -plasma -root \\n\\"
		add["\\(kaleidoscope\\)\" rs-euphoria"] =     "           GL:  \"Really Slick Euphoria (kaleidoscope)\" rs-euphoria     -wisps 3  -background 0 -density 25 -visibility 40 -speed 15 -feedback 90 -feedbackspeed 3  -feedbacksize 8 -root \\n\\"
	}
	
	if (skip !~ /fieldlines/)
		add["rs-fieldlines"] =                        "           GL:  \"Really Slick Fieldlines\" rs-fieldlines -root               \\n\\"
	
	if (skip !~ /flocks/)
		add["rs-flocks"] =                            "           GL:  \"Really Slick Flocks\" rs-flocks -root                       \\n\\"
	
	if (skip !~ /flux/) {
		add["\\(regular\\)\" rs-flux"] =              "           GL:  \"Really Slick Flux (regular)\" rs-flux -root                 \\n\\"
		add["\\(hypnotic\\)\" rs-flux"] =             "           GL:  \"Really Slick Flux (hypnotic)\" rs-flux                 -fluxes 2  -particles 10 -length 40 -lights  -size 15  -randomness 80 -speed 20 -rotation 0  -wind 40 -instability 10  -blur 30 -root \\n\\"
		add["\\(insane\\)\" rs-flux"] =               "           GL:  \"Really Slick Flux (insane)\" rs-flux                   -fluxes 4  -particles 30 -length 8  -lights  -size 25  -randomness 0  -speed 80 -rotation 60 -wind 40 -instability 100 -blur 10 -root \\n\\"
		add["\\(sparklers\\)\" rs-flux"] =            "           GL:  \"Really Slick Flux (sparklers)\" rs-flux                -fluxes 3  -particles 20 -length 6  -spheres -size 20  -randomness 85 -speed 60 -rotation 30 -wind 20 -instability 30  -blur 0  -root \\n\\"
		add["\\(paradigm\\)\" rs-flux"] =             "           GL:  \"Really Slick Flux (paradigm)\" rs-flux                 -fluxes 1  -particles 40 -length 40 -lights  -size 5   -randomness 90 -speed 30 -rotation 20 -wind 10 -instability 5   -blur 10 -root \\n\\"
		add["\\(fusion\\)\" rs-flux"] =               "           GL:  \"Really Slick Flux (fusion)\" rs-flux                   -fluxes 10 -particles 3  -length 10 -lights  -size 100 -randomness 0  -speed 50 -rotation 30 -wind 40 -instability 35  -blur 50 -root \\n\\"
	}
	
	if (skip !~ /helios/)
		add["rs-helios"] =                            "           GL:  \"Really Slick Helios\" rs-helios -root                       \\n\\"
	
	if (skip !~ /hyperspace/)
		add["rs-hyperspace"] =                        "           GL:  \"Really Slick Hyperspace\" rs-hyperspace -root               \\n\\"
	
	if (skip !~ /lattice/) {
		add["\\(regular\\)\" rs-lattice"] =           "           GL:  \"Really Slick Lattice (regular)\" rs-lattice -root           \\n\\"
		add["\\(chainmail\\)\" rs-lattice"] =         "           GL:  \"Really Slick Lattice (chainmail)\" rs-lattice          -longitude 24 -latitude 12 -thickness 50  -density 80 -depth 3 -chrome   -smooth -root \\n\\"
		add["\\(brass mesh\\)\" rs-lattice"] =        "           GL:  \"Really Slick Lattice (brass mesh)\" rs-lattice         -longitude 4  -latitude 4  -thickness 40  -density 50 -depth 4 -brass    -no-smooth -root \\n\\"
		add["\\(computer\\)\" rs-lattice"] =          "           GL:  \"Really Slick Lattice (computer)\" rs-lattice           -longitude 4  -latitude 6  -thickness 70  -density 90 -depth 4 -circuits -no-smooth -root \\n\\"
		add["\\(slick\\)\" rs-lattice"] =             "           GL:  \"Really Slick Lattice (slick)\" rs-lattice              -longitude 24 -latitude 12 -thickness 100 -density 30 -depth 4 -shiny    -smooth -root \\n\\"
		add["\\(tasty\\)\" rs-lattice"] =             "           GL:  \"Really Slick Lattice (tasty)\" rs-lattice              -longitude 24 -latitude 12 -thickness 100 -density 25 -depth 4 -donuts   -smooth -root \\n\\"
	}
	
	if (skip !~ /plasma/)
		add["rs-plasma"] =                            "           GL:  \"Really Slick Plasma\" rs-plasma -root                       \\n\\"
	
	if (skip !~ /skyrocket/)
		add["rs-skyrocket"] =                         "           GL:  \"Really Slick Skyrocket\" rs-skyrocket -volume 0 -root                 \\n\\"
	
	if (skip !~ /solarwinds/) {
		add["\\(regular\\)\" rs-solarwinds"] =        "           GL:  \"Really Slick Solar Winds (regular)\" rs-solarwinds -root    \\n\\"
		add["\\(cosmic strings\\)\" rs-solarwinds"] = "           GL:  \"Really Slick Solar Winds (cosmic strings)\" rs-solarwinds -winds 1 -emitters 50  -particles 3000 -lines  -size 20 -windspeed 10  -emitterspeed 10  -speed 10  -blur 10 -root \\n\\"
		add["\\(cold pricklies\\)\" rs-solarwinds"] = "           GL:  \"Really Slick Solar Winds (cold pricklies)\" rs-solarwinds -winds 1 -emitters 300 -particles 3000 -lines  -size 5  -windspeed 20  -emitterspeed 100 -speed 15  -blur 70 -root \\n\\"
		add["\\(space fur\\)\" rs-solarwinds"] =      "           GL:  \"Really Slick Solar Winds (space fur)\" rs-solarwinds      -winds 2 -emitters 400 -particles 1600 -lines  -size 15 -windspeed 20  -emitterspeed 15  -speed 10  -blur 0  -root \\n\\"
		add["\\(jiggly\\)\" rs-solarwinds"] =         "           GL:  \"Really Slick Solar Winds (jiggly)\" rs-solarwinds         -winds 1 -emitters 40  -particles 1200 -points -size 20 -windspeed 100 -emitterspeed 20  -speed 4   -blur 50 -root \\n\\"
		add["\\(undertow\\)\" rs-solarwinds"] =       "           GL:  \"Really Slick Solar Winds (undertow)\" rs-solarwinds       -winds 1 -emitters 400 -particles 1200 -lights -size 40 -windspeed 20  -emitterspeed 1   -speed 100 -blur 50 -root \\n\\"
	}
}

/^[ \t]*\*programs:/ {
	print
	output = ""
	do {
		getline
		output = output $0 ORS
		x = 0
		for (name in add)
			if ($0 ~ name)
				remove[++x] = name
		for (; x > 0; --x)
			delete add[remove[x]]
	} while ($0 ~ /\\$/)
	for (name in add) {
		print add[name]
	}
	ORS=""
	print output
	next
}

! /^[ \t]*\*programs:/ {
	ORS="\n"
	print
}
