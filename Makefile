SUBDIRS := $(dir $(wildcard */CMakeCache.txt))

all: $(SUBDIRS)

$(SUBDIRS):
	@ $(MAKE) -C $@ -s -j

.PHONY: $(SUBDIRS)
