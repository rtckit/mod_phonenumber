NAME       = phonenumber
MODNAME    = mod_$(NAME).so
VERSION    = 1.0.0
MODOBJ     = mod_$(NAME).o mod_$(NAME)_util.o mod_$(NAME)_actions.o
MODCFLAGS  = -Wall -Werror
MODLDFLAGS = -lphonenumber -lgeocoding

CC  = gcc
CXX = g++

ifeq ($(BUILD),dist)
CFLAGS   = -fPIC -O2 -s `pkg-config --cflags freeswitch` $(MODCFLAGS)
CXXFLAGS = -fPIC -O2 -s `pkg-config --cflags freeswitch` $(MODCFLAGS)
else
CFLAGS   = -fPIC -g -ggdb `pkg-config --cflags freeswitch` $(MODCFLAGS)
CXXFLAGS = -fPIC -g -ggdb `pkg-config --cflags freeswitch` $(MODCFLAGS)
endif

LDFLAGS = `pkg-config --libs freeswitch` $(MODLDFLAGS)

.PHONY: all
all: $(MODNAME)

$(MODNAME): $(MODOBJ)
	$(CXX) -shared -o $@ $(MODOBJ) $(LDFLAGS)

.c.o: $<
	$(CXX) $(CXXFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -f $(MODNAME) $(MODOBJ) *.la *lo

.PHONY: install
install: $(MODNAME)
	install -d $(DESTDIR)/usr/lib/freeswitch/mod
	install -m 0644 $(MODNAME) $(DESTDIR)/usr/lib/freeswitch/mod
	install -d $(DESTDIR)/etc/freeswitch/autoload_configs
	install -m 0644 $(NAME).conf.xml $(DESTDIR)/etc/freeswitch/autoload_configs

.PHONY: deb
deb: install
	$(eval BUILD_ROOT:=$(shell mktemp -d))
	$(eval ARCH:=$(shell dpkg --print-architecture))
	$(eval FREESWITCH_VERSION:=$(shell freeswitch -version | cut -d' ' -f3 | cut -d'-' -f1 | cut -d'.' -f1,2))
	cp -r debian/* $(BUILD_ROOT)/

	mkdir -p $(BUILD_ROOT)/usr/lib/freeswitch/mod
	mkdir -p $(BUILD_ROOT)/etc/freeswitch/autoload_configs

	cp /etc/freeswitch/autoload_configs/$(NAME).conf.xml $(BUILD_ROOT)/etc/freeswitch/autoload_configs/$(NAME).conf.xml
	cp /usr/lib/freeswitch/mod/$(MODNAME) $(BUILD_ROOT)/usr/lib/freeswitch/mod/$(MODNAME)
	strip -s $(BUILD_ROOT)/usr/lib/freeswitch/mod/$(MODNAME)

	sed -i "s/_ARCH_/$(ARCH)/g" $(BUILD_ROOT)/DEBIAN/control
	sed -i "s/_MOD_VERSION_/$(VERSION)/g" $(BUILD_ROOT)/DEBIAN/control
	sed -i "s/_FREESWITCH_VERSION_/$(FREESWITCH_VERSION)/g" $(BUILD_ROOT)/DEBIAN/control
	dpkg-deb --build $(BUILD_ROOT) freeswitch-mod-$(NAME).deb

	rm -rf $(BUILD_ROOT)

.PHONY: apk
apk: install
	id rtckit || adduser --disabled-password --gecos '' rtckit
	$(eval BUILD_ROOT:=$(shell mktemp -d))
	$(eval ARCH:=$(shell uname -m))

	cp -r alpine/* $(BUILD_ROOT)/
	mkdir -p $(BUILD_ROOT)/usr/lib/freeswitch/mod
	mkdir -p $(BUILD_ROOT)/etc/freeswitch/autoload_configs
	cp /etc/freeswitch/autoload_configs/$(NAME).conf.xml $(BUILD_ROOT)/etc/freeswitch/autoload_configs/$(NAME).conf.xml
	cp /usr/lib/freeswitch/mod/$(MODNAME) $(BUILD_ROOT)/usr/lib/freeswitch/mod/$(MODNAME)
	strip -s $(BUILD_ROOT)/usr/lib/freeswitch/mod/$(MODNAME)

	chmod 0777 $(BUILD_ROOT)

	sed -i "s/_ARCH_/$(ARCH)/g" $(BUILD_ROOT)/APKBUILD
	su -m rtckit -c -- 'cd $(BUILD_ROOT) && abuild'
	cp /root/packages/tmp/x86_64/freeswitch-$(NAME)-*.apk ./freeswitch-$(NAME).apk

	rm -rf $(BUILD_ROOT)

.PHONY: check
check: $(MODNAME)
	mkdir -p .libs
	cp $(MODNAME) .libs
	$(CC) $(CFLAGS) $(LDFLAGS) -o test/test_$(NAME) test/test_$(NAME).c
	cd test && ./test_$(NAME)

create-docker-%:
	docker build -t mod_$(NAME):$* -f docker/$* .

run-docker-%:
	docker run --rm -it --network host --name mod_$(NAME)-$* mod_$(NAME):$*

shell-docker-%:
	docker run --rm -it --network host --name mod_$(NAME)-$* mod_$(NAME):$* bash

dist-deb-%:
	$(eval ARCH:=$(shell dpkg --print-architecture))
	mkdir -p dist/$*
	docker create --name mod_$(NAME)-$* mod_$(NAME):$*
	docker cp mod_$(NAME)-$*:/usr/src/mod_$(NAME)/freeswitch-mod-$(NAME).deb dist/$*/freeswitch-mod-$(NAME)_$(VERSION)_$(ARCH).deb
	docker rm mod_$(NAME)-$*

dist: dist-deb-debian-buster
