
PYTHON_CFLAGS	:= $(shell pkg-config --cflags python3)

CCOPT	= -Wall -g -O3
CFLAGS	= -D_GNU_SOURCE -fPIC $(CCOPT) $(PYTHON_CFLAGS) -mavx2

OBJS	= extension.o \
	  domain.o \
	  set.o \
	  member.o \
	  transform.o \
	  bitvec.o

all:	fastsets.so

test: ;

fastsets.so: $(OBJS)
	$(CC) --shared -o $@ $(OBJS)

distclean clean::
	rm -f *.o *.so

distclean::
	;

install: fastsets.so
	install -d $(DESTDIR)$(PYTHON_INSTDIR)
	install -m555 fastsets.so $(DESTDIR)$(PYTHON_INSTDIR)
