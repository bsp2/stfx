
#include $(TKS_ROOT)/install_macos.mk

.cpp.o:
	$(CPP) $(CPPFLAGS) $(OPTFLAGS) $(DBGFLAGS) -fPIC -c $< -o $@ 

.c.o:
	$(CC) $(CFLAGS) $(OPTFLAGS) $(DBGFLAGS) -fPIC -c $< -o $@ 


.PHONY: bin
bin: $(ALL_OBJ)
	$(CPP) -dynamiclib -o "$(TARGET).dylib" $(ALL_OBJ) $(EXTRALIBS)
#$(LDFLAGS)
ifneq ($(DEBUG),y)
	$(STRIP) "$(TARGET).dylib"
endif
	@echo "Build finished at `date +%H:%M`."

.PHONY: install
install:
	$(CP) "$(TARGET).dylib" "$(TARGET_DIR)/"

.PHONY: clean
clean:
	$(RM) $(ALL_OBJ) "$(TARGET).dll" "$(TARGET).map" "$(TARGET).lib" "$(TARGET).exp" "$(TARGET).so" "$(TARGET).dylib"
