#!/usr/local/bin/bash

DAT=$1
MAT=$2

# Base run
models/capacity/src/capacity --synch=1 --mat=$MAT --routes=$DAT > base.raw
cat base.raw | grep \\\[ | sort > base.out

# Run it sequential 
#echo "Running Sequential.."
#mpirun -np 1 models/capacity/src/capacity --synch=1 --mat=$MAT --routes=$DAT | grep \\\[ | sort > seq.out
#echo "Runing conservative..."
#mpirun -np 4 models/capacity/src/capacity --synch=2 --mat=$MAT --routes=$DAT | grep \\\[ | sort > con.out
#echo $?

echo "Running optimistic..."
# Run it optimistic, lots of times
for i in {1..1}
    do
        mpirun -np 4 models/capacity/src/capacity --synch=3 --mat=$MAT --routes=$DAT > opt.raw
        # break out if it fails
        if [ "$?" -ne 0 ]
        then
            echo "Failed!"
            break
        fi

        cat opt.raw | grep \\\[ | sort > opt.out
    done
