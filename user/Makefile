SUBDIRS := $(filter-out stdlib, $(wildcard */.))

all: 
	$(MAKE) -C libc
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done