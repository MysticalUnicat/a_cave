SUBDIRS := $(dir $(wildcard */CMakeCache.txt))

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: $(SUBDIRS)
