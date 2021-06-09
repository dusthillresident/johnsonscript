#!/bin/bash

echo > TestTranscompilerOut.txt; for i in debugtests/*; do ./run.sh "$i" >> TestTranscompilerOut.txt; done
echo > TestInterpreterOut.txt; for i in debugtests/*; do ./run.sh "$i" >> TestInterpreterOut.txt; done
echo "Tests passed OK if nothing is printed between these lines:"
echo "-----------"
diff TestInterpreterOut.txt TestTranscompilerOut.txt
echo "-----------"
rm TestTranscompilerOut.txt TestInterpreterOut.txt
