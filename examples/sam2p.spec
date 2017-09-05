# Contributed by Richard Zidlicky <rz@linux-m68k.org> .
# Works for some version of Fedora.
# This is for debug-flavor. Do not remove. Package is stripped conditionally.
#%#define __os_install_post       %{nil}
# %#define __spec_install_post /usr/lib/rpm/brp-compress

%define name sam2p
%define version 0.47
%define sfx tar.gz
%define release 2
%define descr convert images to PDF

Summary: %{descr}
Name: %{name}
Version: %{version}
Release: %{release}
Source0: %{name}-%{version}.%{sfx}
Source1: sam2p_pdf_scale.pl
#Patch0: %{name}-rz.patch
License: GPLv2, see COPYING
Group: System Environment/Libs
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
#Prefix: %#{_prefix}
#URL:

%description

%{descr}


%prep
case "${RPM_COMMAND:-all}" in
all)

 # -n dirname
%setup -q
#%#patch0 -p1 -b .rz
;;
esac

%build

case "${RPM_COMMAND:-all}" in
all|config)
#export CFLAGS="$RPM_OPT_FLAGS -O1 -g"
#./configure --prefix=%{prefix}
%configure
;;
esac

case "${RPM_COMMAND:-all}" in
all|config|build)
make
;;
esac

%install

case "${RPM_COMMAND:-all}" in
all|config|build|install)
%makeinstall
;;esac
install -m 755 %{SOURCE1} $RPM_BUILD_ROOT%{_bindir}/sam2p_pdf_scale

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc COPYING README
%{_prefix}/*
#%#config /etc/*

%changelog
* Mon Mar 14 2011 Richard Zidlicky <rz@linux-m68k.org> - 0.47-2
- add sam2p_pdf_scale

* Mon Mar 14 2011 Richard Zidlicky <rz@linux-m68k.org> - 0.47-1
- first build of sam2p

* Fri Dec 26 2003 Richard Zidlicky <rz@linux-m68k.org>
- created skel.spec
