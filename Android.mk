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
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := eng debug
vtunnel_version := 1.0
VERSION_STR := $(vtunnel_version)
LOCAL_SRC_FILES := src/vtunnel.c
LOCAL_MODULE := vtunnel
LOCAL_LD_LIBS := m
LOCAL_C_INCLUDES := 
LOCAL_SHARED_LIBRARIES :=
LOCAL_CFLAGS := -Wall -g -DVERSION_STR=\"$(VERSION_STR)\"
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := init.vtunnel.rc
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := scripts/init.vtunnel-$(TARGET_ARCH).rc
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
include $(BUILD_PREBUILT)


# for m -j12 iso
iso_img: vtunnel

# for m vtunnel and mm
vtunnel: vtunnel-binaries vtunnel-depends vtunnel-files init-rc-update

vtunnel-binaries:
	test -d external/vtunnel/dist/$(TARGET_PRODUCT)/ || mkdir external/vtunnel/dist/$(TARGET_PRODUCT)/
	cp $(TARGET_OUT)/bin/vtunnel external/vtunnel/dist/$(TARGET_PRODUCT)/

vtunnel-depends:
	cp out/target/product/$(TARGET_ARCH)/obj/kernel/drivers/misc/vmw_vmci/vmw_vmci.ko external/vtunnel/dist/$(TARGET_PRODUCT)/
	cp out/target/product/$(TARGET_ARCH)/obj/kernel/net/vmw_vsock/vsock.ko external/vtunnel/dist/$(TARGET_PRODUCT)/
	cp out/target/product/$(TARGET_ARCH)/obj/kernel/net/vmw_vsock/vmw_vsock_virtio_transport_common.ko external/vtunnel/dist/$(TARGET_PRODUCT)/
	cp out/target/product/$(TARGET_ARCH)/obj/kernel/net/vmw_vsock/vmw_vsock_virtio_transport.ko external/vtunnel/dist/$(TARGET_PRODUCT)/
	cp out/target/product/$(TARGET_ARCH)/obj/kernel/net/vmw_vsock/vmw_vsock_vmci_transport.ko external/vtunnel/dist/$(TARGET_PRODUCT)/

vtunnel-files:
	cp $(TARGET_ROOT_OUT)/init.vtunnel.rc external/vtunnel/dist/$(TARGET_PRODUCT)/init.vtunnel-$(TARGET_ARCH).rc

vtunnel-init-rc-update:
	echo "import /init.vtunnel.rc" >> $(TARGET_ROOT_OUT)/init.rc

