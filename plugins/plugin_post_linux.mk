
#include $(TKS_ROOT)/install_linux.mk

.cpp.o:
	$(CPP) $(CPPFLAGS) $(OPTFLAGS) $(DBGFLAGS) -fPIC -c $< -o $@ 

.c.o:
	$(CC) $(CFLAGS) $(OPTFLAGS) $(DBGFLAGS) -fPIC -c $< -o $@ 


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
	$(RM) $(ALL_OBJ) "$(TARGET).dll" "$(TARGET).map" "$(TARGET).lib" "$(TARGET).exp" "$(TARGET).so"

