library:
	cd ./etc && python install.py --setlib
uics:
	cd ./UICS && make
f2r:
	cd ./fits2ramp-utils && make
CONF:
	cd ./etc && python install.py --config
SRC:
	cd ./src && make all
all: library uics f2r SRC CONF
	@echo "done!"
clean:
	cd ./UICS && make clean
	cd ./fits2ramp-utils && make clean
	cd ./src && make clean
	cd ./etc && python install.py --clean
install:
	cp ./mcd/* $(INITPATH)/mcd/
