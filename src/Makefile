#
#	vtunnel
#
#	Copyright 2018 Carnegie Mellon University. All Rights Reserved.
#
#	NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
#	INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
#	UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
#	AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF FITNESS FOR
#	PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS OBTAINED FROM USE OF
#	THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT MAKE ANY WARRANTY OF
#	ANY KIND WITH RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT
#	INFRINGEMENT.
#
#	Released under a BSD (SEI)-style license, please see license.txt or
#	contact permission@sei.cmu.edu for full terms.
#
#	[DISTRIBUTION STATEMENT A] This material has been approved for public
#	release and unlimited distribution.  Please see Copyright notice for
#	non-US Government use and distribution.
#
#	Carnegie Mellon® and CERT® are registered in the U.S. Patent and
#	Trademark Office by Carnegie Mellon University.
#
#	This Software includes and/or makes use of the following Third-Party
#	Software subject to its own license:
#	1. open-vm-tools (https://github.com/vmware/open-vm-tools/blob/9369f1d77fdd90f50b60b44f1ba8c8da00ef55ca/open-vm-tools/LICENSE) Copyright 2018 VMware, Inc..
# 
#	DM18-1221
#

VERSION_STR="\"0.6.0\""
VERSION=0.6.0

# check whether we are cross compiling for openwrt
ifdef CROSS
OUTDIR := .
MAN := man/
else
OUTDIR := ../bin
MAN := ../man
endif

vtunnel-i686-w64-mingw32: CC=i686-w64-mingw32-gcc
vtunnel-i686-w64-mingw32: LD=i686-w64-mingw32-ld
vtunnel-x86_64-w64-mingw32: CC=x86_64-w64-mingw32-gcc
vtunnel-x86_64-w64-mingw32: LD=x86_64-w64-mingw32-ld

CFLAGS = -g -Wall -O2
LDFLAGS = -lpthread -lm

CFLAGS+=-DVERSION_STR=$(VERSION_STR)

.PHONY: all linux windows dist clean doc esx source

all: linux windows esx vyos

linux: vtunnel-x86_64-Linux vtunnel-i386-Linux

windows: vtunnel-x86_64-w64-mingw32 vtunnel-i686-w64-mingw32

esx: vtunnel-esx
	cd ..; tar --owner=0 --group=0 -czvf vtunnel.tgz bin/vtunnel-esx etc/; cd -
	cp ../scripts/install-vtunnel-esx.sh ../install-vtunnel-esx.sh
	tar --exclude=.git -cjvf ../../vtunnel_$(VERSION)_esx.tar.bz2 ../vtunnel.tgz ../install-vtunnel-esx.sh
	rm ../install-vtunnel-esx.sh
	cp ../../vtunnel_$(VERSION)_esx.tar.bz2 ../dist/esx/
	rm ../vtunnel.tgz

openwrt: vtunnel-$(CROSS:-=)
	test -d ../../../package/vtunnel/dist/$(CROSS:-=)/ || mkdir ../../../package/vtunnel/dist/$(CROSS:-=)/
	cp $(OUTDIR)/vtunnel ../../../package/vtunnel/dist/$(CROSS:-=)/
	if [ -f ../../../bin/x86/packages/base/vtunnel_$(VERSION)_$(ARCH).ipk ]; then cp ../../../bin/x86/packages/base/vtunnel_$(VERSION)_$(ARCH).ipk ../../../package/vtunnel/dist/$(CROSS:-=)/; cp ../../../bin/x86/packages/base/vtunnel_$(VERSION)_$(ARCH).ipk ../../../package/vtunnel/dist/$(CROSS:-=)/vtunnel_$(VERSION)_$(CROSS:-=).ipk; fi

i386-Linux: vtunnel-i386-Linux

x86_64-Linux: vtunnel-x86_64-Linux 

x86_64-Linux-vyos: vtunnel-x86_64-Linux-vyos

vtunnel-i486-openwrt-linux-musl:
	test -d $(OUTDIR) || mkdir $(OUTDIR)
	$(CC) -o $(OUTDIR)/vtunnel vtunnel.c $(LDFLAGS) $(CFLAGS) -m32

vtunnel-x86_64-openwrt-linux-musl:
	test -d $(OUTDIR) || mkdir $(OUTDIR)
	$(CC) -o $(OUTDIR)/vtunnel vtunnel.c $(LDFLAGS) $(CFLAGS) 

vtunnel-i486-openwrt-linux-uclibc:
	test -d $(OUTDIR) || mkdir $(OUTDIR)
	$(CC) -o $(OUTDIR)/vtunnel vtunnel.c $(LDFLAGS) $(CFLAGS) -m32

vtunnel-x86_64-openwrt-linux-uclibc:
	test -d $(OUTDIR) || mkdir $(OUTDIR)
	$(CC) -o $(OUTDIR)/vtunnel vtunnel.c $(LDFLAGS) $(CFLAGS) 

vtunnel-i386-Linux-static:
	test -d $(OUTDIR) || mkdir $(OUTDIR)
	$(CC) -o $(OUTDIR)/vtunnel vtunnel.c $(LDFLAGS) $(CFLAGS) -m32 -static

vtunnel-i386-Linux:
	test -d $(OUTDIR) || mkdir $(OUTDIR)
	$(CC) -o $(OUTDIR)/vtunnel vtunnel.c $(LDFLAGS) $(CFLAGS) -m32
	test -d ../dist/i386-Linux || mkdir ../dist/i386-Linux
	cp $(OUTDIR)/vtunnel ../dist/i386-Linux/

vtunnel-x86_64-Linux:
	test -d $(OUTDIR) || mkdir $(OUTDIR)
	$(CC) -o $(OUTDIR)/vtunnel vtunnel.c $(LDFLAGS) $(CFLAGS) 
	test -d ../dist/x86_64-Linux || mkdir ../dist/x86_64-Linux/
	cp $(OUTDIR)/vtunnel ../dist/x86_64-Linux/

vtunnel-x86_64-Linux-vyos:
	test -d $(OUTDIR) || mkdir $(OUTDIR)
	$(CC) -o $(OUTDIR)/vtunnel vtunnel.c $(LDFLAGS) $(CFLAGS) -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
	test -d ../dist/x86_64-Linux || mkdir ../dist/x86_64-Linux/
	cp $(OUTDIR)/vtunnel ../dist/x86_64-Linux/

vtunnel-esx:
	test -d $(OUTDIR) || mkdir $(OUTDIR)
	$(CC) -o $(OUTDIR)/$@ vtunnel.c $(LDFLAGS) $(CFLAGS) -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
	test -d ../dist/esx || mkdir ../dist/esx
	cp $(OUTDIR)/$@ ../dist/esx/

vtunnel-x86_64-w64-mingw32:
	$(CC) -o $(OUTDIR)/vtunnel.exe vtunnel.c windows_error.c -L/usr/lib/gcc/x86_64-w64-mingw32/4.9-posix/ -lws2_32 -DVERSION_STR=$(VERSION_STR) -g -D_POSIX -static $(LDFLAGS)
	test -d ../dist/x86_64-w64-mingw32 || mkdir ../dist/x86_64-w64-mingw32
	cp $(OUTDIR)/vtunnel.exe ../dist/x86_64-w64-mingw32/

vtunnel-i686-w64-mingw32:
	$(CC) -o $(OUTDIR)/vtunnel.exe vtunnel.c windows_error.c -L/usr/lib/gcc/i686-w64-mingw32/4.9-posix/ -lws2_32 -DVERSION_STR=$(VERSION_STR) -g -D_POSIX -static $(LDFLAGS)
	test -d ../dist/i686-w64-mingw32 || mkdir ../dist/i686-w64-mingw32
	cp $(OUTDIR)/vtunnel.exe ../dist/i686-w64-mingw32/

clean:
	rm -f $(OBJECTS) $(OUTDIR)/* vtunnel-*.deb ../vtunnel.tgz
	rm -rf ../html ../latex vtunnel-*-vyos/ vtunnel-*/

source: clean
	cd ../..; tar --exclude=.git --exclude=dist --exclude=bin -cjvf vtunnel_$(VERSION)_src.tar.bz2 vtunnel/; cd -

doc:
	cd ..; doxygen Doxyfile; cd -

rpms: rpm-x86_64-Linux rpm-i386-Linux
debians: debian-x86_64-Linux debian-i386-Linux vyos

dist: all doc rpms debians
	cd ../..; tar --exclude=.git -cjvf vtunnel_$(VERSION)_dist.tar.bz2 vtunnel/; cd -

debian:
	mkdir -p vtunnel-$(VERSION)/DEBIAN/
	mkdir -p vtunnel-$(VERSION)/etc/
	mkdir -p vtunnel-$(VERSION)/bin/
	mkdir -p vtunnel-$(VERSION)/lib/systemd/system/
	cp ../DEBIAN/control  vtunnel-$(VERSION)/DEBIAN/
	sed -i "s/0.5.0/$(VERSION)/"  vtunnel-$(VERSION)/DEBIAN/control
	cp $(OUTDIR)/vtunnel vtunnel-$(VERSION)/bin/
	cp ../scripts/vtunnel.path vtunnel-$(VERSION)/lib/systemd/system/
	cp ../scripts/vtunnel.service vtunnel-$(VERSION)/lib/systemd/system/
	dpkg -b vtunnel-$(VERSION)

debian-x86_64-Linux: x86_64-Linux debian
	mv vtunnel-$(VERSION).deb ../dist/x86_64-Linux/
	rm -rf vtunnel-$(VERSION)

debian-i386-Linux: i386-Linux debian
	mv vtunnel-$(VERSION).deb ../dist/i386-Linux/
	rm -rf vtunnel-$(VERSION)

debian-vyos:
	mkdir -p vtunnel-$(VERSION)-vyos/DEBIAN/
	mkdir -p vtunnel-$(VERSION)-vyos/etc/
	mkdir -p vtunnel-$(VERSION)-vyos/etc/init.d/
	mkdir -p vtunnel-$(VERSION)-vyos/bin/
	cp ../DEBIAN/control  vtunnel-$(VERSION)-vyos/DEBIAN/
	sed -i "s/0.5.0/$(VERSION)/"  vtunnel-$(VERSION)-vyos/DEBIAN/control
	cp $(OUTDIR)/vtunnel vtunnel-$(VERSION)-vyos/bin/
	cp ../scripts/init-vtunnel-debian vtunnel-$(VERSION)-vyos/etc/init.d/vtunnel
	chmod +x vtunnel-$(VERSION)-vyos/etc/init.d/vtunnel
	dpkg -b vtunnel-$(VERSION)-vyos
	rm -rf vtunnel-$(VERSION)-vyos

vyos: x86_64-Linux-vyos debian-vyos
	mv vtunnel-$(VERSION)-vyos.deb ../dist/x86_64-Linux/
	rm -rf vtunnel-$(VERSION)

.ONESHELL: rpm

rpm:
	mkdir -p ../rpmbuild/{RPMS,SRPMS,BUILD,SOURCES,SPECS,tmp}
	cp ../rpmmacros ~/
	cd ../rpmbuild
	sed -i "s:PATH:`pwd`:" ~/.rpmmacros
	mkdir -p vtunnel-$(VERSION)
	mkdir -p vtunnel-$(VERSION)/bin/
	mkdir -p vtunnel-$(VERSION)/lib/systemd/system/
	install -m 755 ../bin/vtunnel vtunnel-$(VERSION)/bin/
	install -m 755 ../scripts/vtunnel.path vtunnel-$(VERSION)/lib/systemd/system/
	install -m 755 ../scripts/vtunnel.service vtunnel-$(VERSION)/lib/systemd/system/
	tar czvf vtunnel-$(VERSION).tar.gz vtunnel-$(VERSION)/
	mv vtunnel-$(VERSION).tar.gz SOURCES/
	cp ../vtunnel.spec SPECS/
	sed -i "s/0.0.0/$(VERSION)/g" SPECS/vtunnel.spec
	rpmbuild -bb --target $(ARCH) SPECS/vtunnel.spec
	cd -
	rm -rf ../rpmbuild ../.rpmmacros

rpm-x86_64-Linux: ARCH=x86_64
rpm-i386-Linux: ARCH=i386

rpm-x86_64-Linux: x86_64-Linux rpm
	cp ../rpmbuild/RPMS/x86_64/vtunnel-$(VERSION)-1.*.x86_64.rpm ../dist/x86_64-Linux/
	cp ../rpmbuild/RPMS/x86_64/vtunnel-debuginfo-$(VERSION)-1.*.x86_64.rpm ../dist/x86_64-Linux/
	rm -rf ../rpmbuild

rpm-i386-Linux: i386-Linux rpm
	cp ../rpmbuild/RPMS/i386/vtunnel-$(VERSION)-1.*.i386.rpm ../dist/i386-Linux/
	cp ../rpmbuild/RPMS/i386/vtunnel-debuginfo-$(VERSION)-1.*.i386.rpm ../dist/i386-Linux/
	rm -rf ../rpmbuild


