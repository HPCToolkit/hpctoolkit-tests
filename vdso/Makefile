TARGETS=testvdso

VDSO=[vdso]
VDSO_STRUCT=$(VDSO).hpcstruct

CFLAGS=-g

OFILES=$(VDSO) $(VDSO_STRUCT)

all: $(TARGETS)

%: $.c
	$(CC) $(CFLAGS) -o $@ $<

run: all
	hpcrun ./testvdso
	hpcstruct ./testvdso
	hpcprof -S testvdso.hpcstruct hpctoolkit-testvdso-measurements

clean:
	/bin/rm -rf $(TARGETS) $(OFILES) hpctoolkit-testvdso-* *.hpcstruct
