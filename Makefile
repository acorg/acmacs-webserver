# -*- Makefile -*-
# Eugene Skepner 2017

# ----------------------------------------------------------------------

MAKEFLAGS = -w

# ----------------------------------------------------------------------

ACMACS_WEBSERVER_SOURCES = server.cc server-impl.cc server-settings.cc
ACMACS_WEBSERVER_LIB = $(DIST)/libacmacswebserver.so

WSPP_TEST = $(DIST)/wspp-test
WSPP_TEST_SOURCES = wspp-test.cc server.cc server-impl.cc server-settings.cc

LDLIBS = -L$(LIB_DIR) -L/usr/local/opt/openssl/lib $$(pkg-config --libs libssl) $$(pkg-config --libs liblzma) $$(pkg-config --libs libcrypto) -lboost_filesystem -lboost_system -lpthread

# ----------------------------------------------------------------------

CLANG = $(shell if g++ --version 2>&1 | grep -i llvm >/dev/null; then echo Y; else echo N; fi)
ifeq ($(CLANG),Y)
  WEVERYTHING = -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded
  WARNINGS = -Wno-weak-vtables # -Wno-padded
  STD = c++1z
else
  WEVERYTHING = -Wall -Wextra
  WARNINGS =
  STD = c++1z
endif

LIB_DIR = $(ACMACSD_ROOT)/lib

OPTIMIZATION = -O3 #-fvisibility=hidden -flto
PROFILE = # -pg
CXXFLAGS = -g -MMD $(OPTIMIZATION) $(PROFILE) -fPIC -std=$(STD) $(WEVERYTHING) $(WARNINGS) -Icc -I$(BUILD)/include -I$(ACMACSD_ROOT)/include $(PKG_INCLUDES)
LDFLAGS = $(OPTIMIZATION) $(PROFILE)

PKG_INCLUDES = $$(pkg-config --cflags liblzma) $$(pkg-config --cflags libcrypto)

ifeq ($(shell uname -s),Darwin)
PKG_INCLUDES += -I/usr/local/opt/openssl/include
# $$(pkg-config --cflags libuv)
endif

# ----------------------------------------------------------------------

BUILD = build
DIST = $(abspath dist)
CC = cc

all: check-acmacsd-root $(ACMACS_WEBSERVER_LIB) $(WSPP_TEST)

install: check-acmacsd-root $(ACMACS_WEBSERVER_LIB) $(WSPP_TEST)
	ln -sf $(ACMACS_WEBSERVER_LIB) $(ACMACSD_ROOT)/lib
	if [ $$(uname) = "Darwin" ]; then /usr/bin/install_name_tool -id $(ACMACSD_ROOT)/lib/$(notdir $(ACMACS_WEBSERVER_LIB)) $(ACMACSD_ROOT)/lib/$(notdir $(ACMACS_WEBSERVER_LIB)); fi
	if [ ! -d $(ACMACSD_ROOT)/include/acmacs-webserver ]; then mkdir $(ACMACSD_ROOT)/include/acmacs-webserver; fi
	ln -sf $(abspath cc)/*.hh $(ACMACSD_ROOT)/include/acmacs-webserver

test: install
	test/test

# ----------------------------------------------------------------------

-include $(BUILD)/*.d

# ----------------------------------------------------------------------

$(ACMACS_WEBSERVER_LIB): $(patsubst %.cc,$(BUILD)/%.o,$(ACMACS_WEBSERVER_SOURCES)) | $(DIST) $(LOCATION_DB_LIB)
	g++ -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(WSPP_TEST): $(patsubst %.cc,$(BUILD)/%.o,$(WSPP_TEST_SOURCES)) | $(DIST)
	g++ $(LDFLAGS) -o $@ $^ $(LDLIBS)

# ----------------------------------------------------------------------

clean:
	rm -rf $(DIST) $(BUILD)/*.o $(BUILD)/*.d

distclean: clean
	rm -rf $(BUILD)

# ----------------------------------------------------------------------

$(BUILD)/%.o: $(CC)/%.cc | $(BUILD)
	@echo $<
	@g++ $(CXXFLAGS) -c -o $@ $<

# ----------------------------------------------------------------------

check-acmacsd-root:
ifndef ACMACSD_ROOT
	$(error ACMACSD_ROOT is not set)
endif

$(DIST):
	mkdir -p $(DIST)

$(BUILD):
	mkdir -p $(BUILD)

.PHONY: check-acmacsd-root

# ----------------------------------------------------------------------
# UWS: uWebSocket based

# UWS_ACMACS_WEBSERVER = $(DIST)/uws-acmacs-webserver
# UWS_TEST = $(DIST)/uws-test

# UWS_SOURCES = uws/acmacs-webserver.cc http-request-dispatcher.cc
# UWS_TEST_SOURCES = uws/uws-test.cc uws/http-request-dispatcher.cc
# UWS_LDLIBS = -luWS $$(pkg-config --libs libuv) -lssl -lz

# $(UWS_ACMACS_WEBSERVER): $(patsubst %.cc,$(BUILD)/%.o,$(SOURCES)) | $(DIST)
#	g++ $(LDFLAGS) -o $@ $^ $(UWS_LDLIBS) $(LDLIBS)

# $(UWS_TEST): $(patsubst %.cc,$(BUILD)/%.o,$(UWS_TEST_SOURCES)) | $(DIST)
#	g++ $(LDFLAGS) -o $@ $^ $(UWS_LDLIBS) $(LDLIBS)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
