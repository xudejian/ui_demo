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
scp: $(LIBO)
	scp $(LIBO) a.dc.xdf.cn:~/
	ssh a.dc.xdf.cn 'scp ui 10.200.130.197:~/uid/bin/ui.new'
