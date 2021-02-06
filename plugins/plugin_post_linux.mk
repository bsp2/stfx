
include $(TKS_ROOT)/install_msvc.mk

.cpp.o:
	$(CPP) $(CPPFLAGS) $(OPTFLAGS_PLUGIN) $(DBGFLAGS) -c $< -o $@ 

.c.o:
	$(CC) $(CFLAGS) $(OPTFLAGS_PLUGIN) $(DBGFLAGS) -c $< -o $@ 


.PHONY: bin
bin: $(ALL_OBJ)
	$(CPP) -shared -o "$(TARGET).so" -Wl,-soname,$(TARGET).so $(ALL_OBJ) $(EXTRALIBS)
ifneq ($(DEBUG),y)
	$(STRIP) "$(TARGET).so"
endif
	@echo "Build finished at `date +%H:%M`."

.PHONY: install
install:
	$(CP) "$(TARGET).so" "$(TARGET_DIR)/"

.PHONY: clean
clean:
	$(RM) $(ALL_OBJ)
