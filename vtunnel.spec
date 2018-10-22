Name:		vtunnel
Version:	0.0.0
Release:	1%{?dist}
Summary:	vsock tunnel

Group:		Applications/Emulators
License:	BSD
URL:		http://www.cert.org
Source0:	%{name}-%{version}.tar.gz

#Requires:	

%description
vsock tunnel

#%global debug_package %{nil}

%prep
%autosetup
#%setup -q


%build


%install
mkdir -p %{buildroot}/bin/
mkdir -p %{buildroot}/lib/systemd/system/
install -m 755 bin/vtunnel %{buildroot}/bin/
install -m 755 lib/systemd/system/vtunnel.path %{buildroot}/lib/systemd/system/
install -m 755 lib/systemd/system/vtunnel.service %{buildroot}/lib/systemd/system/

%files
/bin/vtunnel
/lib/systemd/system/vtunnel.path
/lib/systemd/system/vtunnel.service

#%doc

%clean
rm -rf %{buildroot}

%changelog

