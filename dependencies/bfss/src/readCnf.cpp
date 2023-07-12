#include "readCnf.h"

using namespace std;

//#define VERILOG_HEADER "// Generated using findDep.cpp \n"
//#define MAX_DEP_SIZE 5

//Currently for an unary universal variable - a wire definition is generated.
//This should be changed if it doesn't meet the specifications. - SS

static int iter = 0;
vector<vector<int> > allClauses;
vector<bool> tseitinClauses;
vector<bool> prevTseitinClauses;
vector<int> varsX;
vector<int> origVarsY;
vector<int> varsY;
//vector<int> unates;
int numVars, numClauses;
int numA, numE;
int origNumVars;
vector<int> tseitinVars;
vector<int> constVars;
vector<int> opVars; //tseitinVars + constVars + opVars = numE;

vector<set<int> > existsAsPos;
vector<set<int> > existsAsNeg;
vector<map<int,int> > posImplies;
vector<map<int,int> > negImplies;

map<int, vector<int> > depAND;
map<int, vector<int> > depOR;
map<int, vector<int> > depXOR;
map<int, vector<int> > depCl; //Store the clause numbers which result in a dependency. Required if cycles are to be broken
set<int> depCONST;
vector<bool> depFound;

//set<int> unaLit;
queue<int> litToPropagate;

void readCnf0 (char * qdFileName);
void readQdimacsFile(char * qdFileName);
void writeOutput (char* baseFileName);
int preprocess( set<int> &unateVarNums );
void print(vector<int> & v);
void print(set<int> & v);
void findDependencies();
void cleanDependencies();
bool findDepAND(int y);
bool findDepOR(int y);
bool findDepXOR(int y);
void findLitToProp(set <int> );
void propagateLiteral(int lit);
void writeVerilogFile(string fname, string moduleName);
int addFlatClausesToVerilog(ofstream&ofs, int start, int end, int&nextVar);
void writeVariableFile(string fname);
void writeDependenceFile(string fname);
string vecToVerilogLine(vector<int> &v, string op);
void writeNonTseitinToQdimacsFile(string fname);
static inline void addToImpliesMap(map<int,int>&m, int lit, int clauseNum);
static inline void processBinaryClause(int clauseNum);
static inline void setConst(int lit);
static inline map<int,int>& getImpliesMap(int lit);
bool checkForCycles();
bool DFS_checkForCycles(vector<set<int> >& graph, int node, vector<int>& DFS_startTime, vector<int>& DFS_endTime, int& DFS_currTime);
void labelVars ( ); //Label Vars as Tseitin, const or output
void reduceDependencySizes();

inline string varNumToName(int v) {
	return ("v_"+to_string(v));
}
inline string extraNumToName(int v) {
	return ("x_"+to_string(v));
}

#ifdef MAIN
int main(int argc, char * argv[]) {
    char * qdFileName;
    if ( argc < 2 ) {
        cout << "Wrong number of command-line arguments. Usage: readCnf qdimacs_filename " << endl;
        return 1;
    }
    qdFileName = argv[1];

    //readCnf0(qdFileName);
	readQdimacsFile(qdFileName);
    set <int> unateVarNums;
    preprocess (unateVarNums);
    writeOutput (qdFileName);
}
#endif
void readCnf0 ( char * qdFileName)
{
/*
	string baseFileName(qdFileName);
	baseFileName = baseFileName.substr(baseFileName.find_last_of("/") + 1);  //Get the file name;
	baseFileName.erase(baseFileName.find (".qdimacs"), string::npos); //This contains the code for the raw file name;
	cout << "BaseName:     " << baseFileName << endl;

	string varFileName = baseFileName + "_var.txt";
	string aigFileName = baseFileName + ".v" ;
	string depFileName = baseFileName + "_dep.txt" ;
	string qdmFileName = baseFileName + ".qdimacs.noUnary" ;
*/
	readQdimacsFile(qdFileName);


	cout << "Finished readQdimacsFile" << endl;
}

int preprocess( set<int>& unateVarNums )
{

        bool firstIter = true;
        cout << " # clauses" << allClauses.size() << endl;
    //Re-iterating
        std::fill (tseitinClauses.begin(), tseitinClauses.end(), 0);
        std::fill (depFound.begin(), depFound.end(), 0);
            //recompute dependencies if unates are found 
    //    tseitinClauses.clear();
        depAND.clear();
      //  depFound.clear();
        depOR.clear();
        depXOR.clear();


        
        if (unateVarNums.size() > 0)
        {
            firstIter = false;
            if (varsY.size() > numE)
            {
                //varsY.clear();
                varsY.resize(numE);     //In case extra vars have been generated
            }

            numVars = origNumVars;
            //depFound.clear();
            depFound.resize(numVars+1, false);
            for (auto & it : depCONST)
            {
	            litToPropagate.push(it);
                depFound[abs(it)] = true;
            }

        }
        cout << "preprocess: sizeofvarsY " << varsY.size() << endl;
        findLitToProp(unateVarNums);
        cout << "Finished findLitToProp" << endl;

        while(!litToPropagate.empty()) {
            int toProp = litToPropagate.front();
            litToPropagate.pop();
            propagateLiteral(toProp);
        }
        cout << "Finished propagateLiteral" << endl;


        findDependencies();
        cout << "Finished findDependencies" << endl;

        if (! firstIter)
        {
            cleanDependencies(); // an earlier tseitin var be become a const because of unate propagation. Remove such tseitin dependencies
            cout << "Cleaned dependencies " << endl;
        }

        assert(!checkForCycles());
        cout << "Finished checkForCycles" << endl;

        reduceDependencySizes();
        cout << "Finished reduceDependencySizes" << endl;

        int numNonTseitin = 0;
        int numTseitin = 0;
        for(int i = 0; i < allClauses.size(); i++) {
            if(!tseitinClauses[i]) {
                numNonTseitin++;
                // cout<<i<<": \t"; print(allClauses[i]);
            }
            else
                if (prevTseitinClauses[i] != tseitinClauses[i]) //New Tseitin's discovered
                        numTseitin ++;
             //    cout<<i<<": Tseitin Clause; \t"; print(allClauses[i]);
        }
        cout << "depCONST.size(): " << depCONST.size() << endl;
        cout << "numNonTseitin:   " << numNonTseitin << endl;
	    for(auto it:varsY) {
		    if(depFound[it])
            {
		    	tseitinVars.push_back(it); 
                //cout << it << " is a tseitinVar " << endl;
	        }
        }
        if ( numTseitin > 0)
        {
            for(int i = 0; i < allClauses.size(); i++) {
                prevTseitinClauses[i] = tseitinClauses[i];
                //tseitinClauses[i] = false;
           }
        }
       // cout << " Number of Tseitins  in  iteration " << iter << " = " << numTseitin << endl;
       cout << "num of vars Y " << varsY.size() << endl;
        return (numTseitin != 0);
   //     tseitinClauses.clear
  } 

void writeOutput (char* qdFileName) 
//void writeOutput (char* qdFileName, bool all) 
{
	string baseFileName(qdFileName);
	baseFileName = baseFileName.substr(baseFileName.find_last_of("/") + 1);  //Get the file name;
	baseFileName.erase(baseFileName.find (".qdimacs"), string::npos); //This contains the code for the raw file name;
	cout << "BaseName:     " << baseFileName << endl;
	string varFileName = baseFileName + "_varstoelim.txt";
	string aigFileName = baseFileName + ".v" ;
   
	string depFileName = baseFileName + "_dep.txt" ;
	string qdmFileName = baseFileName + "_" +to_string(iter++) + ".qdimacs" ;

   // if (all)
    //{
        writeNonTseitinToQdimacsFile(qdmFileName);
        writeDependenceFile(depFileName);
   // }
    writeVerilogFile(aigFileName, baseFileName);
    writeVariableFile(varFileName);
}

void readQdimacsFile(char * qdFileName) {
    char C[100], c;
    int tmpVar;

	FILE *qdFPtr = fopen (qdFileName, "r");

	// Number of variables and clauses
	fscanf (qdFPtr, "%c", &c);
	fscanf (qdFPtr, "%s", C);
	while (strcmp (C, "cnf") != 0)
		fscanf (qdFPtr, "%s", C);
	fscanf(qdFPtr, "%d %d", &numVars, &numClauses); // read first line p cnf
	cout << "numVars:       " <<  numVars << endl;
	cout << "NumClauses:   " << numClauses << endl;

	// Vars X
	fscanf (qdFPtr, "%c", &c);
	while (c != 'a')
		fscanf (qdFPtr, "%c", &c);

	fscanf(qdFPtr, "%d", &tmpVar);
	while (tmpVar !=0) {
		varsX.push_back(tmpVar);
		fscanf(qdFPtr, "%d", &tmpVar);
	}
	cout << "varsX.size(): " << varsX.size() << endl;
	assert (numVars > varsX.size());

	// Vars Y (to elim)
	fscanf (qdFPtr, "%c", &c);

	while (c != 'e')
		fscanf (qdFPtr, "%c", &c);
	fscanf(qdFPtr, "%d", &tmpVar);
	while (tmpVar !=0) {
		varsY.push_back(tmpVar);
        origVarsY.push_back(tmpVar);
		fscanf(qdFPtr, "%d", &tmpVar);
	}
	cout << "varsY.size(): " << varsY.size() << endl;
	assert (numVars > varsY.size());
    origNumVars = numVars;

	// Update numVars = maxVar
	int maxVar = 0;
	for(auto it:varsX)
		maxVar = max(maxVar,it);
	for(auto it:varsY)
		maxVar = max(maxVar,it);
	cout << "maxVar:       " << maxVar << endl;
	if(maxVar < numVars) {
		cout << "Setting numVars = " << maxVar << endl;
		numVars = maxVar;
	}

    numA = varsX.size();
    numE = varsY.size();
	existsAsPos.resize(numVars+1);
	existsAsNeg.resize(numVars+1);
	posImplies.resize(numVars+1, map<int,int>());
	negImplies.resize(numVars+1, map<int,int>());
	depFound.resize(numVars+1, false);

	// Clauses
	for (int i = 0; i < numClauses ; i++) {
		vector<int> tempClause;
		fscanf(qdFPtr, "%d", &tmpVar);
		while (tmpVar != 0) {
            if (std::find (tempClause.begin(), tempClause.end(), tmpVar) != tempClause.end()) //The variable has occured earlier. Ignore
            {
			    fscanf(qdFPtr, "%d", &tmpVar);
                continue;
            }
            if (std::find(tempClause.begin (), tempClause.end(), -tmpVar) != tempClause.end()) //var and neg present - so this evaluates to 1. Ignore clause.
                break;

			tempClause.push_back(tmpVar);
            if (tmpVar > maxVar)
                cerr << " Variable not defined " << tmpVar <<endl;
			else if(tmpVar > 0) { // pos
				existsAsPos[tmpVar].insert(i);
			}
			else { // neg
				existsAsNeg[-tmpVar].insert(i);
			}
		fscanf(qdFPtr, "%d", &tmpVar);
		}

		if(!tempClause.empty()) {
			allClauses.push_back(tempClause);
			tseitinClauses.push_back(false);
			prevTseitinClauses.push_back(false);
		}
        else
            cout << "Clause " << i << " is empty " << endl;

		if(tempClause.size() == 2) { // populate ___Implies
			processBinaryClause(allClauses.size()-1);
		}
	}

	fclose (qdFPtr);
    //For debugging - Shetal
   // for (int i = 0; i < posImplies.size(); i++) {
    //    for (auto it : posImplies[i])
     //   cout << i << " " << it.first << ": " << it.second << endl; 

    //}
    //for (int i = 0; i < negImplies.size(); i++)
   // {
    //    for (auto it : negImplies[i])
     //   cout << i << " " << it.first << ": " << it.second << endl; 

    //}
}

void cleanDependencies()
{
	for(auto&it: depAND) {
		int var = abs(it.first); 
        if (depCONST.find(var) != depCONST.end() || depCONST.find(-var) != depCONST.end ()) //The var is a const
            depAND.erase(var);
    }
	for(auto&it: depOR) {
		int var = abs(it.first); 
        if (depCONST.find(var) != depCONST.end() || depCONST.find(-var) != depCONST.end ()) //The var is a const
            depOR.erase(var);
    }
	for(auto&it: depXOR) {
		int var = abs(it.first); 
        if (depCONST.find(var) != depCONST.end() || depCONST.find(-var) != depCONST.end ()) //The var is a const
            depXOR.erase(var);
    }
}

void findDependencies() {
	// Find AND dependencies
    cout << "findDependencies: Size of varsY " << varsY.size() << endl;
	for(auto y:boost::adaptors::reverse(varsY)) {
		depFound[y] = depFound[y] or findDepAND(y) or findDepOR(y);
	}
	for(auto y:boost::adaptors::reverse(varsY)) {
		depFound[y] = depFound[y] or findDepXOR(y);
	}
}

bool findDepAND(int y) {
	// Check for y = AND(...)
    //cout << "y : " << y  << endl;
	for(auto clauseNum: existsAsPos[y]) { // Checking all possible clauses

       // cout << " clauseNum " << clauseNum << endl;
		if(tseitinClauses[clauseNum] == true)
			continue;

		bool gotcha = true;
		for(auto v2: allClauses[clauseNum]) {
			if(tseitinClauses[clauseNum] == true) //Required - SS?
				continue;
			if(v2!=y and posImplies[y].find(-v2)==posImplies[y].end()) {
				gotcha = false;
				break;
			}
		}
		if(gotcha) {
			// Print it
			string dep = "AND(";
			for(auto v2: allClauses[clauseNum]) {
				if(tseitinClauses[clauseNum] == true) //Required - SS?
					continue;
				if(v2!=y) {
					dep = dep + to_string(-v2) + ", ";
				}
			}
			dep = dep.substr(0,dep.length()-2) + ")";
		//	cout << "DEP" << y << " = " << dep << endl;

			// Found Dependency
			// assert(depAND.find(y) == depAND.end());
			depAND[y] = vector<int>();
            depCl[y] = vector<int>();
			for(auto v2: allClauses[clauseNum]) {
				if(tseitinClauses[clauseNum] == true)
					continue;
				if(v2!=y) {
					depAND[y].push_back(-v2);
					tseitinClauses[posImplies[y][-v2]] = true; // tseitinClauses=true
                    depCl[y].push_back(posImplies[y][-v2]);
				}
			}
			tseitinClauses[clauseNum] = true; // tseitinClauses=true
            depCl[y].push_back(clauseNum);
			return true;
		}
	}
	return false;
}

bool findDepOR(int y) {
	// Check for -y = AND(...)
	for(auto clauseNum: existsAsNeg[y]) { // Checking all possible clauses

		if(tseitinClauses[clauseNum] == true)
			continue;

		bool gotcha = true;
		for(auto v2: allClauses[clauseNum]) {
			if(tseitinClauses[clauseNum] == true)
				continue;
			if(v2!=-y and negImplies[y].find(-v2)==negImplies[y].end()) {
				gotcha = false;
				break;
			}
		}
		if(gotcha) {
			// Print it
			string dep = "OR(";
			for(auto v2: allClauses[clauseNum]) {
				if(tseitinClauses[clauseNum] == true)
					continue;
				if(v2!=-y) {
					dep = dep + to_string(v2) + ", ";
				}
			}
			dep = dep.substr(0,dep.length()-2) + ")";
	//		cout << "DEP" << y << " = " << dep << endl;

			// Found Dependency
			// assert(depOR.find(y) == depOR.end());
			depOR[y] = vector<int>();
			depCl[y] = vector<int>();
			for(auto v2: allClauses[clauseNum]) {
				if(tseitinClauses[clauseNum] == true)
					continue;
				if(v2!=-y) {
					depOR[y].push_back(v2);
				 	tseitinClauses[negImplies[y][-v2]] = true; // tseitinClauses=true
                    depCl[y].push_back(negImplies[y][-v2]);
				}
			}
			tseitinClauses[clauseNum] = true; // tseitinClauses=true
            depCl[y].push_back(clauseNum);
			return true;
		}
	}
	return false;
}

bool findDepXOR(int y) {
	// Check for y = XOR(...)
	for(auto clauseNum: existsAsPos[y]) { // Checking all possible clauses

		auto & cl1 = allClauses[clauseNum];
		if(tseitinClauses[clauseNum] == true or cl1.size()!=3)
			continue;

		int iTemp = 0;
		vector<int> otherVars(2);
		if(cl1[iTemp]==y)
			iTemp++;
		otherVars[0] = cl1[iTemp];
		iTemp++;
		if(cl1[iTemp]==y)
			iTemp++;
		otherVars[1] = cl1[iTemp];
		iTemp++;

		for(auto clauseNum2: existsAsPos[y]) { // Checking all possible clauses

			auto & cl2 = allClauses[clauseNum2];
			if(tseitinClauses[clauseNum2] == true or cl2.size()!=3
				or clauseNum==clauseNum2)
				continue;

			if(find(cl2.begin(),cl2.end(),-otherVars[0])!=cl2.end()
				and find(cl2.begin(),cl2.end(),-otherVars[1])!=cl2.end()) {

				int clause1foundAt = -1;
				int clause2foundAt = -1;
				for(auto clauseNum3: existsAsNeg[y]) { // Checking all possible clauses
					auto & cl3 = allClauses[clauseNum3];
					if(tseitinClauses[clauseNum3] == true or cl3.size()!=3
						or clauseNum3==clauseNum2 or clauseNum3==clauseNum)
						continue;

					if(find(cl3.begin(),cl3.end(),otherVars[0])!=cl3.end()
						and find(cl3.begin(),cl3.end(),-otherVars[1])!=cl3.end()) {
						clause1foundAt = clauseNum3;
					}
					if(find(cl3.begin(),cl3.end(),-otherVars[0])!=cl3.end()
						and find(cl3.begin(),cl3.end(),otherVars[1])!=cl3.end()) {
						clause2foundAt = clauseNum3;
					}
				}
				if(clause1foundAt != -1 and clause2foundAt != -1) {
					// Print it
					string dep = "XOR(" + to_string(otherVars[0]) + ", " + to_string(-otherVars[1]) + ")";
				//	cout << "DEP" << y << " = " << dep << endl;

					// Found Dependency
					vector<int> res(2);
			        depCl[y] = vector<int>();
					res[0] = otherVars[0];
					res[1] = -otherVars[1];
					depXOR[y] = res;

                    
					tseitinClauses[clauseNum] = true;	// tseitinClauses=true
					tseitinClauses[clauseNum2] = true;	// tseitinClauses=true
					tseitinClauses[clause1foundAt] = true;	// tseitinClauses=true
					tseitinClauses[clause2foundAt] = true;	// tseitinClauses=true
                    depCl[y].push_back(clauseNum);
                    depCl[y].push_back(clauseNum2);
                    depCl[y].push_back(clause1foundAt);
                    depCl[y].push_back(clause2foundAt);

					return true;
				}
			}
		}
	}
	return false;
}

void print(vector<int> & v) {
	for(auto it:v)
		cout << it << " ";
	cout << endl;
}

void print(map<int, int> & v) {
	for(auto it:v)
		cout << "(" << it.first << "," << it.second << ") ";
	cout << endl;
}

void print(set<int> & v) {
	for(auto it:v)
		cout << it << " ";
	cout << endl;
}

void findLitToProp(set<int> unateVarNums) {

	for(int clauseNum = 0; clauseNum < allClauses.size(); clauseNum++) {
       // cout << "clause " << clauseNum << " has size " << clause.size () << endl;
        if (tseitinClauses[clauseNum] == true) //Useful if the function is called repeatedly
            continue;

		auto & clause = allClauses[clauseNum];
		if(clause.size() == 2) {
			int v0 = clause[0];
			int v1 = clause[1];

			// -v1 -> v0, check if v1 -> v0, then v0_isConst
			// -v0 -> v1, check if v0 -> v1, then v1_isConst

			map<int,int>& v0_map = getImpliesMap(v0);
			map<int,int>& v1_map = getImpliesMap(v1);

			if(v1_map.find(v0) != v1_map.end()) { // v0_isConst
				setConst(v0);
				tseitinClauses[clauseNum] = true;	// tseitinClauses=true
				tseitinClauses[v1_map[v0]] = true;	// tseitinClauses=true
			}
			if(v0_map.find(v1) != v0_map.end()) { // v1_isConst
				setConst(v1);
				tseitinClauses[clauseNum] = true;	// tseitinClauses=true
				tseitinClauses[v0_map[v1]] = true;	// tseitinClauses=true
			}
		}
		else if(clause.size() == 1 ) {
			setConst(clause[0]);
			if (std::find (varsX.begin(), varsX.end(), abs (clause [0])) != varsX.end())
				cout << " A universally quantified variable is unary " << clause[0] << endl;
			tseitinClauses[clauseNum] = true; // Unary tseitinClauses=true
		}
	}

	// findPureLiterals
    int var;
	for (int i = 0; i < numE; i++) { //Extra nodes are added to varsY so iterate only till numY

        var = varsY[i];
		if(existsAsPos[var].empty() && depCONST.find(var) != depCONST.end()) {
			// set var = 0
		//	cout << "PureNeg " << var << endl;
			setConst(-var);
		}
		else if(existsAsNeg[var].empty()&& depCONST.find(var) != depCONST.end()) {
			// set var = 1
		//	cout << "PurePos" << var << endl;
			setConst(var);
		}
	}
    for (auto var: unateVarNums)
    {
            cout << "Setting unate " << var << " as const " << endl;
            setConst(var);
    }
}

void propagateLiteral(int lit) {
	int var = abs(lit);
//	cout << " Propogating literal " << lit << endl;
	bool pos = lit>0;
	for(auto clauseNum:existsAsPos[var]) {
		if(tseitinClauses[clauseNum]) //Should this be commented out?
            	continue;
		if(pos) {
			tseitinClauses[clauseNum] = true;	// tseitinClauses=true
		}
		else{
			// Remove var from allClauses
			auto it = find(allClauses[clauseNum].begin(), allClauses[clauseNum].end(), var);
			assert(it != allClauses[clauseNum].end());
			*it = allClauses[clauseNum].back();
			allClauses[clauseNum].resize(allClauses[clauseNum].size()-1);

			assert(!allClauses[clauseNum].empty()); // CNF formula is unsat
			if(allClauses[clauseNum].size() == 1) {
				setConst(allClauses[clauseNum][0]);
				tseitinClauses[clauseNum] = true;	// Unary tseitinClauses=true
			}
			else if(allClauses[clauseNum].size() == 2) {
				processBinaryClause(clauseNum);
			}
		}
	}

	for(auto clauseNum:existsAsNeg[var]) {
		if(tseitinClauses[clauseNum])
			continue;
		if(!pos) {
			tseitinClauses[clauseNum] = true;	// Unary tseitinClauses=true
		}
		else{
			// Remove var from allClauses

			auto it = find(allClauses[clauseNum].begin(), allClauses[clauseNum].end(), -var);
			assert(it != allClauses[clauseNum].end());
			*it = allClauses[clauseNum].back();
			allClauses[clauseNum].resize(allClauses[clauseNum].size()-1);

			assert(!allClauses[clauseNum].empty()); // CNF formula is unsat
			if(allClauses[clauseNum].size() == 1) {
				setConst(allClauses[clauseNum][0]);
				tseitinClauses[clauseNum] = true;	// Unary tseitinClauses=true
			}
			else if(allClauses[clauseNum].size() == 2) {
				processBinaryClause(clauseNum);
			}
		}
	}

	if(pos) {
		existsAsNeg[var].clear();
	}
	else{
		existsAsPos[var].clear();
	}
}

void writeVerilogFile(string fname, string moduleName) {
	ofstream ofs (fname, ofstream::out);
	ofs << VERILOG_HEADER;
	ofs << "module " << moduleName << " ";
	ofs << "(";
	for(auto it:varsX) {
		if(!depFound[it])
		ofs << varNumToName(it) << ", ";
	}
	for(auto it:varsY) {
		if(!depFound[it])
			ofs << varNumToName(it) << ", ";
	}
	ofs << "o_1);" << endl;

	// Input/Output/Wire
	for(auto it:varsX) {
		//assert(!depFound[it]); //This is required - in verilog one cannot assign
                               // a value to an input
		if (!depFound [it])	//This is being done temporarily
		//	cout << " A Universal Variable Found UnSAT! " << it << endl;
			ofs << "input " << varNumToName(it) << ";\n";
	}
	for(auto it:varsY) {
		if(!depFound[it])
			ofs << "input " << varNumToName(it) << ";\n";
	}
	ofs << "output o_1;\n";
	for(auto it:varsX) {	//Temporary fix - the qdimacs generation should be fixed!
		if(depFound[it])
			ofs << "wire " << varNumToName(it) << ";\n";
	}
	for(auto it:varsY) {
		if(depFound[it])
        {
			ofs << "wire " << varNumToName(it) << ";\n";
            tseitinVars.push_back(it);
        } 
	}

	// Extra wires required for non-tseitin clauses
	int numNonTseitin = 0;
	for(int i = 0; i < allClauses.size(); i++) {
		if(!tseitinClauses[i]) {
			numNonTseitin++;
		}
	}
	assert(numNonTseitin > 0);
	for(int i = 1; i<2*numNonTseitin; i++) {
		ofs << "wire " << extraNumToName(i) << ";\n";
	}


	// Assign Dependencies
	// constants
	for(auto it: depCONST) {
		int var = abs(it);
		bool pos = it>0;
		ofs << "assign " << varNumToName(var) << " = " << (pos?1:0) << ";\n";
	}
	// and
	for(auto&it: depAND) {
		int var = abs(it.first); 
		bool pos = it.first>0;
		string res = vecToVerilogLine(it.second,"&");
		ofs << "assign " << varNumToName(it.first) << " = " << res << ";\n";
	}
	// or
	for(auto&it: depOR) {
		int var = abs(it.first);
		bool pos = it.first>0;
		string res = vecToVerilogLine(it.second,"|");
		ofs << "assign " << varNumToName(it.first) << " = " << res << ";\n";
	}
	// xor
	for(auto&it: depXOR) {
		int var = abs(it.first);
		bool pos = it.first>0;
		string res = vecToVerilogLine(it.second,"^");
		ofs << "assign " << varNumToName(it.first) << " = " << res << ";\n";
	}

	// Assign Non-tseitin dependencies
	int eNum = 1;
	for(int i = 0; i < allClauses.size(); i++) {
		if(!tseitinClauses[i]) {
			string res = vecToVerilogLine(allClauses[i],"|");
			ofs << "assign " << extraNumToName(eNum) << " = " << res << ";\n";
			eNum++;
		}
	}
	assert(eNum == numNonTseitin+1);

	// Conjunct all Extra Variables (x_1 .. x_numNonTseitin)
	int finalVar =  addFlatClausesToVerilog(ofs, 1, numNonTseitin, eNum);
	assert(finalVar <= 2*numNonTseitin);

	ofs << "assign o_1 = " << extraNumToName(finalVar) << ";\n";
	ofs << "endmodule\n";
	ofs.close();
}

int addFlatClausesToVerilog(ofstream&ofs, int start, int end, int&nextVar) {
	assert(start<=end);
	if(start==end)
		return start;

	int mid = (start+end+1)/2 - 1;
	int v1 = addFlatClausesToVerilog(ofs, start, mid, nextVar);
	int v2 = addFlatClausesToVerilog(ofs, mid+1, end, nextVar);

	string res = extraNumToName(v1) + " & " + extraNumToName(v2);
	ofs << "assign " << extraNumToName(nextVar) << " = " << res << ";\n";
	nextVar++;
	return nextVar-1;
}

void writeVariableFile(string fname) {
	ofstream ofs (fname, ofstream::out);

	cout << "# Y : " << varsY.size() << endl;
    bool nonT=false;
	for(auto it:varsY) {
		if(!depFound[it])
		{
			ofs << varNumToName(it) << "\n";
            nonT=true;
			//cout << "Written " <<  varNumToName(it) << " to " << fname << endl;
		}

	//	else
	//		cout << varNumToName(it) << "is a Tseitin variable" << endl;
	}
     if (! nonT)
            cout << "No. variable file written. All variables accounted for in preprocessing! " << endl;

	ofs.close();
}

void writeDependenceFile(string fname) {
	ofstream ofs (fname, ofstream::out);
	// Assign Dependencies
	// constants
	for(auto it: depCONST) {
		int var = abs(it);
		bool pos = it>0;
		ofs << varNumToName(var) << " = " << (pos?1:0) << ";\n";
		cout << varNumToName(var) << " = " << (pos?1:0) << ";\n";
	}
	// and
	for(auto&it: depAND) {
		int var = abs(it.first);
		bool pos = it.first>0;
		string res = vecToVerilogLine(it.second,"&");
		ofs << varNumToName(it.first) << " = " << res << ";\n";
	}
	// or
	for(auto&it: depOR) {
		int var = abs(it.first);
		bool pos = it.first>0;
		string res = vecToVerilogLine(it.second,"|");
		ofs << varNumToName(it.first) << " = " << res << ";\n";
	}
	// xor
	for(auto&it: depXOR) {
		int var = abs(it.first);
		bool pos = it.first>0;
		string res = vecToVerilogLine(it.second,"^");
		ofs << varNumToName(it.first) << " = " << res << ";\n";
	}
	ofs.close();
}

string vecToVerilogLine(vector<int> &v, string op) {
	string res;
	for(auto el:v) {
		if(el > 0)
			res = res + varNumToName(abs(el)) + " " + op + " ";
		else
			res = res + "~" + varNumToName(abs(el)) + " " + op + " ";
	}
	return res.substr(0,res.length()-2-op.length());
}

void writeNonTseitinToQdimacsFile(string fname) {
	ofstream ofs (fname, ofstream::out);

	// Extra wires required for non-tseitin clauses
	int numNonTseitin = 0;
	for(int i = 0; i < allClauses.size(); i++) {
		if(!tseitinClauses[i]) {
			numNonTseitin++;
		}
	}

	ofs << VERILOG_HEADER;
	ofs << "p cnf " << numVars << " " << numNonTseitin+depCONST.size() << endl;

	ofs << "a ";
	for(auto it:varsX) {
		if(!depFound[it])
		ofs << it << " ";
	}
	ofs << 0 << endl;

	ofs << "e ";
	for(auto it:varsY) {
	//	if(!depFound[it])
			ofs << it << " ";
	}
	ofs << 0 << endl;

	// constants
	for(auto it: depCONST) {
		ofs << it << " 0 \n";
	}

	// Non-tseitin clauses
	for(int i = 0; i < allClauses.size(); i++) {
		if(!tseitinClauses[i]) {
			for(auto el: allClauses[i]) {
				ofs << el << " ";
			}
			ofs << "0\n";
		}
	}

	ofs.close();
}

static inline map<int,int>& getImpliesMap(int lit) {
	return (lit>0)?posImplies[lit]:negImplies[-lit];
}

static inline void addToImpliesMap(map<int,int>&m, int lit, int clauseNum) {
	auto it = m.find(lit);
	if(it == m.end()) {
		m[lit] = clauseNum;
        //cout << "added " << clauseNum << " to " << lit << endl;
	}
	else if(it->second != clauseNum) { // set clauseNum is redundant
		tseitinClauses[clauseNum] = true;
	}
}

static inline void setConst(int lit) {
    if (depCONST.find (lit) == depCONST.end())
    {
	    depFound[abs(lit)] = true;
	    depCONST.insert(lit);
	    litToPropagate.push(lit);
    }
}

static inline void processBinaryClause(int clauseNum) {
	vector<int>&clause  = allClauses[clauseNum];
	assert(clause.size() == 2);
	int v0 = clause[0];
	int v1 = clause[1];

	map<int,int>& v0_map = getImpliesMap(-v0);
	map<int,int>& v1_map = getImpliesMap(-v1);

	addToImpliesMap(v0_map, v1, clauseNum);
	addToImpliesMap(v1_map, v0, clauseNum);
    
}

bool checkForCycles() {
	vector<set<int> > graph(numVars+1);

	// Create Graph
	for(auto it: depCONST) {
		int var = abs(it);
		graph[var].insert(0);
	}
	for(auto&it: depAND) {
		int var = abs(it.first);
		for(auto&it2:it.second)
			graph[var].insert(abs(it2));
	}
	for(auto&it: depOR) {
		int var = abs(it.first);
		for(auto&it2:it.second)
			graph[var].insert(abs(it2));
	}
	for(auto&it: depXOR) {
		int var = abs(it.first);
		for(auto&it2:it.second)
			graph[var].insert(abs(it2));
	}

	vector<int> DFS_startTime(numVars+1,-1);
	vector<int> DFS_endTime(numVars+1,-1);
	int DFS_currTime = 0;

	for (int i = 0; i < numVars + 1; ++i) {
		if(DFS_startTime[i] == -1) {
			if(DFS_checkForCycles(graph, i, DFS_startTime, DFS_endTime, DFS_currTime)) {
				cerr << i << endl;
				return true;
			}
		}
	}
	return false;
}

bool DFS_checkForCycles(vector<set<int> >& graph, int node, vector<int>& DFS_startTime, vector<int>& DFS_endTime, int& DFS_currTime) {
	if(DFS_startTime[node] != -1) {
		if(DFS_endTime[node] == -1) {
			// Back Edge
			cout << "Found dependency cycle: ";
			return true;
		}
		else {
			// Cross Edge
			return false;
		}
	}
	// Forward Edge
	DFS_startTime[node] = DFS_currTime++;
	
//If a cycle is present, break it by removing a dependency
	for(auto it:graph[node]) {
		if(DFS_checkForCycles(graph, it, DFS_startTime, DFS_endTime, DFS_currTime)) {
			cout << it << " ";
		if (depAND.find(node) != depAND.end()) {
			cout << "Breaking AND dep " << endl;
			depAND.erase(node);	
            for (int i = 0; i < depCl[node].size(); i++) //Breaking the cycle so the clauses are no longer tseitin
            {
                tseitinClauses[depCl[node][i]] = false;
                cout << "Clause " << depCl[node][i] << " containing " << node << " no longer tseitin " << endl;

            }
		}
		else 
		if (depXOR.find(node) != depXOR.end()) {
			cout << "Breaking XOR dep " << endl;
			depXOR.erase(node);	
            for (int i = 0; i < depCl[node].size(); i++) //Breaking the cycle so the clauses are no longer tseitin
            {
                tseitinClauses[depCl[node][i]] = false;
                cout << "Clause " << depCl[node][i] << " containing " << node << " no longer tseitin " << endl;
            }
		}
		else 
		if (depOR.find(node) != depOR.end()) {
			cout << "Breaking OR dep " << endl;
			depOR.erase(node);	
            for (int i = 0; i < depCl[node].size(); i++) //Breaking the cycle so the clauses are no longer tseitin
            {
                tseitinClauses[depCl[node][i]] = false;
                cout << "Clause " << depCl[node][i] << " containing " << node << " no longer tseitin " << endl;
            }
		}
		}
	}

	DFS_endTime[node] = DFS_currTime++;
	return false;
}

void reduceDependencySizes() {
    
    int end = MAX_DEP_SIZE;
	for(auto&it: depAND) {
      //  if (it.second.size() > MAX_DEP_SIZE ) //Added by Shetal to remove additional variables being generated
       //     end = it.second.size(); 
		while(it.second.size() > MAX_DEP_SIZE) {
			int start = 0;
			int end = MAX_DEP_SIZE;
			vector<int> newV;
			while(start<it.second.size()) {
				numVars++;
                //origNumVars++;
				varsY.push_back(numVars);
		//		assert(depFound.size() == numVars);
				depFound.push_back(true);

				depAND[numVars] = vector<int>(it.second.begin()+start,it.second.begin()+end);
				newV.push_back(numVars);

				start = end;
				end = min(start + MAX_DEP_SIZE, (int)it.second.size());
			}
			it.second = newV;
		}
	}
	for(auto&it: depOR) {
		while(it.second.size() > MAX_DEP_SIZE) {
			int start = 0;
			int end = MAX_DEP_SIZE;
			vector<int> newV;
			while(start<it.second.size()) {
				numVars++;
                //origNumVars++;
				varsY.push_back(numVars);
				//assert(depFound.size() == numVars);
				depFound.push_back(true);

				depOR[numVars] = vector<int>(it.second.begin()+start,it.second.begin()+end);
				newV.push_back(numVars);

				start = end;
				end = min(start + MAX_DEP_SIZE, (int)it.second.size());
			}
			it.second = newV;
		}
	}
	for(auto&it: depXOR) {
		while(it.second.size() > MAX_DEP_SIZE) {
			int start = 0;
			int end = MAX_DEP_SIZE;
			vector<int> newV;
			while(start<it.second.size()) {
				numVars++;
                //origNumVars++;
				varsY.push_back(numVars);
			//	assert(depFound.size() == numVars);
				depFound.push_back(true);

				depXOR[numVars] = vector<int>(it.second.begin()+start,it.second.begin()+end);
				newV.push_back(numVars);

				start = end;
				end = min(start + MAX_DEP_SIZE, (int)it.second.size());
			}
			it.second = newV;
		}
	}
}

void labelVars ( ) //Label Vars as Tseitin, const or output
{
    int var;
    for (int i = 0; i < numE; i++)
    {
        var = varsY[i];
		if(!depFound[var])
        {
			opVars.push_back(var);
        }
        else
        if (depCONST.find(varsY[i]) != depCONST.end())
        {
            constVars.push_back(var);

        }
        else
            tseitinVars.push_back(var);
    }
}

