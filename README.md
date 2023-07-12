This is the implemention of the cnf version of nnf2syn used in the paper:

Tractable Representations for Boolean Functional Synthesis
S. Akshay,  Supratik Chakraborty and Shetal Shah
In  Annals of Mathematics and Artificial Intelligence, 2023

-----
Download the code using git clone 

To make:

You will need to install the following packages if they are not already installed in your system

sudo apt install libreadline-dev libboost-all-dev libgmp-dev build-essential 

To make nnf2syn, we first need to make the libraries it is dependent on. This can be done by running the install.sh script or
alternatively using the following commands:
1. cd dependencies/abc; make; make libabc.a; cd ../../
2. cd dependencies/dsharp; make; make libdsharp.a; cd ../../
3. cd dependencies/bfss; make; make libcombfss.a; cd ../../
4. Finally do a make in the base directory. This will generate the binary nnf2syn in the bin directory

To run:

To run nnf2syn use the following command ${PATH-TO-nnf2syn}/nn2syn {benchmark-name}.qdimacs

If you want the synNNF representation printed to a file please use the option --dumpResult while running the command above.
The synNNF representation is printed in {benchmark-name}.syn.blif file.

To verify the {benchmark-name}.syn.blif file use:
    ${PATH-TO-c2syn}/verify {benchmark-name}.qdimacs {benchmark-name}.syn.blif {benchmark-name}_varstoelim.txt

Please see the script test/run-nnf2syn for more details.

Known issues:

1. The generated synNNF file ({benchmark-name}.syn.blif) can be quite large.  
For large representations,  we are currently unable to verify these files as they are too big for ABC to read them.
