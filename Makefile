CFLAGS = -I. -fPIC \
	       -g -Wall -c
HY_OBJS = pg_cop_hypervisor.o

LIB_OBJS = pg_cop_service.o \
				   pg_cop_util.o \
					 pg_cop_modules.o \
	         pg_cop_rodata_strings.o

LIBS = -L. -lPGCOP -ldl -lpthread

all: libPGCOP.so pg-cop-hypervisor

pg-cop-hypervisor: $(HY_OBJS)
	gcc -o $@ $+ $(LIBS) -lc

libPGCOP.so: $(LIB_OBJS)
	gcc --shared -o $@ $+ -lc

%.o: %.c %.h
	gcc $(CFLAGS) -o $@ $<

clean:
	rm -rf *.o *~ pg-cop_hypervisor
