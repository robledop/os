SUBDIRS := $(filter-out stdlib, $(wildcard */.))

all: 
	$(MAKE) -C stdlib
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done