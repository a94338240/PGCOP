CFLAGS = -I. -fPIC \
	       -g -Wall -c
HY_OBJS = pg_cop_hypervisor.o

LIB_OBJS = pg_cop_util.o \
					 pg_cop_modules.o \
					 pg_cop_hooks.o \
	         pg_cop_rodata_strings.o

MOD_TRANSCEIVER_OBJS = pg_cop_mod_transceiver.o

MOD_SOCKET_OBJS = pg_cop_mod_socket.o

LIBS = -L. -lPGCOP -ldl -lpthread

all: libPGCOP.so \
		 pg-cop-hypervisor \
     modules/mod_socket.so \
     modules/mod_transceiver.so

pg-cop-hypervisor: $(HY_OBJS)
	gcc -o $@ $+ $(LIBS)

libPGCOP.so: $(LIB_OBJS)
	gcc --shared -o $@ $+

modules/mod_socket.so: $(MOD_SOCKET_OBJS)
	gcc --shared -o $@ $+

modules/mod_transceiver.so: $(MOD_TRANSCEIVER_OBJS)
	gcc --shared -o $@ $+

%.o: %.c %.h
	gcc $(CFLAGS) -o $@ $<

clean:
	rm -rf *.o *~ .*~ pg-cop-hypervisor *.so
