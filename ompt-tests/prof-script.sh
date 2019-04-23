for f in *.hpcrun; do
	echo
	echo
	echo
	echo "profile $f"
	echo
	echo
	hpcprof $f
	hpcviewer hpctoolkit-database/
	rm -rf hpctoolkit-database	
done
