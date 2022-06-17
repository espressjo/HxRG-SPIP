CC = g++
OBJ = argument_parser.o inst_time.o hxrg_init_config.o \
      uics_base64.o autocheck.o hxrg_get_status.o \
      pheader.o init_handler.o macie_control.o 
TARGET = HxRG-SPIP
CFLAGS =-I./ \
	-I../UICS/lib\
        -I../UICS/src \
	-I$(INITPATH)/lib \
	-I../fits2ramp-utils/lib

LIBS=-lcfitsio -lMACIE -lm -luics -lf2r -lpthread

LDIR=-L../UICS/lib -L../fits2ramp-utils/lib -L$(INITPATH)/lib 

ifndef INITPATH
ipath := /opt/HxRG-SPIP
else
ipath := $(INITPATH)
endif
main.o:main.cpp
	$(CC) -std=c++11 -DINITPATH='"$(ipath)/config"' -o $@ -c $< $(CFLAGS)
%.o: %.cpp %.h
	$(CC) -std=c++11 -o $@ -c $< $(CFLAGS)

all: $(OBJ) main.o 
	$(CC) -std=c++11 -Wl,--disable-new-dtags,-rpath=$(INITPATH)/lib -g -o $(INITPATH)/bin/$(TARGET) $(CFLAGS) $(LDIR) *.o $(LIBS)   

f2r:
	cd ../fits2ramp-utils && make
uics:
	cd ../UICS && make
clean:
	rm -f *.o
	rm -f $(INITPATH)/bin/$(TARGET)
