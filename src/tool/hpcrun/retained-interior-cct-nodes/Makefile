EXEC=main
${EXEC}: main.c dereference.o
	gcc -g -O2 -o $@ $^ 

dereference.o: dereference.c
	gcc -c -Ofast $<

run: 
	hpcrun -e cycles -t ./$(EXEC)
	hpcstruct $(EXEC)
	hpcprof -S $(EXEC).hpcstruct hpctoolkit-$(EXEC)-measurements

clean:
	/bin/rm -rf $(EXEC) *.o hpctoolkit-$(EXEC)-measurements hpctoolkit-$(EXEC)-database $(EXEC).hpcstruct

