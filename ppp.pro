# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

#TARGET = ppp

HEADERS = \
   $$PWD/common/zlib.h \
   $$PWD/include/linux/if_ppp.h \
   $$PWD/include/linux/ppp-comp.h \
   $$PWD/include/linux/ppp_defs.h \
   $$PWD/include/net/if_ppp.h \
   $$PWD/include/net/ppp-comp.h \
   $$PWD/include/net/ppp_defs.h \
   $$PWD/include/net/pppio.h \
   $$PWD/include/net/slcompress.h \
   $$PWD/include/net/vjcompress.h \
   $$PWD/modules/ppp_mod.h \
   $$PWD/pppd/plugins/pppoatm/atm.h \
   $$PWD/pppd/plugins/pppoatm/atmres.h \
   $$PWD/pppd/plugins/pppoatm/atmsap.h \
   $$PWD/pppd/plugins/pppol2tp/l2tp_event.h \
   $$PWD/pppd/plugins/radius/includes.h \
   $$PWD/pppd/plugins/radius/options.h \
   $$PWD/pppd/plugins/radius/pathnames.h \
   $$PWD/pppd/plugins/radius/radiusclient.h \
   $$PWD/pppd/plugins/rp-pppoe/config.h \
   $$PWD/pppd/plugins/rp-pppoe/pppoe.h \
   $$PWD/pppd/cbcp.h \
   $$PWD/pppd/ccp.h \
   $$PWD/pppd/chap-md5.h \
   $$PWD/pppd/chap-new.h \
   $$PWD/pppd/chap_ms.h \
   $$PWD/pppd/eap.h \
   $$PWD/pppd/ecp.h \
   $$PWD/pppd/eui64.h \
   $$PWD/pppd/fsm.h \
   $$PWD/pppd/ipcp.h \
   $$PWD/pppd/ipv6cp.h \
   $$PWD/pppd/ipxcp.h \
   $$PWD/pppd/lcp.h \
   $$PWD/pppd/magic.h \
   $$PWD/pppd/md4.h \
   $$PWD/pppd/md5.h \
   $$PWD/pppd/mppe.h \
   $$PWD/pppd/patchlevel.h \
   $$PWD/pppd/pathnames.h \
   $$PWD/pppd/pppcrypt.h \
   $$PWD/pppd/pppd.h \
   $$PWD/pppd/session.h \
   $$PWD/pppd/sha1.h \
   $$PWD/pppd/spinlock.h \
   $$PWD/pppd/tdb.h \
   $$PWD/pppd/upap.h \
   $$PWD/pppdump/ppp-comp.h \
   $$PWD/pppdump/zlib.h

SOURCES = \
   $$PWD/chat/chat.c \
   $$PWD/common/zlib.c \
   $$PWD/contrib/pppgetpass/pppgetpass.gtk.c \
   $$PWD/contrib/pppgetpass/pppgetpass.vt.c \
   $$PWD/modules/bsd-comp.c \
   $$PWD/modules/deflate.c \
   $$PWD/modules/if_ppp.c \
   $$PWD/modules/ppp.c \
   $$PWD/modules/ppp_ahdlc.c \
   $$PWD/modules/ppp_comp.c \
   $$PWD/modules/vjcompress.c \
   $$PWD/pppd/plugins/pppoatm/ans.c \
   $$PWD/pppd/plugins/pppoatm/misc.c \
   $$PWD/pppd/plugins/pppoatm/pppoatm.c \
   $$PWD/pppd/plugins/pppoatm/text2atm.c \
   $$PWD/pppd/plugins/pppoatm/text2qos.c \
   $$PWD/pppd/plugins/pppol2tp/openl2tp.c \
   $$PWD/pppd/plugins/pppol2tp/pppol2tp.c \
   $$PWD/pppd/plugins/radius/avpair.c \
   $$PWD/pppd/plugins/radius/buildreq.c \
   $$PWD/pppd/plugins/radius/clientid.c \
   $$PWD/pppd/plugins/radius/config.c \
   $$PWD/pppd/plugins/radius/dict.c \
   $$PWD/pppd/plugins/radius/ip_util.c \
   $$PWD/pppd/plugins/radius/lock.c \
   $$PWD/pppd/plugins/radius/md5.c \
   $$PWD/pppd/plugins/radius/radattr.c \
   $$PWD/pppd/plugins/radius/radius.c \
   $$PWD/pppd/plugins/radius/radrealms.c \
   $$PWD/pppd/plugins/radius/sendserver.c \
   $$PWD/pppd/plugins/radius/util.c \
   $$PWD/pppd/plugins/rp-pppoe/common.c \
   $$PWD/pppd/plugins/rp-pppoe/debug.c \
   $$PWD/pppd/plugins/rp-pppoe/discovery.c \
   $$PWD/pppd/plugins/rp-pppoe/if.c \
   $$PWD/pppd/plugins/rp-pppoe/plugin.c \
   $$PWD/pppd/plugins/rp-pppoe/pppoe-discovery.c \
   $$PWD/pppd/plugins/minconn.c \
   $$PWD/pppd/plugins/passprompt.c \
   $$PWD/pppd/plugins/passwordfd.c \
   $$PWD/pppd/plugins/winbind.c \
   $$PWD/pppd/auth.c \
   $$PWD/pppd/cbcp.c \
   $$PWD/pppd/ccp.c \
   $$PWD/pppd/chap-md5.c \
   $$PWD/pppd/chap-new.c \
   $$PWD/pppd/chap_ms.c \
   $$PWD/pppd/demand.c \
   $$PWD/pppd/eap.c \
   $$PWD/pppd/ecp.c \
   $$PWD/pppd/eui64.c \
   $$PWD/pppd/fsm.c \
   $$PWD/pppd/ipcp.c \
   $$PWD/pppd/ipv6cp.c \
   $$PWD/pppd/ipxcp.c \
   $$PWD/pppd/lcp.c \
   $$PWD/pppd/magic.c \
   $$PWD/pppd/main.c \
   $$PWD/pppd/md4.c \
   $$PWD/pppd/md5.c \
   $$PWD/pppd/multilink.c \
   $$PWD/pppd/options.c \
   $$PWD/pppd/pppcrypt.c \
   $$PWD/pppd/session.c \
   $$PWD/pppd/sha1.c \
   $$PWD/pppd/spinlock.c \
   $$PWD/pppd/srp-entry.c \
   $$PWD/pppd/sys-linux.c \
   $$PWD/pppd/tdb.c \
   $$PWD/pppd/tty.c \
   $$PWD/pppd/upap.c \
   $$PWD/pppd/utils.c \
   $$PWD/pppdump/bsd-comp.c \
   $$PWD/pppdump/deflate.c \
   $$PWD/pppdump/pppdump.c \
   $$PWD/pppdump/zlib.c \
   $$PWD/pppstats/pppstats.c \
   $$PWD/scripts/chatchat/chatchat.c

INCLUDEPATH = \
    $$PWD/common \
    $$PWD/include/linux \
    $$PWD/include/net \
    $$PWD/modules \
    $$PWD/pppd \
    $$PWD/pppd/plugins/pppoatm \
    $$PWD/pppd/plugins/pppol2tp \
    $$PWD/pppd/plugins/radius \
    $$PWD/pppd/plugins/rp-pppoe \
    $$PWD/pppdump

DEFINES = HAVE_MULTILINK

