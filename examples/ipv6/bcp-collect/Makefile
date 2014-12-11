CONTIKI = ../../..
APPS = powertrace collect-view
CONTIKI_PROJECT = bcp-sender
PROJECT_SOURCEFILES += bcp-icmp6.c bcp_collect-common.c

WITH_UIP6=1
UIP_CONF_IPV6=1
CFLAGS+= -DUIP_CONF_IPV6_RPL

ifdef PERIOD
CFLAGS=-DPERIOD=$(PERIOD)
endif

all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include

