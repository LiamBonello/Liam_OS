.PHONY: all core core-debug run run-debug check-tools check-x86_64 clean manifest dist structure

all: core

core:
	$(MAKE) -C core

core-debug:
	$(MAKE) -C core debug

run:
	$(MAKE) -C core run

run-debug:
	$(MAKE) -C core run-debug

check-tools:
	$(MAKE) -C core check-tools

check-x86_64:
	./scripts/check-x86_64.sh

clean:
	$(MAKE) -C core clean
	rm -rf dist

manifest:
	find . \
		-path './.git' -prune -o \
		-path './core/build' -prune -o \
		-path './dist' -prune -o \
		-type f -print | sort > SOURCE_MANIFEST.txt

dist: manifest
	./scripts/package-source.sh

structure:
	find . -path './.git' -prune -o -path './core/build' -prune -o -type f -print | sort
