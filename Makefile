SUBDIRS= dummywget cloremwebserver
     
.PHONY: subdirs $(SUBDIRS)
	       
subdirs: $(SUBDIRS)
         
$(SUBDIRS):
	$(MAKE) -C $@
