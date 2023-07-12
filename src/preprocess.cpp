#include "helper.h"
#include "nnf.h"
#include "readCnf.h"
#include "synNNF.h"

#define ABC_NAMESPACE NS_ABC
#define MAX_CLAUSE_SIZE = 100;


using namespace std;

extern vector<int> varsX;
extern vector<int> varsY;
extern vector<int> origVarsY;
extern vector<int> tseitinVars;
//extern vector<bool> tseitinClauses;
extern map<int, vector<int> > depAND;
extern map<int, vector<int> > depOR;
extern map<int, vector<int> > depXOR;
extern set<int> depCONST;
extern vector<bool> useR1AsSkolem;
extern int origNumVars;

extern void readQdimacsFile(char * );
extern int preprocess(set<int> & );
extern void writeOutput(char *);
extern Aig_Obj_t* Aig_SubstituteVec(Aig_Man_t* pMan, Aig_Obj_t* initAig, vector<int> varIdVec,
	vector<Aig_Obj_t*>& funcVec);

extern pair<Cnf_Dat_t*,bool> buildErrorFormula(sat_solver* pSat, Aig_Man_t* SAig,
	vector<vector<int> > &r0, vector<vector<int> > &r1, vector<int> &r0Andr1Vars) ;
extern void printAig(Aig_Man_t* pMan);
extern void print (vector <int> &);
extern vector<Aig_Obj_t *> Aig_SupportVec(Aig_Man_t* pMan, Aig_Obj_t* root);
//extern void writeOutput(char *, bool);

Abc_Ntk_t* getNtk(string pFileName);
void getUnates(vector<int> & unate, Aig_Man_t* &FAig);
int checkSemInd (Aig_Man_t* FAig, vector <int>& indX, vector<int>& indY);
int checkSynInd (Aig_Man_t* pMan, vector <int>& indX, vector<int>& indY) ;
//void createFuncT( Aig_Man_t *, map<int, Aig_Obj_t*> &, map<int, int> &);
//Aig_Obj_t* createFuncT( Aig_Man_t* p, map <int, Aig_Obj_t*> & newYCi, map<int,int> &Ymap,  map<int, vector<int> >& graph, int node, map<int,int>& nodeOp ) ;
//void createFuncT(map<int, int>& Ymap, int offset, vector <vector<int> >& funcTClauses);
//void CreateDFSGraph(Aig_Man_t* p, map <int, Aig_Obj_t*> & newYCi, map<int,int> &Ymap) ;
//void prop5 (Aig_Man_t * p);

extern vector<int> varsX;
extern vector<int> varsY;

vector<int> varsXF;
vector<int> varsYF; // to be eliminated
int numOrigInputs, numX, numY;

//Required due to inclusion of helper.h
vector<string> varsNameX, varsNameY;
Abc_Frame_t* pAbc = NULL;
sat_solver* m_pSat = NULL;
Cnf_Dat_t* m_FCnf = NULL;
lit m_f = 0;
sat_solver* pSat = NULL ;
Cnf_Dat_t* pCnf  = NULL;
//lit l_p = 0;
vector<int> varsXS;
vector<int> varsYS; // to be eliminated
//---------------------
//Required for bfssPhaseOne
double sat_solving_time = 0;
double verify_sat_solving_time = 0;
double reverse_sub_time = 0;
chrono_steady_time helper_time_measure_start = TIME_NOW;
chrono_steady_time main_time_start = TIME_NOW;
map<string, int> name2IdF;
map<int, string> id2NameF;
map <int, Aig_Obj_t* > qd2AigMap; //Maps a var in qdimacs to the corresponding Object in AIG.
map <int,  Aig_Obj_t*> funcT;

vector<int> unate;

synSolver s;
bool dumpResult = false;

bool bfssPhaseOne(Abc_Ntk_t* FNtk, Aig_Man_t* FAig, vector<int>& unate, int numTotUnates, bool useBDD, string varFileName, string baseFileName);
void saveSkolemFns(Aig_Man_t* SAig, vector<int>&,  string outfname) ;

int main(int argc, char * argv[]) {
	char * qdFileName;
    if ( argc < 2 ) {
       cout << "Wrong number of command-line arguments. Usage: nnf2syn qdimacs_filename --dumpResult" << endl;
        
        return 1;
    }
    qdFileName = argv[1];

	string baseFileName(qdFileName);
	baseFileName = baseFileName.substr(baseFileName.find_last_of("/") + 1);  //Get the file name;
	baseFileName.erase(baseFileName.find (".qdimacs"), string::npos); //This contains the code for the raw file name;
	cout << "BaseName:     " << baseFileName << endl;

    if(argc > 2)
    {
        string arg = argv[2];
        if (arg.find("dumpResult" ) != string::npos)
            dumpResult = true; 
        else
            cout << "Argument 2 is incorrect. Results will not be dumped." << endl;
    }
   // string noUnaryFile = baseFileName + ".qdimacs.noUnary" ;
	string aigFileName = baseFileName + ".v" ;
	string varFileName = baseFileName + "_varstoelim.txt";
	string tseitinFileName = baseFileName + "_dep.txt";
//Preprocessing round 0 - Find unary variables and simplify and also  << varfind Tseitin variables.


    numOrigInputs = 0;
    numX = 0;
    numY = 0;
//Read the Qdimacs file
    readQdimacsFile(qdFileName); 

    set<int> unateVarNums;
    bool with_preprocess = true;
    if (with_preprocess)
    {

        preprocess (unateVarNums); //Do unate check even if no tseitin's found
        writeOutput (qdFileName); //Do not write the preprocessed qdimacs file.
        
        bool moreUnates = true;

        vector<string> varOrder;
        Abc_Ntk_t* FNtk;
        Aig_Man_t* FAig = NULL;
        while (moreUnates)
        {
          //  cout << " varsX " << endl;
           // print (varsX);
            //cout << " varsY " << endl;
           // print (origVarsY);

            FNtk = getNtk(aigFileName);

            if (FNtk == NULL)
                cout << " Aig File " << aigFileName << " not read " << endl;

    //Abc retains the names of the ci's and co's but changes the names of the internal nodes.
            if (FAig != NULL)
            {
                Aig_ManStop (FAig);
            }
            FAig = Abc_NtkToDar(FNtk, 0, 0);
            assert (FAig != NULL);

            Aig_ManCleanup(FAig);


            if (FAig == NULL)
                cout << " In while loop : Manager NULL " << endl;

            varsXF.clear();
            varsYF.clear();
            name2IdF.clear();
            id2NameF.clear();
            varOrder.clear();


            cout << "populateVars" << endl;
            populateVars(FNtk, varFileName, varOrder, varsXF, varsYF, name2IdF, id2NameF);

            numX = varsXF.size();
            numY = varsYF.size();

            cout<<"numX: " << numX << " numY: " << numY << endl;

            unate.clear();
            unate.resize(numY, -1); //Check if unates need to be resized each time or once is enough

            getUnates(unate, FAig);

            unateVarNums.clear();

            for(int index=0; index < numY; index++) {
            
                string varName = id2NameF[varsYF[index]];
                int varNum =  stoi(varName.substr(2));

                if(unate[index]==1) { 
                    unateVarNums.insert(varNum);
                //	cout<<"unate: "<<varNum  << " varName: " << varName <<endl;
                    
                 //   ofs << varNum << " 0 " << endl;
                }

                else if(unate[index]==0){		
                    unateVarNums.insert(-varNum);
                //ofs << -varNum << " 0 " << endl;
                 //   cout<<"unate: "<<(-varNum)<<endl;
                }
            }

            if (unateVarNums.size() == 0 )
            {
                moreUnates = false;
                writeOutput (qdFileName); //THe unates are not reflected in the verilog file until they are preprocessed.
                cout << "End of preprocessing. ";
                break; 
            }
            else if( unateVarNums.size() == numY)
            {
                    cout << "End of preprocessing. ALL variables UNATE!" << endl;
                    cout << endl;
                    preprocess (unateVarNums);
                    writeOutput (qdFileName); //process the unates and simplify the qdimacs file.
                    //s.printSynNNFAllTseitinsAndUnates();
                    //cout << "depCONSt : "  ;
                    //for (auto &it : depCONST)
                     //   cout << it << " " ;
                    //cout << endl;
                  //  Abc_NtkStop (FNtk);
                    //Aig_ManStop (FAig);


                    assert (allClauses.size() <= numClauses);
                    //varsXF.clear();
                    //varsYF.clear();
                    //name2IdF.clear();
                    //id2NameF.clear();
                    //bfssPhaseOne (FNtk, FAig, unate, unateVarNums.size(), false, varFileName, baseFileName);
                    Aig_ManStop (FAig); //Need to return the SynNNF form here - TODO
                    Abc_Stop();
                    if (dumpResult)
                        s.CreateSynNNF(allClauses, tseitinClauses, varsX, origVarsY, origNumVars, tseitinVars, baseFileName, depCONST, depAND, depOR, depXOR, true, dumpResult);
                    return 0;
            }
            else
            {
                //Abc_NtkStop (FNtk);
                //Aig_ManStop (FAig);
                bool cont = preprocess (unateVarNums); //Unates discovered - 
                writeOutput (qdFileName); 
                //Read the ntk once again to take care of unates passed to readCnf.
                
                FNtk = getNtk(aigFileName);

                if (FNtk == NULL)
                    cout << " Aig File " << aigFileName << " not read " << endl;

        //Abc retains the names of the ci's and co's but changes the names of the internal nodes.
                if (FAig != NULL)
                {
                    Aig_ManStop (FAig);
                }
                FAig = Abc_NtkToDar(FNtk, 0, 0);
                assert (FAig != NULL);

                Aig_ManCleanup(FAig);


                if (FAig == NULL)
                    cout << " In while loop : Manager NULL " << endl;

                varsXF.clear();
                varsYF.clear();
                name2IdF.clear();
                id2NameF.clear();
                varOrder.clear();


                cout << "populateVars" << endl;
                populateVars(FNtk, varFileName, varOrder, varsXF, varsYF, name2IdF, id2NameF);

                if (! cont) //No more tseitins discovered; 
                 break;
            }
        }

        cout << "In Preprocess.cpp : NumX = " << varsX.size() << " numY = " << varsY.size() << endl;
        //exit (1);
        assert (FAig != NULL); 
     
        //printAig (FAig);
        vector<int> indX(numX);
        vector<int> indY(numY);
        //Collect leaves in the suport)
        int status;

        if (FAig == NULL)
            cout << "Aig Manager NULL. Check " << endl;

        cout << " Checking for Syntatic Independence " << endl;
        status = checkSynInd(FAig, indX, indY); //
       // if (status == -1)
        //    status = checkSemInd (FAig, indX, indY);

        if (status == 0)
        {
            cout << " F syntactically/semantically independent of X " << endl;
            //check the algorithm what to return. 
            return 0;
        }
        else if (status == 1)
        {
            cout << " F syntactically/semantically independent of Y " << endl;
            //check the algorithm what to return 
            return 0;
        }
/*  --Commenting out sematic independence checks and substitution
    //Subsititute Semantic Independence
	vector<int> varIdVec;
	vector<Aig_Obj_t*> funcVec;

    for (int i = 0; i < numX ; i++)
    {
        if (indX[i] == 1)
        {
			varIdVec.push_back(varsXF[i]);
			funcVec.push_back(Aig_ManConst1(FAig));
            cout <<" Setting " << varsXF[i] << " to 1 " << endl;
            //Set XF[i] to 1/0
        }

    }

	for (int i = 0; i < numY; ++i) {
		if(indY[i] == 1) {
			varIdVec.push_back(varsYF[i]);
			funcVec.push_back(Aig_ManConst1(FAig));
            cout <<" Setting " << varsYF[i] << " to 1 " << endl;
		}
	}
	Aig_Obj_t* pAigObj = Aig_SubstituteVec(FAig, Aig_ManCo(FAig,0), varIdVec, funcVec);
	
//	cout << "Support of pAigObj " << Aig_SupportSize(pMan, Aig_Regular(pAigObj));

	Aig_ObjPatchFanin0(FAig, Aig_ManCo(FAig,0), pAigObj);
	Aig_ManCleanup(FAig);
	Aig_Man_t* tempAig = FAig;
	cout << " Duplicating AIG " << endl;

	FAig = Aig_ManDupSimple(FAig);
    //printAig (pMan);
	Aig_ManStop(tempAig);
    //printAig (FAig);
*/
        varsXF.clear();
        varsYF.clear();
        name2IdF.clear();
        id2NameF.clear();
        //Check for error formula
        bool useBDD = false;
        //if  (!bfssPhaseOne (FNtk, FAig, unate, unateVarNums.size(), useBDD, varFileName, baseFileName))
        //{ 
            Aig_ManStop (FAig); //Need to return the SynNNF form here - TODO
            cout << " Calling the synNNF Solver " << endl;
            //cout << " varsX " << endl;
            //print (varsX);
            cout << " varsY " << endl;
            print (varsY);
        //Add Unates/Unit Clauses; May lead to some duplication.
            assert (allClauses.size() <= numClauses);
            s.CreateSynNNF(allClauses, tseitinClauses, varsX, varsY, origNumVars, tseitinVars, baseFileName, depCONST, depAND, depOR, depXOR, false, dumpResult);
       // }
        //else
       // {
            //printSkolemFunctions(FAig);
        //    Aig_ManStop (FAig); //Need to return the SynNNF form here - TODO
        //}
    }
    else
          s.CreateSynNNF(allClauses, tseitinClauses, varsX, varsY, origNumVars, tseitinVars, baseFileName, depCONST, depAND, depOR, depXOR, false, dumpResult);

    //Abc_NtkStop (FNtk);
	Abc_Stop();
    return 0;
//write Unate variables to the qdimacs file
}
//Compute syntactic and semantic unates
void getUnates(vector<int> & unate, Aig_Man_t* &FAig) {
        options.unateTimeout = 2000;
        unate.clear();
		unate.resize(numY, -1);

		int n, numSynUnates = 0;
		while((n = checkUnateSyntacticAll(FAig, unate)) > 0) {
				substituteUnates(FAig, unate);
				numSynUnates += n;
			}

		cout << "Syntactic Unates Done" << endl;
		int numSemUnates = 0;

		numSemUnates = checkUnateSemAll (FAig, unate);
		substituteUnates(FAig, unate);

		cout << "Total Syntactic Unates: " << numSynUnates << endl;
		cout << "Total Semantic  Unates: " << numSemUnates << endl;
}

Abc_Ntk_t*  getNtk(string pFileName) {
	string cmd, initCmd;

	initCmd = "balance; rewrite -l; refactor -l; balance; rewrite -l; \
						rewrite -lz; balance; refactor -lz; rewrite -lz; balance";

	pAbc = Abc_FrameGetGlobalFrame();

	cmd = "read " + pFileName;
    cout << cmd << endl;
	if (CommandExecute(pAbc, cmd)) { // Read the AIG
        cerr<< "Could not read " << pFileName << endl;
		return NULL;
	}
	cmd = initCmd;
    cout << cmd << endl;
	if (CommandExecute(pAbc, cmd)) { // Simplify
        cerr<< "Could not simplify " << pFileName << endl;
		return NULL;
	}

	Abc_Ntk_t* pNtk =  Abc_FrameReadNtk(pAbc);
	// Aig_Man_t* pAig = Abc_NtkToDar(pNtk, 0, 0);
	return pNtk;
}


//Check Syntactic Independence 
//Return 0 means independent of X
//Return 1 means independent of Y
//Return -1 means not independent
//It is  inefficient - should be improved.
int checkSynInd (Aig_Man_t* pMan, vector <int>& indX, vector<int>& indY) 
{
    vector <Aig_Obj_t*> supportVec = Aig_SupportVec(pMan, Aig_ManCo(pMan, 0));

    //cout << " Obtained support  - in Syn Ind check " << endl; 
    //cout << " Size " << supportVec.size() << endl;

    int i;
    bool ind = true;
    for (i = 0; i < numX ; i++)
    {
        
        if (find (supportVec.begin(), supportVec.end(), Aig_ManObj(pMan, varsXF[i])) != supportVec.end())
        {
            indX[i] = -1;
            ind = false;
        }
        else
            indX[i] = 1;

    }
    if (ind) return 0; //Syntactically independent of X

    ind = true;
    
    for (i = 0; i < numY ; i++)
    {
        if (find (supportVec.begin(), supportVec.end(), Aig_ManObj(pMan, varsYF[i])) != supportVec.end())
        {
            indY[i] = -1;
            ind = false;
        }
        else
            indY[i] = 1;
    }
    if (ind) return 1; //Syntactically independent of Y

    return -1;
    
}
//Function which checks for semantic independence of X and Y

//Return 0 means independent of X
//Return 1 means independent of Y
//Return -1 means not independent
//Copied from helper.cpp and modified appropriately
int checkSemInd (Aig_Man_t* FAig, vector <int>& indX, vector<int>& indY)
{

//	auto unate_start = std::chrono::steady_clock::now();
//	auto unate_end = std::chrono::steady_clock::now();
//	auto unate_run_time = std::chrono::duration_cast<std::chrono::microseconds>(unate_end - unate_start).count()/1000000.0;

	cout << " Preparing for semantic independence checks " << endl;
	pSat = sat_solver_new();
	Cnf_Dat_t* SCnf = Cnf_Derive(FAig, Aig_ManCoNum(FAig));
	addCnfToSolver(pSat, SCnf);
    //Cnf_DataPrint(SCnf, 1);
	int numCnfVars = SCnf->nVars;
	Cnf_Dat_t* SCnf_copy = Cnf_DataDup(SCnf);
 	Cnf_DataLift (SCnf_copy, numCnfVars);	
	addCnfToSolver(pSat, SCnf_copy);
	int status, numUnate, totalNumUnate = 0;
//	assert(unate.size()==numY);
//Equate the X's and Y's. 
 //	int x_iCnf, y_iCnf; 
	lit Lits[3];
	int cont_XVars[numX]; //control Variables
    int retVal = -1;
	for(int i = 0; i < numX; ++i) {
    //cout << "i = " << i << " indX " << indX[i] << endl;
        if (indX[i] == -1) //It is not syntactically independent
        //Equate the X'es of the two CNFs in the presence of control variables. If control variable c1 
        // is true, than the equality does not hold. If it is false, the variables are forced equal.
        {   
            cont_XVars[i] = sat_solver_addvar (pSat);   //
            Lits[0] = toLitCond( SCnf->pVarNums[ varsXF[i]], 0 );
            Lits[1] = toLitCond( SCnf_copy->pVarNums[ varsXF[i]], 1 );
            Lits[2] = Abc_Var2Lit (cont_XVars[i], 0);
        //	cout << i  << " : Adding clause " <<  Lits [0] << " " << Lits [1] << " " << Lits[2] << endl;
                if ( !sat_solver_addclause( pSat, Lits, Lits+3 ) )
                        assert( 0 );
            Lits[0] = toLitCond( SCnf->pVarNums[ varsXF[i]], 1 );
            Lits[1] = toLitCond( SCnf_copy->pVarNums[ varsXF[i]], 0 );
            Lits[2] = Abc_Var2Lit (cont_XVars[i], 0);
	   	 //   cout << i << " : Adding clause " <<  Lits [0] << "  " << Lits [1] << "  " << Lits[2] << endl;
                if ( !sat_solver_addclause( pSat, Lits, Lits+3 ) )
                        assert( 0 );
        }
	}

	int cont_YVars[numY]; //control Variables
	for(int i = 0; i < numY; ++i) {
    //cout << "i = " << i << " indY " << indY[i] << endl;
        if (indY[i] == -1) //It is present in the support. Add equality clauses for the Y variables
        {
            cont_YVars[i] = sat_solver_addvar (pSat);
            Lits[0] = toLitCond( SCnf->pVarNums[ varsYF[i]], 0 );
            Lits[1] = toLitCond( SCnf_copy->pVarNums[ varsYF[i]], 1 );
            Lits[2] = Abc_Var2Lit (cont_YVars[i], 0);
    	//	cout << " Adding clause for " << i << " "  <<  Lits [0] << " " << Lits [1] << " " << Lits [2] << endl;
                if ( !sat_solver_addclause( pSat, Lits, Lits+3 ) )
                        assert( 0 );
            Lits[0] = toLitCond( SCnf->pVarNums[ varsYF[i]], 1 );
            Lits[1] = toLitCond( SCnf_copy->pVarNums[ varsYF[i]], 0 );
            Lits[2] = Abc_Var2Lit (cont_YVars[i], 0);
    	//	cout << " Adding clause for " << i << " "  <<  Lits [0] << " " << Lits [1] << " " << Lits [2] << endl;
                if ( !sat_solver_addclause( pSat, Lits, Lits+3 ) )
                        assert( 0 );
         }
	}
	
    

	//cout << "NumCnfVars " <<  numCnfVars << endl;
	//cout << "SCnfVars " <<  SCnf->nVars << endl;
	int coID = getCnfCoVarNum(SCnf, FAig, 0);
	
	Aig_Obj_t * pCo = Aig_ManCo(FAig, 0);
	assert (coID == SCnf->pVarNums[pCo->Id]);

//Add the bi-implication \neg (F <=> F')
    lit biImp[2];
	biImp[0] = toLitCond( SCnf->pVarNums[pCo->Id],0);	
	biImp[1] = toLitCond(SCnf_copy->pVarNums[pCo->Id],0); 
    //cout << " Adding clause 1 for biImp " <<  biImp [0] << " " << biImp[1]  << endl;
    if ( !sat_solver_addclause( pSat, biImp, biImp+2 ) )
                  assert( 0 );
	biImp[0] = toLitCond( SCnf->pVarNums[pCo->Id],1);	
	biImp[1] = toLitCond(SCnf_copy->pVarNums[pCo->Id],1); 
    //cout << " Adding clause 2 for biImp " <<  biImp [0] << " " << biImp[1]  << endl;
    if ( !sat_solver_addclause( pSat, biImp, biImp+2 ) )
                  assert( 0 );


	lit LA[numX+ numY];
    int asIndex = 0;
    int numIndX = 0;
   //Check for semantic independence for each X. Break if even one is not independent 
	for (int i = 0; i < numX; ++i)
	{
          if (indX[i] == 1) //already independent of X
          {
              numIndX ++;
              continue;
          }
          asIndex = 0;
                
		  for (int j = 0; j < numX; j++)
		  {
         //       cout << "j = " << j << " " << indX[j] << endl;
                if (indX[j] == -1)
                { 
                    if (j == i)
                    {
                  //      cout << " adding X : j =  " << j << " i = " << i << Abc_Var2Lit (cont_XVars[j], 0)  << " to assumptions " << endl;
                        LA[asIndex++] = Abc_Var2Lit (cont_XVars[j], 0); // 
                    }
                    else
                    {
                 //       cout << " adding X : j =  " << j << " i = " << i << Abc_Var2Lit (cont_XVars[j], 1)  << " to assumptions " << endl;
                        LA[asIndex++] = Abc_Var2Lit (cont_XVars[j], 1);
                    }
                } 
		  }

		  for (int j = 0; j < numY; j++)
		  {
                if (indY[j] == -1)
                {
				     LA[asIndex++] = Abc_Var2Lit (cont_YVars[j], 1);
            //            cout << " adding Y : j =  " << j << Abc_Var2Lit (cont_YVars[j], 1)  << " to assumptions " << endl;
                }
		  }

				// Check if semantically independent
			//LA[0] = toLitCond(SCnf->pVarNums[varsYF[i]], 1); //check the 0 and 1's
			//LA[1] = toLitCond(SCnf_copy->pVarNums[varsYF[i]], 0);
				//cout << "Printing assumptions for pos unate : " << LA [0] << " " << LA [1] << " " << LA [2] << " " << LA [3] << endl;
			status = sat_solver_solve(pSat, LA, LA+asIndex, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0);
           
				//cout << "Status of sat solver " << status << endl;
			if (status == l_False) {
				indX[i] = 1;
				cout << "Var x" << i << " (" << varsXF[i] << ") is semantically independent" << endl;
				addVarToSolver(pSat, cont_XVars[i], 1); //Should I add 0 or 1?? Do I need to add anything?
				numIndX++;
                string varName = id2NameF[varsXF[i]];
                int varNum =  stoi(varName.substr(2));
                depCONST.insert (varNum);
			}
            else
            {
                if (status == l_True)
                {
				    //cout << "Var x" << i << " (" << varsXF[i] << ") is not semantically independent" << endl;
                    //break;

                }
            }
		}
        if (numIndX == numX)
        {
		    cout << "F semantically independent of X " << endl;
            retVal = 0;
        }

	//	unate_end = std::chrono::steady_clock::now();
	//	unate_run_time = std::chrono::duration_cast<std::chrono::microseconds>(unate_end - main_time_start).count()/1000000.0;
	//	if( unate_run_time >= options.unateTimeout) {
	//		cout << "checkUnateSemanticAll Timed Out. Timeout = " << options.unateTimeout << endl;
	//		break;
	//	}
    //Checking for semantic independence of Y
    if (retVal == -1)
    {
        int numIndY = 0;

        for (int i = 0; i < numY; ++i)
        {

              if (indY[i] == 1) //already independent of Y
              {
                  numIndY ++;
                  continue;
              }
                    
              asIndex = 0;
              for (int j = 0; j < numY; j++)
              {
                    if (indY[j] == -1)
                    {
                        if (j == i)
                            LA[asIndex++] = Abc_Var2Lit (cont_YVars[j], 0);
                        else
                            LA[asIndex++] = Abc_Var2Lit (cont_YVars[j], 1);
                        //cout << " adding Y : j =  " << j << " " << Abc_Var2Lit (cont_YVars[j], 1)  << " to assumptions " << endl;
                    }
              }
             // cout << " numX = " << numX  << " " << indX.size() << endl ;

              for (int j = 0; j < numX; j++)
              {
                    if (indX[j] == -1)
                    {
                        LA[asIndex++] = Abc_Var2Lit (cont_XVars[j], 1);
                        //cout << " adding X : j =  " << j << " i = " << i <<  " " << Abc_Var2Lit (cont_XVars[j], 1)  << " to assumptions " << endl;
                     }
              }

               // Sat_SolverWriteDimacs (pSat, "semcheck.qdimacs", LA, LA+asIndex, 0);
                status = sat_solver_solve(pSat, LA, LA+asIndex, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0);
                    //cout << "Status of sat solver " << status << endl;
                if (status == l_False) {
                    indY[i] = 1;
                    cout << "Var y" << i << " (" << varsYF[i] << ") is semantically independent" << endl;
                    addVarToSolver(pSat, cont_YVars[i], 1); //Should I add 0 or 1?? Do I need to add anything?
                    numIndY++;
                    string varName = id2NameF[varsXF[i]];
                    int varNum =  stoi(varName.substr(2));
                    depCONST.insert(varNum); //Add variable as constant
                }
                else
                {
                    if (status == l_True)
                    {
                       // cout << "Var y" << i << " (" << varsYF[i] << ") is not semantically independent" << endl;
                      //  break;

                    }
                }
            }

         if (numIndY == numY) {
            cout << "F semantically independent of Y " << endl;
                retVal =  1;
         }
     }
	sat_solver_delete(pSat);
	Cnf_DataFree(SCnf);
	Cnf_DataFree(SCnf_copy);
    //indX.resize(0);
    //indY.resize(0);

	return retVal; 

}
//Copied relevants parts from bfss.cpp and helper.cpp... this is vanilla bfssPhaseOne

bool bfssPhaseOne(Abc_Ntk_t* FNtk, Aig_Man_t* FAig, vector<int>& unate, int numTotUnates, bool useBDD, string varsFileName, string baseFileName)
{
	auto main_end = TIME_NOW;
	double total_main_time;
	//map<string, int> name2IdF;
	//map<int, string> id2NameF;

	Aig_ManPrintStats( FAig );
	Nnf_Man nnfNew;
	if(useBDD)
    {
		// ************************
		// Creating BDD Start
		Abc_Ntk_t* FNtkSmall = Abc_NtkFromAigPhase(FAig);
		cout << "Creating BDD..." << endl;
		TIME_MEASURE_START
		// Abc_NtkBuildGlobalBdds(FNtkSmall, int fBddSizeMax, int fDropInternal, int fReorder, int fVerbose );
		DdManager* ddMan = (DdManager*)Abc_NtkBuildGlobalBdds(FNtkSmall, 1e10, 1, 1, 1);
		auto bdd_end = TIME_NOW;
		cout << "Created BDD!" << endl;
		DdNode* FddNode = (DdNode*)Abc_ObjGlobalBdd(Abc_NtkCo(FNtkSmall,0));
		cout << "Time taken:   " << TIME_MEASURE_ELAPSED << endl;
		cout << "BDD Size:     " << Cudd_DagSize(FddNode) << endl;
		assert(ddMan->size == Abc_NtkCiNum(FNtkSmall));
		// Creating BDD End
		// **************************
		
		nnfNew.init(ddMan, FddNode);
		cout << "Created NNF from BDD" << endl;
	}
	else
    {
		nnfNew.init(FAig);
		cout << "Created NNF from FAig" << endl;
	}

    varsXF.clear();
    varsYF.clear();
    varsXS.clear();
    varsYS.clear();
    name2IdF.clear();
    id2NameF.clear();

//    cout << "No. of CI's in FAig  " << Aig_ManCiNum(FAig)<< endl;
	Aig_Man_t* SAig = nnfNew.createAigWithoutClouds();

	numOrigInputs = nnfNew.getCiNum();
 //   cout << "No. of CI's in SAig  " << Aig_ManCiNum(SAig)<< endl;

	OUT("Aig_ManCoNum(SAig): " << Aig_ManCoNum(SAig));
	populateVars(FNtk, nnfNew, varsFileName,
					varsXF, varsXS,
					varsYF, varsYS,
					name2IdF, id2NameF);



	cout << "numX " << numX << endl;
	cout << "numY " << numY << endl;
	cout << "numUnate " << numY - count(unate.begin(), unate.end(), -1) << endl;
	cout << "numOrigInputs " << numOrigInputs << endl;

	#ifdef DEBUG_CHUNK // Print varsXS, varsYS
		cout << "varsXF: " << endl;
		for(auto it : varsXF)
		    cout << it << " ";
		cout<<endl;
		cout << "varsYF: " << endl;
		for(auto it : varsYF)
		    cout << it << " ";
		cout<<endl;

		cout << "varsXS: " << endl;
		for(auto it : varsXS)
			cout << it << " ";
		cout<<endl;
		cout << "varsYS: " << endl;
		for(auto it : varsYS)
			cout << it << " ";
		cout<<endl;
	#endif

	assert(numX + numY == numOrigInputs);

	// Populate Input Name Vectors (Same for SAig and FAig)
	assert(varsNameX.empty());
	assert(varsNameY.empty());
	for(auto id:varsXF)
		varsNameX.push_back(id2NameF[id]);
	for(auto id:varsYF)
		varsNameY.push_back(id2NameF[id]);

	OUT("Cleaning up...");
	int removed = Aig_ManCleanup(SAig);
	OUT("Removed "<<removed<<" nodes");

/* Getting var names of the TseitinVars */
    //Abc_Obj_t * pNode;

  //  Abc_NtkForEachNode( FNtk, pNode, index )   
   // {
    //    cout  << "i : " << index << " " << Abc_ObjName (pNode) << endl;
    //}

	bool isWDNNF = (numTotUnates == numY); //false;
    bool checkwDNNF = true;
	if(checkwDNNF && (! isWDNNF))
    {
		// populate varsYNNF: vector of input numbers
		vector<int> varsYNNF;
		for(auto y:varsYF)
        {
			int i;
			for (i = 0; i < nnfNew.getCiNum(); ++i)
				if(nnfNew.getCiPos(i)->OrigAigId == y)
					break;
			assert(i != nnfNew.getCiNum());
			varsYNNF.push_back(i);
		}

		cout << "Checking wDNNF" << endl;
		isWDNNF = nnfNew.isWDNNF(varsYNNF);
	}
	if(isWDNNF)
    {
		cout << "********************************" << endl;
		cout << "** In wDNNF!" << endl;
		cout << "** Will Predict Exact Skolem Fns" << endl;
		cout << "********************************" << endl;
	}
	else
    {
		cout << "********************************" << endl;
		cout << "** Not wDNNF :(" << endl;
		cout << "********************************" << endl;
	}

    options.evalAigAtNode = true;
	Aig_ManPrintStats( SAig );
	cout << "Compressing SAig..." << endl;
	SAig = compressAigByNtkMultiple(SAig, 3);
	assert(SAig != NULL);
	Aig_ManPrintStats( SAig );

	// Fs[0] - F_SAig      will always be Aig_ManCo( ... , 1)
	// Fs[1] - FPrime_SAig will always be Aig_ManCo( ... , 2)
	vector<Aig_Obj_t* > Fs(2);
	vector<vector<int> > r0(numY), r1(numY);
	cout << "initializeCompose(SAig, Fs, r0, r1)..."<<endl;
	clock_t compose_start = clock();
	initializeCompose(SAig, Fs, r0, r1, unate);
	clock_t compose_end = clock();
	cout<< "Mega compose time: " <<double(compose_end-compose_start)/CLOCKS_PER_SEC << endl;

	Aig_Obj_t* F_SAig = Fs[0];
	Aig_Obj_t* FPrime_SAig = Fs[1];
	Aig_ManSetCioIds(SAig);
	F_SAigIndex = F_SAig->CioId;
	FPrime_SAigIndex = FPrime_SAig->CioId;
	cout << "F_SAigIndex: " << F_SAigIndex << endl;
	cout << "FPrime_SAigIndex: " << FPrime_SAigIndex << endl;

	// Patch 0th Output of SAig (is taking un-necessary size)
	Aig_ObjPatchFanin0(SAig, Aig_ManCo(SAig,0), Aig_ManConst0(SAig));

	// Global Optimization: Used in dinfing k2Max
//	if(!isWDNNF) {
//		m_pSat = sat_solver_new();
//		m_FCnf = Cnf_Derive(SAig, Aig_ManCoNum(SAig));
//		m_f = toLitCond(getCnfCoVarNum(m_FCnf, SAig, F_SAigIndex), 0);
//		addCnfToSolver(m_pSat, m_FCnf);
//	}

	#ifdef DEBUG_CHUNK // Print SAig, checkSupportSanity
		cout << "\nSAig: " << endl;
		Abc_NtkForEachObj(SNtk,pAbcObj,i)
			cout <<"SAig Node "<<i<<": " << Abc_ObjName(pAbcObj) << endl;

		cout << "\nSAig: " << endl;
		Aig_ManForEachObj( SAig, pAigObj, i )
			Aig_ObjPrintVerbose( pAigObj, 1 ), printf( "\n" );

		cout << "checkSupportSanity(SAig, r0, r1)..."<<endl;
		checkSupportSanity(SAig, r0, r1);
	#endif
	cout << "Created SAig..." << endl;
	cout << endl;

    
	useR1AsSkolem = vector<bool>(numY,true);
	initializeAddR1R0toR();
	if(options.proactiveProp)
		switch(options.skolemType) {
			case (sType::skolemR0): propagateR0Cofactors(SAig,r0,r1); break;
			case (sType::skolemR1): propagateR1Cofactors(SAig,r0,r1); break;
			case (sType::skolemRx): propagateR0R1Cofactors(SAig,r0,r1); break;
		}
     
	chooseR_(SAig,r0,r1);
	cout << endl;

    bool solved = false;
	Aig_ManPrintStats( SAig );
	cout << "Compressing SAig..." << endl;
	SAig = compressAigByNtkMultiple(SAig, 2);
	assert(SAig != NULL);
	Aig_ManPrintStats( SAig );
	#ifdef DEBUG_CHUNK // Print SAig, checkSupportSanity
		cout << "\nSAig: " << endl;
		Aig_ManForEachObj( SAig, pAigObj, i )
			Aig_ObjPrintVerbose( pAigObj, 1 ), printf( "\n" );

		cout << "checkSupportSanity(SAig, r0, r1)..."<<endl;
		checkSupportSanity(SAig, r0, r1);
	#endif

	if(isWDNNF)
    {
		cout << "In WDNNF, No need to check Error Formula. Printing SynNNF Form ..." << endl;
        solved = true;
	}
	else 
    {

       
		// CEGAR Loop
		cout << "Checking Error Formula ..."<<endl;
        vector<int> r0Andr1Vars(numY);

        pSat = sat_solver_new();
        auto cnf_flag  = buildErrorFormula(pSat, SAig, r0, r1, r0Andr1Vars);
        Cnf_Dat_t* SCnf  = cnf_flag.first;
        bool allOk = cnf_flag.second;
        OUT("Simplifying..." );
        if(!allOk or !sat_solver_simplify(pSat)) {
            cout << "Formula is trivially unsat" << endl;
            solved = true;
        }
        else {
            OUT("Solving..." );
            vector<lit> assumptions = setAllNegX(SCnf, SAig, false);
            int status = sat_solver_solve(pSat,
                            &assumptions[0], &assumptions[0] + numX,
                            (ABC_INT64_T)0, (ABC_INT64_T)0,
                            (ABC_INT64_T)0, (ABC_INT64_T)0);

            if (status == l_False) {
               cout << "Formula is unsat" << endl;
                solved = true;
            }
            else if (status == l_True) {
                solved = false;
            }

        sat_solver_delete(pSat);
        Cnf_DataFree(SCnf);
      }
   }
   /*
   if(! solved)
   {
    //Add constant skolem functions in depCONST
    	for (int i = 0; i < numY; ++i) {
            if (Aig_ObjIsConst1(Aig_Regular(r1[i][0])))
            {
                string varName = id2NameF[varsXF[i]];
                int varNum =  stoi(varName.substr(2));
                cout << "The r1 skolem function for Y " << varsYS[i] << " is a constant " << endl;
                if (Aig_IsComplement(r1[i])) //Insert -r1
                        depCONST.insert(varNum);
                else
                        depCONST.insert(-varNum);
            }
            else
            if (Aig_ObjIsConst1(Aig_Regular(r0[i][0]))) {
                string varName = id2NameF[varsXF[i]];
                int varNum =  stoi(varName.substr(2));
                cout << "The r0 skolem function for Y " << varsYS[i] << " is a constant " << endl;
                if (Aig_IsComplement(r0[i])) //Insert r0
                        depCONST.insert(-varNum);
                else
                        depCONST.insert(varNum);
            }
	    }
        return false;
   }
   */ 
	vector<int> skolemAig(numY);
	 Aig_Obj_t*pAigObj;
    cout << "Building Skolem AIG/Adding DepCONST with constant skolem functions " << endl;

	for(int i = 0; i < numY; i++) {
		if(useR1AsSkolem[i])
        {
			pAigObj	= Aig_Not(newOR(SAig, r1[i]));
       /*     if (!solved)
            {
                string varName = id2NameF[varsYS[i]];
                int varNum =  stoi(varName.substr(2));
                if (Aig_ObjIsConst1(Aig_Regular(pAigObj)) && Aig_IsComplement(pAigObj)) //If the skolem function given by r1 = 0
                {
                        depCONST.insert(-varNum);
                }    
            }
            */

        }
		else
        {
			pAigObj	= newOR(SAig, r0[i]);
            /*if (!solved)
            {
                string varName = id2NameF[varsYS[i]];
                int varNum =  stoi(varName.substr(2));
                if (Aig_ObjIsConst1(Aig_Regular(pAigObj)) && (! Aig_IsComplement(pAigObj))) //If the skolem function given by r0 = 1
                {
                        depCONST.insert(varNum);
                }    
            }
            */
        }
        if (solved)
        {
            pAigObj = Aig_ObjCreateCo(SAig,pAigObj);
            skolemAig[i] = Aig_ManCoNum(SAig)-1;
            assert(pAigObj!=NULL);
        }
        /*else
        {
            if (Aig_ObjIsConst1(Aig_Regular(pAigObj)))
            {
                string varName = id2NameF[varsYS[i]];
                int varNum =  stoi(varName.substr(2));
                cout << "The r0/r1 skolem function for Y " << varsYS[i] << " is a constant " << endl;
                if (Aig_IsComplement(pAigObj)) //Insert r0
                        depCONST.insert(-varNum);
                else
                        depCONST.insert(varNum);
             }
         }
         */
	}

    if (!solved)
        return false;

	saveSkolemFns(SAig, skolemAig, baseFileName );
	main_end = TIME_NOW;
	total_main_time = std::chrono::duration_cast<std::chrono::microseconds>(main_end - main_time_start).count()/1000000.0;
	cout<< "Total bfss phase 1 time:         " << total_main_time << endl;
	cout<< "Total SAT solving time:  " << sat_solving_time << endl;

	if(m_pSat!=NULL) sat_solver_delete(m_pSat);
	if(m_FCnf!=NULL) Cnf_DataFree(m_FCnf);

	// Stop ABC
    cout << "Bfss phase 1 returns " << solved << endl;
	return solved;
}
//Copied from helper.cpp and modified to include printing of the tseitin variables too
//Tseitin vars not being printed yet

void saveSkolemFns(Aig_Man_t* SAig, vector <int>& skolemAig, string baseFileName ) {

	int i;
	Abc_Obj_t* pObj;
    string outfname = baseFileName + "_norevsub.v";

	// Specify Ci/Co to pick
	Vec_Ptr_t * vPis = Vec_PtrAlloc(numX);
	Vec_Ptr_t * vPos = Vec_PtrAlloc(numY);
	for(auto it:varsXS) {
		Vec_PtrPush(vPis, Aig_ManObj(SAig,it));
	}
	for(auto it:varsYS) {
		Vec_PtrPush(vPis, Aig_ManObj(SAig,it));
	}
	for(auto it:skolemAig) {
		Vec_PtrPush(vPos, Aig_ManCo(SAig,it));
	}

	// Get partial Aig, Ntk
	Aig_Man_t* outAig = Aig_ManDupSimpleDfsPart(SAig, vPis, vPos);
	Abc_Ntk_t* outNtk = Abc_NtkFromAigPhase(outAig);

	// Unset and Set Input, Output Names
	string ntkName = baseFileName +"_skolem";
	Abc_NtkSetName(outNtk, Abc_UtilStrsav((char*)(ntkName).c_str()));
	Nm_ManFree(outNtk->pManName);
	outNtk->pManName = Nm_ManCreate(200);

	int xs =varsXS.size() ;
	assert (xs == numX);

	Abc_NtkForEachCi(outNtk, pObj, i) {
			if (i < numX )
			{
				//cout << "Assigning name " <<  (char*)varsNameX[i].c_str() << " to ci " << i << endl;
				Abc_ObjAssignName(pObj, (char*)varsNameX[i].c_str(), NULL);
			}
			else
			{
				string yname =  "nn" + varsNameY[i - numX];
				//cout << "Assigning name " <<  yname << " to ci " << i << endl;
				Abc_ObjAssignName(pObj, (char*)yname.c_str(), NULL);
			}
	}
	Abc_NtkForEachCo(outNtk, pObj, i) {
			//cout << "Assigning name " <<  (char*)varsNameX[i].c_str() << " to co " << i << endl;
		Abc_ObjAssignName(pObj, (char*)varsNameY[i].c_str(), NULL);
	}

	// Write to verilog
	Abc_FrameSetCurrentNetwork(pAbc, outNtk);
	string command = "write "+outfname;
	if (Cmd_CommandExecute(pAbc, (char*)command.c_str())) {
		cerr << "Could not write result to verilog file" << endl;
	}
	else {
		cout << "Saved skolems to " << outfname << endl;
	}

}
