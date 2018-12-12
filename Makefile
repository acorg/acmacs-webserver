# -*- Makefile -*-
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

# ----------------------------------------------------------------------

all: install

CONFIGURE_BOOST = 1
CONFIGURE_OPENSSL = 1
include $(ACMACSD_ROOT)/share/Makefile.config

LDLIBS = \
  $(AD_LIB)/$(call shared_lib_name,libacmacsbase,1,0) \
  $(OPENSSL_LIBS) $(XZ_LIBS) $(L_BOOST) -lboost_system -lpthread $(CXX_LIBS)

# ----------------------------------------------------------------------

CC = cc

install: install-headers $(TARGETS)
	$(call install_lib,$(ACMACS_WEBSERVER_LIB))

test: install
	test/test
.PHONY: test

# ----------------------------------------------------------------------

$(ACMACS_WEBSERVER_LIB): $(patsubst %.cc,$(BUILD)/%.o,$(ACMACS_WEBSERVER_SOURCES)) | $(DIST)
	$(call echo_shared_lib,$@)
	$(call make_shared_lib,$(ACMACS_WEBSERVER_LIB_NAME),$(ACMACS_WEBSERVER_LIB_MAJOR),$(ACMACS_WEBSERVER_LIB_MINOR)) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(DIST)/wspp-test: $(patsubst %.cc,$(BUILD)/%.o,$(WSPP_TEST_SOURCES)) | $(DIST)
	$(call echo_link_exe,$@)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(AD_RPATH)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
