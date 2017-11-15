# -*- Makefile -*-
# Eugene Skepner 2017
# ----------------------------------------------------------------------

MAKEFLAGS = -w

# ----------------------------------------------------------------------

ACMACS_WEBSERVER_SOURCES = server.cc server-impl.cc server-settings.cc
ACMACS_WEBSERVER_LIB = $(DIST)/libacmacswebserver.so

WSPP_TEST = $(DIST)/wspp-test
WSPP_TEST_SOURCES = wspp-test.cc server.cc server-impl.cc server-settings.cc

LDLIBS = -L$(AD_LIB) -L/usr/local/opt/openssl/lib $(shell pkg-config --libs libssl) $(shell pkg-config --libs liblzma) $(shell pkg-config --libs libcrypto) -lboost_system -lpthread $(FS_LIB)

# ----------------------------------------------------------------------

include $(ACMACSD_ROOT)/share/makefiles/Makefile.g++
include $(ACMACSD_ROOT)/share/makefiles/Makefile.dist-build.vars

CXXFLAGS = -g -MMD $(OPTIMIZATION) $(PROFILE) -fPIC -std=$(STD) $(WARNINGS) -Icc -I$(AD_INCLUDE) $(PKG_INCLUDES)
LDFLAGS = $(OPTIMIZATION) $(PROFILE)

PKG_INCLUDES = $(shell pkg-config --cflags liblzma) $(shell pkg-config --cflags libcrypto)

ifeq ($(shell uname -s),Darwin)
PKG_INCLUDES += -I/usr/local/opt/openssl/include
# $(shell pkg-config --cflags libuv)
endif

# ----------------------------------------------------------------------

CC = cc

all: check-acmacsd-root $(ACMACS_WEBSERVER_LIB) $(WSPP_TEST)

install: check-acmacsd-root $(ACMACS_WEBSERVER_LIB) $(WSPP_TEST)
	ln -sf $(ACMACS_WEBSERVER_LIB) $(AD_LIB)
	if [ $$(uname) = "Darwin" ]; then /usr/bin/install_name_tool -id $(AD_LIB)/$(notdir $(ACMACS_WEBSERVER_LIB)) $(AD_LIB)/$(notdir $(ACMACS_WEBSERVER_LIB)); fi
	if [ ! -d $(AD_INCLUDE)/acmacs-webserver ]; then mkdir $(AD_INCLUDE)/acmacs-webserver; fi
	ln -sf $(abspath cc)/*.hh $(AD_INCLUDE)/acmacs-webserver

test: install
	test/test

# ----------------------------------------------------------------------

-include $(BUILD)/*.d
include $(ACMACSD_ROOT)/share/makefiles/Makefile.dist-build.rules
include $(ACMACSD_ROOT)/share/makefiles/Makefile.rtags

# ----------------------------------------------------------------------

$(ACMACS_WEBSERVER_LIB): $(patsubst %.cc,$(BUILD)/%.o,$(ACMACS_WEBSERVER_SOURCES)) | $(DIST) $(LOCATION_DB_LIB)
	@printf "%-16s %s\n" "SHARED" $@
	@$(CXX) -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(WSPP_TEST): $(patsubst %.cc,$(BUILD)/%.o,$(WSPP_TEST_SOURCES)) | $(DIST)
	@printf "%-16s %s\n" "LINK" $@
	@$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
