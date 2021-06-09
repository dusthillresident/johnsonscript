#!/bin/bash

echo > TestTranscompilerOut.txt; for i in debugtests/*; do ./run.sh "$i" >> TestTranscompilerOut.txt; done
echo > TestInterpreterOut.txt; for i in debugtests/*; do ./run.sh "$i" >> TestInterpreterOut.txt; done
diff TestInterpreterOut.txt TestTranscompilerOut.txt
rm TestTranscompilerOut.txt TestInterpreterOut.txt

