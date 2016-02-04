action:
    ifeq ($(ARG),)
	    @echo "Tu doit donner un argument dans ARG"
    else
		g++ -g ./knapbb_to_complete.c -o plop.exe
		./plop.exe ./txtfiles/$(ARG)
		glpsol --bfs --model ./glpsol/Mathprog.mod --data ./glpsol/Data.dat > ./glpsol/Res.dat
    endif
