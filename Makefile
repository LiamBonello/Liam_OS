.PHONY: all core core-debug run run-debug check-tools clean dist structure

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

clean:
	$(MAKE) -C core clean
	rm -rf dist

dist:
	mkdir -p dist
	git archive --format=zip --output=dist/Liam_OS_source.zip HEAD

structure:
	find . -path './.git' -prune -o -path './core/build' -prune -o -type f -print | sort
