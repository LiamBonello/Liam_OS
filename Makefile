.PHONY: all core core-debug run run-debug check-tools check-x86_64 x86_64-info x86_64-check-tools x86_64-iso-check-tools x86_64-kernel x86_64-iso x86_64-run legacy-i386 legacy-i386-debug legacy-i386-run clean manifest dist structure

all: x86_64-iso

core: x86_64-iso

core-debug:
	$(MAKE) -C core x86_64-kernel

run: x86_64-run

run-debug: x86_64-run

check-tools: x86_64-iso-check-tools

check-x86_64:
	./scripts/check-x86_64.sh

x86_64-info:
	$(MAKE) -C core x86_64-info

x86_64-check-tools:
	$(MAKE) -C core x86_64-check-tools

x86_64-iso-check-tools:
	$(MAKE) -C core x86_64-iso-check-tools

x86_64-kernel:
	$(MAKE) -C core x86_64-kernel

x86_64-iso:
	$(MAKE) -C core x86_64-iso

x86_64-run:
	$(MAKE) -C core x86_64-run

legacy-i386:
	$(MAKE) -C core ARCH=i386

legacy-i386-debug:
	$(MAKE) -C core ARCH=i386 debug

legacy-i386-run:
	$(MAKE) -C core ARCH=i386 run

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
