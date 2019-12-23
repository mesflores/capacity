#!/usr/local/bin/bash

if [  $# -le 1 ] 
then 
    echo -e "\nUsage:\n test_run.sh <routes> <mat>\n" 
    exit 1
fi 

# Make a dir for nodes to dump data to
mkdir -p out_dat

MAXDIFF=4
MAXTS=400000

ROUTES=$1
MAT=$2


# Clean up leftovers
rm -f base.out seq.out con.out opt.raw opt.out

# Base run
#models/capacity/src/capacity --synch=1 --mat=$MAT --routes=$ROUTES --end=1382400> base.raw
#cat base.raw | grep \\\[ | sort > base.out

## Run it sequential 
echo "Running Sequential.."
mpirun -np 1 models/capacity/src/capacity --synch=1 --mat=$MAT --routes=$ROUTES --end=$MAXTS | grep \\\[ | grep -v "Total KPs" | sort > seq.out
echo "Running Conservative..."
mpirun -np 4 models/capacity/src/capacity --synch=2 --mat=$MAT --routes=$ROUTES --end=$MAXTS | grep \\\[ | grep -v "Total KPs" | sort > con.out

# Compare them
diff seq.out con.out

if [ $? -eq 1 ]
then
    echo "Sequential and Conservative runs did not match!"
    exit 1
fi

echo "Running Optimistic..."
# Run it optimistic, lots of times
for i in {1..1}
    do
        mpirun -np 4 models/capacity/src/capacity --synch=3 --mat=$MAT --routes=$ROUTES --end=$MAXTS --extramem=1024 > opt.raw
        # break out if it fails
        if [ "$?" -ne 0 ]
        then
            echo "Failed!"
            exit 1
            break
        fi
        cat opt.raw | grep \\\[ | grep -v "Total KPs" | sort > opt.out

        # Compare it it to the seq/con
        diff con.out opt.out
        if [ $? -eq 1 ]
        then
            echo "Optimistic and Conservative mode did not match!"
            exit 1
        fi
    done
