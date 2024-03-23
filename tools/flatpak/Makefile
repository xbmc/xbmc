PROJECT ?= tv.kodi.Kodi

.PHONY: update-addons build flatpak install run clean

update-addons:
	cd tools && python3 addon_updater.py -r
	find addons -type d -not -path "addons/pvr.*" | sort | sed -r -e 's|^addons/?||' -e '/^$$/d' > addon-list.txt
build:
	flatpak-builder build-dir $(PROJECT).yml --repo=repo --force-clean --ccache 2>&1 | tee -a build.log
flatpak:
	flatpak build-bundle repo $(PROJECT).flatpak $(PROJECT)
install:
	flatpak install local $(PROJECT)
run:
	flatpak update -y $(PROJECT)
	flatpak run $(PROJECT)
clean:
	rm -rf .flatpak-builder/cache
