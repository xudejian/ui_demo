CC = g++

OPT ?=
ifeq "${DEBUG}" "1"
OPT += -g -DDEBUG_TIME -DDEBUG
else
OPT += -O2 -DNDEBUG
endif

CXXFLAGS= -Wall -DLINUX=2 -D_REENTRANT -D_GNU_SOURCE $(OPT)
LDFLAGS= -static
LDLIBS= -luv -lhiredis -lpthread
OBJS=main.o conf.o core.o server.o template.o util.o
LIBO=ui
all	: $(LIBO)

scpSrc= as_ui
scpObj= as_ui.new

$(LIBO) : $(OBJS)
	$(CC) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)
clean:
	@rm -f *.o $(LIBO) *~
cleanall: clean
	@rm -f tags
tags:
	@ctags `find . -name '*.*[h|c|cpp]'`
rz:
	@rz -ayc
scp: $(scpSrc)
	@mv ui ui.new
	scp $(scpObj) a.dc.xdf.cn:~/
