#
#       vtunnel
#
#       Copyright 2018 Carnegie Mellon University. All Rights Reserved.
#
#       NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
#       INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
#       UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR IMPLIED,#       AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF FITNESS FOR
#       PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS OBTAINED FROM USE OF#       THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF
#       ANY KIND WITH RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT
#       INFRINGEMENT.
#
#       Released under a BSD (SEI)-style license, please see license.txt or
#       contact permission@sei.cmu.edu for full terms.
#
#       [DISTRIBUTION STATEMENT A] This material has been approved for public
#       release and unlimited distribution.  Please see Copyright notice for
#       non-US Government use and distribution.
#
#       Carnegie Mellon® and CERT® are registered in the U.S. Patent and
#       Trademark Office by Carnegie Mellon University.
#
#       This Software includes and/or makes use of the following Third-Party
#       Software subject to its own license:
#       1. open-vm-tools (https://github.com/vmware/open-vm-tools/blob/9369f1d77fdd90f50b60b44f1ba8c8da00ef55ca/open-vm-tools/LICENSE) Copyright 2018 VMware, Inc..
# 
#       DM18-1221

##############################################
# OpenWrt Makefile for vtunnel program
#
#
# Most of the variables used here are defined in
# the include directives below. We just need to
# specify a basic description of the package,
# where to build our program, where to find
# the source files, and where to install the
# compiled program on the router.
#
# Be very careful of spacing in this file.
# Indents should be tabs, not spaces, and
# there should be no trailing whitespace in
# lines that are not commented.
#
##############################################

include $(TOPDIR)/rules.mk

# Name and release number of this package
PKG_NAME:=vtunnel
PKG_RELEASE:=0.6.0

# This specifies the directory where we're going to build the program.
# The root build directory, $(BUILD_DIR), is by default the build_mipsel
# directory in your OpenWrt SDK directory
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

# Specify package information for this program.
# The variables defined here should be self explanatory.
# Package description I had to change from DESCRIPTION.
# DEPENDS had to be added as well.
define Package/vtunnel
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=vtunnel -- vsock tunnel
	Package/PKG_NAME/description:=\
	Tunnels IP traffic between guest \\\
	and host machines.
	DEPENDS:=+libpthread
	MAINTAINER:=Adam Welle <arwelle@cert.org>
endef

# Specify what needs to be done to prepare for building the package.
# In our case, we need to copy the source files to the build directory.
# This is NOT the default.  The default uses the PKG_SOURCE_URL and the
# PKG_SOURCE which is not defined here to download the source from the web.
# In order to just build a simple program that we have just written, it is
# much easier to do it this way.
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/*c $(PKG_BUILD_DIR)/
	$(CP) ./src/*h $(PKG_BUILD_DIR)/
	$(CP) ./src/Makefile $(PKG_BUILD_DIR)/
endef

# We do not need to define Build/Configure or Build/Compile directives
# The defaults are appropriate for compiling a simple program such as this one

define Build/Compile
	CFLAGS="$(TARGET_CFLAGS) $(EXTRA_CPPFLAGS) " \
	LDFLAGS="$(EXTRA_LDFLAGS) " \
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		CROSS="$(TARGET_CROSS)" \
		CXXFLAGS="$(TARGET_CFLAGS) $(EXTRA_CPPFLAGS) " \
		ARCH="$(ARCH)" \
		openwrt;
endef
	
# Specify where and how to install the program. We will copy multiple files to
# the router. The $(1) variable represents the root directory on the router 
# running OpenWrt. The $(INSTALL_DIR) variable contains a command to prepare 
# the install directory if it does not already exist.  Likewise $(INSTALL_BIN) 
# contains the command to copy the binary file from its current location (in our
# case the build directory) to the install directory. $(INSTALL_DATA) will copy
# files with permissions 644, while $(INSTALL_BIN)  uses 755.

define Package/vtunnel/install
	$(INSTALL_DIR) $(1)/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/vtunnel $(1)/bin/
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) ./scripts/init-vtunnel-openwrt $(1)/etc/init.d/vtunnel
endef

# Configure the package after install
define Package/vtunnel/postinst
#!/bin/sh
if [ -z "$${IPKG_INSTROOT}" ]; then
	echo "Enabling rc.d symlink for vtunnel"
	/etc/init.d/vtunnel enable
fi
exit 0
endef

# Clean up prior to package removal
define Package/vtunnel/prerm
#!/bin/sh
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Removing rc.d symlink for vtunnel"
        /etc/init.d/vtunnel disable
fi
exit 0

exit 0
endef

# This line executes the necessary commands to compile our program.
# The above define directives specify all the information needed, but this
# line calls BuildPackage which in turn actually uses this information to
# build a package.
$(eval $(call BuildPackage,vtunnel))

