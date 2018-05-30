# -*- Makefile -*-
# Eugene Skepner 2017
# ----------------------------------------------------------------------

MAKEFLAGS = -w

# ----------------------------------------------------------------------

TARGETS = \
	$(ACMACS_WEBSERVER_LIB) \
	$(ACMACS_WEBSERVER_PY_LIB) \
	$(DIST)/wspp-test

ACMACS_WEBSERVER_SOURCES = server.cc server-impl.cc
ACMACS_WEBSERVER_LIB = $(DIST)/libacmacswebserver.so

WSPP_TEST_SOURCES = wspp-test.cc server.cc server-impl.cc

ACMACS_WEBSERVER_LIB_MAJOR = 1
ACMACS_WEBSERVER_LIB_MINOR = 0
ACMACS_WEBSERVER_LIB_NAME = libacmacswebserver
ACMACS_WEBSERVER_LIB = $(DIST)/$(call shared_lib_name,$(ACMACS_WEBSERVER_LIB_NAME),$(ACMACS_WEBSERVER_LIB_MAJOR),$(ACMACS_WEBSERVER_LIB_MINOR))

LDLIBS = \
  $(AD_LIB)/$(call shared_lib_name,libacmacsbase,1,0) \
  -L/usr/local/opt/openssl/lib $(shell pkg-config --libs libssl) \
  $(shell pkg-config --libs liblzma) \
  $(shell pkg-config --libs libcrypto) \
  -L$(AD_LIB) -lboost_system -lpthread \
  $(CXX_LIB)

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

all: check-acmacsd-root $(TARGETS)

install: check-acmacsd-root $(TARGETS)
	$(call install_lib,$(ACMACS_WEBSERVER_LIB))
	if [ ! -d $(AD_INCLUDE)/acmacs-webserver ]; then mkdir $(AD_INCLUDE)/acmacs-webserver; fi
	ln -sf $(abspath cc)/*.hh $(AD_INCLUDE)/acmacs-webserver

test: install
	test/test

# ----------------------------------------------------------------------

-include $(BUILD)/*.d
include $(ACMACSD_ROOT)/share/makefiles/Makefile.dist-build.rules
include $(ACMACSD_ROOT)/share/makefiles/Makefile.rtags

# ----------------------------------------------------------------------

$(ACMACS_WEBSERVER_LIB): $(patsubst %.cc,$(BUILD)/%.o,$(ACMACS_WEBSERVER_SOURCES)) | $(DIST)
	@printf "%-16s %s\n" "SHARED" $@
	@$(call make_shared,$(ACMACS_WEBSERVER_LIB_NAME),$(ACMACS_WEBSERVER_LIB_MAJOR),$(ACMACS_WEBSERVER_LIB_MINOR)) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(DIST)/wspp-test: $(patsubst %.cc,$(BUILD)/%.o,$(WSPP_TEST_SOURCES)) | $(DIST)
	@printf "%-16s %s\n" "LINK" $@
	@$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(AD_RPATH)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
