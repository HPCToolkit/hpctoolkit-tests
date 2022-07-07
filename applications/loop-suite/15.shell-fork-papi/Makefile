TARGET=loop

COMPILER=gcc
COMPILER_ARGS=-g -O2
MEASUREMENT_ARGS=-e PAPI_TOT_CYC

$(TARGET).d: $(TARGET).m
	hpcstruct $<
	hpcprof -o $@ $<

$(TARGET).m: $(TARGET)
	$(LAUNCHER) hpcrun $(MEASUREMENT_ARGS) -o $(TARGET).m ./shell

$(TARGET): $(TARGET).c
	$(COMPILER) $(COMPILER_ARGS) -o $@ $<

clean:
	rm -rf $(TARGET) $(TARGET).{m,d}

