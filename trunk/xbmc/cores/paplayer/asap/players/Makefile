PERL ?= perl
XASM ?= xasm -q -o $@
MADS ?= mads -s -o:$@

PLAYERS_OBX_DIR ?= ../players
PLAYERS = cmc cm3 cms dlt mpt rmt4 rmt8 tmc tm2
PLAYERS_OBX = $(PLAYERS:%=$(PLAYERS_OBX_DIR)/%.obx)

../players.h: ../files2anylang.pl $(PLAYERS_OBX)
	$(PERL) $^ >$@

$(PLAYERS_OBX_DIR)/cmc.obx: ../players/cmc.asx
	$(XASM) -d CM3=0 $<

$(PLAYERS_OBX_DIR)/cm3.obx: ../players/cmc.asx
	$(XASM) -d CM3=1 $<

$(PLAYERS_OBX_DIR)/dlt.obx: ../players/dlt.as8
	$(MADS) -c $<

$(PLAYERS_OBX_DIR)/rmt4.obx: ../players/rmt.asx
	$(XASM) -d STEREOMODE=0 $<

$(PLAYERS_OBX_DIR)/rmt8.obx: ../players/rmt.asx
	$(XASM) -d STEREOMODE=1 $<

$(PLAYERS_OBX_DIR)/%.obx: ../players/%.asx
	$(XASM) $<

clean:
	$(RM) $(PLAYERS_OBX)
