# -*- Makefile -*-
# Eugene Skepner 2017

# ----------------------------------------------------------------------

MAKEFLAGS = -w

# ----------------------------------------------------------------------

WSPP_TEST = $(DIST)/wspp-test

WSPP_TEST_SOURCES = wspp-test.cc
WSPP_LDLIBS = -L$(LIB_DIR) $$(pkg-config --libs libcrypto) -lssl -lboost_system

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
LDLIBS = -L$(LIB_DIR) -lpthread $$(pkg-config --libs liblzma)

PKG_INCLUDES = $$(pkg-config --cflags liblzma) $$(pkg-config --cflags libcrypto)

ifeq ($(shell uname -s),Darwin)
PKG_INCLUDES += -I/usr/local/opt/openssl/include
# $$(pkg-config --cflags libuv)
LDLIBS += -L/usr/local/opt/openssl/lib
endif

# ----------------------------------------------------------------------

BUILD = build
DIST = $(abspath dist)
CC = cc

all: check-acmacsd-root $(WSPP_TEST)

install: check-acmacsd-root $(WSPP_TEST)
	#ln -sf $(ACMACS_WEBSERVER) $(ACMACSD_ROOT)/bin

test: install
	test/test

# ----------------------------------------------------------------------

-include $(BUILD)/*.d

# ----------------------------------------------------------------------

$(WSPP_TEST): $(patsubst %.cc,$(BUILD)/%.o,$(WSPP_TEST_SOURCES)) | $(DIST)
	g++ $(LDFLAGS) -o $@ $^ $(WSPP_LDLIBS) $(LDLIBS)

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
