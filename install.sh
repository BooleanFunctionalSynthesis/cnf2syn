#! /bin/bash

cd dependencies/abc; make; make libabc.a; cd ../../
cd dependencies/dsharp; make; make libdsharp.a; cd ../../
cd dependencies/bfss; make; make libcombfss.a; cd ../../
make
