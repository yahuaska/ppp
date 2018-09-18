cmake_minimum_required(VERSION 3.5)
project(ppp)

set(CMAKE_CXX_STANDARD 14)

include_directories(pppd)
include_directories(pppd/plugins/pppoatm)
include_directories(pppd/plugins/pppol2tp)
include_directories(pppd/plugins/radius)
include_directories(pppd/plugins/rp-pppoe)

add_executable(pppd
        pppd/auth.c
        pppd/cbcp.c
        pppd/cbcp.h
        pppd/ccp.c
        pppd/ccp.h
        pppd/chap-md5.c
        pppd/chap-md5.h
        pppd/chap-new.c
        pppd/chap-new.h
        pppd/chap_ms.c
        pppd/chap_ms.h
        pppd/demand.c
        pppd/eap.c
        pppd/eap.h
        pppd/ecp.c
        pppd/ecp.h
        pppd/eui64.c
        pppd/eui64.h
        pppd/fsm.cpp
        pppd/fsm.c
        pppd/fsm.h
        pppd/ipcp.c
        pppd/ipcp.h
        pppd/ipv6cp.c
        pppd/ipv6cp.h
        pppd/ipxcp.c
        pppd/ipxcp.h
        pppd/lcp.cpp
        pppd/lcp.c
        pppd/lcp.h
        pppd/magic.c
        pppd/magic.h
        pppd/main.c
        pppd/Makefile.linux
        pppd/Makefile.sol2
        pppd/md4.c
        pppd/md4.h
        pppd/md5.c
        pppd/md5.h
        pppd/mppe.h
        pppd/multilink.c
        pppd/options.c
        pppd/patchlevel.h
        pppd/pathnames.h
        pppd/ppp.pam
        pppd/pppcrypt.c
        pppd/pppcrypt.h
        pppd/pppd.8
        pppd/pppd.h
        pppd/session.c
        pppd/session.h
        pppd/sha1.c
        pppd/sha1.h
        pppd/spinlock.c
        pppd/spinlock.h
        pppd/srp-entry.8
        pppd/srp-entry.c
        pppd/sys-linux.c
        pppd/sys-solaris.c
        pppd/tdb.c
        pppd/tdb.h
        pppd/tty.c
        pppd/upap.c
        pppd/upap.h
        pppd/utils.c)


ADD_DEFINITIONS(-DUSE_TDB -DHAVE_MULTILINK -DINET6)
add_executable(pppd2
        pppd/main.cpp
        pppd/lcp.cpp
        pppd/fsm.cpp
        pppd/options.cpp
        )
ADD_DEFINITIONS(-DUSE_TDB -DHAVE_MULTILINK -DINET6)
add_executable(test_options
        pppd/options.cpp
        test/test_options.cpp)