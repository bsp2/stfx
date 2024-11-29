
.cpp.o:
	$(CC) $(CPPFLAGS) $(OPTFLAGS_PLUGIN) -c $< -Fo"$@"

.c.o:
	$(CC) $(CPPFLAGS) $(OPTFLAGS_PLUGIN) -c $< -Fo"$@"


.PHONY: bin
bin: $(ALL_OBJ)
	$(LD) $(LDFLAGS_SIZE) -OUT:"$(TARGET).dll" $(ALL_OBJ) $(EXTRALIBS)
	@echo "Build finished at `date +%H:%M`."

.PHONY: install
install:
	$(CP) "$(TARGET).dll" "$(TARGET_DIR)/"

.PHONY: clean
clean:
	$(RM) $(ALL_OBJ) "$(TARGET).dll" "$(TARGET).map" "$(TARGET).lib" "$(TARGET).exp" "$(TARGET).so"
