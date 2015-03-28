# TODO: add a rule to fetch the submodules if missing
# TODO: add a rule to fetch the esp open sdk submodule if missing

FRANK=frankenstein
FRANK_CONFIG=config-esp8266
SDK=esp-open-sdk

all: prereq $(FRANK_CONFIG)
	$(MAKE) -C $(FRANK)

menuconfig:
	@diff -q $(FRANK)/.config $(FRANK_CONFIG) || { echo "[WARNING] Config in $(FRANK) is different from $(FRANK_CONFIG). Fix that first." ; false ; }
	$(MAKE) -C $(FRANK) menuconfig
	@diff -q $(FRANK)/.config $(FRANK_CONFIG) || cp -v $(FRANK)/.config $(FRANK_CONFIG)

forceconfig:
	@cp -v $(FRANK_CONFIG) $(FRANK)/.config

prereq: | $(SDK) $(FRANK) $(FRANK)/.config $(FRANK)/antares .stamp.frank

# I would rather use a symlink, but make menuconfig keeps breaking it
$(FRANK)/.config:
	@cp -v $(FRANK_CONFIG) $(FRANK)/.config

# We track frankenstein as a submodule because then we can stick at a specific git version
# So this should be a NO-OP
$(FRANK):
	@echo "oops - $(FRANK) is missing, this should be a git submodule!"
	@false

# Similarly, antares...
$(FRANK)/antares: $(FRANK)
	$(MAKE) -C $(FRANK) antares

# On my workstation I symlink this elsewhere so I dont have to keep building it
# For a fresh build on a new PC we want to clone it from github
# Dont bother using a submodule (although perhaps that might be useful, so it can be stuck at a particular version)
$(SDK):
	@echo "oops - $(SDK) is missing, either create a symlink, or clone it here!"
	@echo "Clone alternatives:"
	@echo "1. https://github.com/pastcompute/esp-open-sdk"
	@echo "2. https://github.com/pfalcon/esp-open-sdk"
	@false

# Force a distclean in frankenstein when first setup
.stamp.frank:
	$(MAKE) -C $(FRANK) distclean
	touch $@

.PHONY: all prereq menuconfig
