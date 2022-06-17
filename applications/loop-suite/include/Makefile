$(TARGET).d: $(TARGET).m
	hpcstruct $< 
	hpcprof -o $@ $<

$(TARGET).m: $(TARGET)
	${LAUNCHER} hpcrun $(MEASUREMENT_ARGS) -o $@ ./$<

$(TARGET): $(TARGET).c
	$(COMPILER) $(COMPILER_ARGS) -o $@ $<



clean::
	rm -rf $(TARGET) $(TARGET).m $(TARGET).d
