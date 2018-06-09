
# platform
P="chalk"

VERSION=$(shell cat package.json | grep version | grep -o "[0-9][0-9]*\.[0-9][0-9]*")
NAME=$(shell cat package.json | grep '"name":' | head -1 | sed 's/,//g' |sed 's/"//g' | awk '{ print $2 }')

all: build install

build:
	pebble build

config:
	pebble emu-app-config --emulator $(P)

log:
	pebble logs --emulator $(P)

travis_build:
	yes | ~/pebble-dev/${PEBBLE_SDK}/bin/pebble build

install:
	pebble install --emulator $(P)

clean:
	pebble clean

size:
	pebble analyze-size

logs:
	pebble logs --emulator $(P)

.PHONY: all build config log install clean size logs
