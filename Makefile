.PHONY: all core core-debug run run-debug check-tools check-x86_64 x86_64-info x86_64-kernel x86_64-iso x86_64-run clean manifest dist structure

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

x86_64-info:
	$(MAKE) -C core x86_64-info

x86_64-kernel:
	$(MAKE) -C core x86_64-kernel

x86_64-iso:
	$(MAKE) -C core x86_64-iso

x86_64-run:
	$(MAKE) -C core x86_64-run

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
