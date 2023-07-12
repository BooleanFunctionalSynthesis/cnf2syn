/** Code to verify if the output synNNF (blif file) is indeed in synNNF 
    Copied from Jatin and Divya's code
**/
#include "helper.h"

//Declared so that linking happens.
//Required due to inclusion of helper.h
vector<int> varsXF;
vector<int> varsYF; // to be eliminated
int numOrigInputs = 0, numX = 0, numY = 0;

vector<string> varsNameX, varsNameY;
Abc_Frame_t* pAbc = NULL;
sat_solver* m_pSat = NULL;
Cnf_Dat_t* m_FCnf = NULL;
lit m_f = 0;
sat_solver* pSat = NULL;
Cnf_Dat_t* pCnf = NULL;
lit l_p = 0;
double sat_solving_time = 0;
double verify_sat_solving_time = 0;
double reverse_sub_time = 0;
chrono_steady_time helper_time_measure_start = TIME_NOW;
chrono_steady_time main_time_start = TIME_NOW;
vector<int> varsXS;
vector<int> varsYS; // to be eliminated

//--------------------------------
	Abc_Ntk_t* blifToNtk(string blifFile) {
		string cmd, initCmd, varsFile, line;
		Abc_Obj_t* pPi, *pCi;
		set<int> varsX;
		set<int> varsY; // To Be Eliminated
		map<string, int> name2Id;
		int liftVal, cummulativeLift = 0;

        
		bool fraig = true;
		initCmd = "balance; rewrite -l; refactor -l; balance; rewrite -l; \
							rewrite -lz; balance; refactor -lz; rewrite -lz; balance";

        pAbc = Abc_FrameGetGlobalFrame();

		cmd = "read " + blifFile + "; strash";
		if (CommandExecute(pAbc, cmd)) { // Read the AIG
			return NULL;
		}
		cmd = fraig?initCmd:"balance";
		if (CommandExecute(pAbc, cmd)) { // Simplify
			return NULL;
		}

		Abc_Ntk_t* pNtk =  Abc_FrameReadNtk(pAbc);
		//cout<<"ntk type: "<<pNtk->ntkType<<endl;

		//Aig_Man_t* SAig = Abc_NtkToDar(pNtk, 0, 0);
		// printAig(SAig);
		
		//int i;
		// Abc_Obj_t* pPi;
		//Abc_NtkForEachCi( pNtk, pPi, i ) {
		//	int var = getLiteralNum(string(Abc_ObjName(pPi)));
			// cout<<"name: "<<Abc_ObjName(pPi)<<" id:"<<pPi->Id<<endl;
		//}
		//checkReordering(SAig, pNtk);
        cout << "Read the syn nnf form in abc " << endl;
		return pNtk;
	}


Aig_Man_t* QdimacsToAig(const char * qdFileName, vector<Aig_Obj_t*>& varsXAig, vector<Aig_Obj_t*>& varsYAig)
{
	    char C[100], c;
	    int tmpVar;

		FILE *qdFPtr = fopen (qdFileName, "r");

        int numVars = 0, numClauses=0;
		// Number of variables and clauses
		fscanf (qdFPtr, "%c", &c);
		fscanf (qdFPtr, "%s", C);
		while (strcmp (C, "cnf") != 0)
			fscanf (qdFPtr, "%s", C);

		fscanf(qdFPtr, "%d %d", &numVars, &numClauses); // read first line p cnf
		// cout << "numVars:       " <<  numVars << endl;
		// cout << "NumClauses:   " << numClauses << endl;

		// Vars X
		set<int> aline, eline;
		fscanf (qdFPtr, "%c", &c);
		while (c != 'a')
			fscanf (qdFPtr, "%c", &c);
		fscanf(qdFPtr, "%d", &tmpVar);
		while (tmpVar !=0) {
			aline.insert(tmpVar);
			fscanf(qdFPtr, "%d", &tmpVar);
		}
		// cout << "varsXF.size(): " << varsXFCopy.size() << endl;
		assert (numVars > varsXF.size());

		// Vars Y (to elim)
		fscanf (qdFPtr, "%c", &c);
		while (c != 'e')
			fscanf (qdFPtr, "%c", &c);
		fscanf(qdFPtr, "%d", &tmpVar);
		while (tmpVar !=0) {
			eline.insert(tmpVar);
			fscanf(qdFPtr, "%d", &tmpVar);
		}
		// cout << "varsYF.size(): " << varsYFCopy.size() << endl;
		assert (numVars > varsYF.size());

//Start building the AIG
	    Aig_Man_t* pMan = Aig_ManStart(0);

		for (int i = 1; i <=  numVars ; i++) {

			Aig_Obj_t* ci = Aig_ObjCreateCi(pMan);
		}
//Fill in the varsXAig and varsYAig vectors
        for (auto &it : aline)
        {
            varsXAig.push_back( Aig_ManCi(pMan, it - 1));
        }
        for (auto &it : eline)
        {
            varsYAig.push_back( Aig_ManCi(pMan, it - 1));
        }

		// Clauses
		Aig_Obj_t* and_node = Aig_ManConst1(pMan);
		Aig_Obj_t* cl1 = NULL, *cl2 = NULL;
		for (int i = 0; i < numClauses ; i++)
        {

			//vector<int> tempClause;
			fscanf(qdFPtr, "%d", &tmpVar);
			Aig_Obj_t* or_node = Aig_ManConst0(pMan);
			Aig_Obj_t* c1 = NULL, *c2 = NULL;
			bool first = true;

            
			while (tmpVar != 0)
            {
				bool pos = (tmpVar > 0) ? true: false;

				//c1 = freshVars? Aig_ManCiFresh(pMan, tmpVar, aline, eline, numVars) : Aig_ManCi(pMan, abs(tmpVar)-1);
				c1 =  Aig_ManCi(pMan, abs(tmpVar)-1);
		        if (!pos)
				    c1 = Aig_Not(c1);

				or_node = Aig_Or(pMan, c1, c2=or_node);

				fscanf(qdFPtr, "%d", &tmpVar);
			}

			and_node = Aig_And(pMan, or_node, cl2=and_node);
		}

		Aig_ObjCreateCo(pMan, and_node);

		return pMan;

		fclose (qdFPtr);
	}


/*	void checkReordering(Aig_Man_t* SAig, Abc_Ntk_t* FNtk) {
		vector<int> orderVars;
		map<int, int> varNumToAbcId;
		Abc_Obj_t* pPi;
		int i;

		// Vec_Ptr_t * vPis = Vec_PtrAlloc(2* originalVarCount);
		int numX = 0, numY =0;
		Abc_NtkForEachCi( FNtk, pPi, i ) {
			int var = getLiteralNum(string(Abc_ObjName(pPi)));
			varNumToAbcId[var] = pPi->Id;
			// cout<<"name: "<<Abc_ObjName(pPi)<<" id:"<<pPi->Id<<endl;
			if(!isVarElim(abs(var))){
				numX++;
				orderVars.push_back(var);
			}
		}

		for(auto q: varOrderGraph.current_order) {
			assert(q==0 || varId_to_num.count(q) > 0);
			
			if(isOutputVar(q)) {
				numY++;
				orderVars.push_back(varId_to_num[q]);
			} 
		}

		for(int i=1; i<=originalVarCount; i++) {
			if(isVarElim(i)) {
				if(find(orderVars.begin(), orderVars.end(), i)==orderVars.end()) {
					numY++;
					orderVars.push_back(i);
				}
			}
		}

		for(int i=0; i<numX; i++) {
			Aig_Obj_t* ci = Aig_ObjCreateCi(SAig);
			int litNum = -1*(orderVars[i]);
			orderVars.push_back(litNum);
			varNumToAbcId[litNum] = ci->Id;
		}

		for(int i=0; i<numY; i++) {
			orderVars.push_back(-1*(orderVars[i+numX]));
		}

		cout<<"numX "<<numX<<" numY: "<<numY<<" originalVarCount "<<originalVarCount<<endl;
		assert(numX+numY==originalVarCount);


		// cout<<"orderVars "; 
		// for(auto q: orderVars)
		// 	cout<<q<<" ";
		// cout<<endl;
		
		Vec_Ptr_t * vPis = Vec_PtrAlloc(2*originalVarCount);
		Vec_Ptr_t * vPos = Vec_PtrAlloc(1);

		
		for(int j=0; j<2*originalVarCount; j++) {
			int q = orderVars[j];
			if(q>0) {
				
				// if(!isVarElim(q))
				// 	varsXS.push_back(j+1);
				// else 
				// 	varsYS.push_back(j+1);

			}

			Vec_PtrPush(vPis, Aig_ManObj(SAig, varNumToAbcId[q]));
		}


		Vec_PtrPush(vPos, Aig_ManCo(SAig,0));
		Aig_Man_t* SAig_ordered = Aig_ManDupSimpleDfsPart(SAig, vPis, vPos);

		// printAig(SAig_ordered);	
	}
    */


//F_man has 3 CO's - CO-0 is F(X, Y), CO-1 is F~(X, Y) CO-2 is F(X, Y')
	void writeConversionCheckA(Aig_Man_t* F_man, vector<Aig_Obj_t*> varsXAig, vector<Aig_Obj_t*> varsYAig,  vector<Aig_Obj_t*> varsYCopy, string checkA_filename) {

       // Aig_Obj_t* F_node  Aig_ManCo(F_man, 0);
        Aig_Obj_t* Fd_node = Aig_ObjChild0(Aig_ManCo(F_man, 1));
//		Aig_Obj_t* Fd_node_neg = Aig_Not(Fd_node);
		Aig_Obj_t* Fprime_node = Aig_ObjChild0(Aig_ManCo(F_man,2));
		Aig_Obj_t* Fprime_node_neg = Aig_Not(Fprime_node);

		Aig_Obj_t* cond = Aig_Or(F_man, Fprime_node_neg, Fd_node);
        Aig_Obj_t* checkCo = Aig_ObjCreateCo (F_man, cond);
        //Aig_ObjPatchFanin0 (F_man, Aig_ManCo(F_man,0), cond);
       // Vec_Ptr_t * vPis = F_man->vCis

        //cout << "printing Aig F_man" << endl;
       //printAig (F_man);

        Vec_Ptr_t * vPos = Vec_PtrAlloc(1);
		Vec_PtrPush(vPos, checkCo);
	    Aig_Man_t* checkA = Aig_ManDupSimpleDfsPart(F_man, F_man->vCis, vPos);

        //cout << "printing Aig checkA" << endl;
       //printAig (checkA);
        Aig_Obj_t * pObj;
        int i;
        //Aig_ManForEachCi(checkA, pObj, i)
        //{
         //       cout << " Var " << i << " is " << Aig_ObjId(pObj) << endl;
        //}
        Aig_ManPrintStats (checkA);

		//Aig_ObjCreateCo(F_man, cond);
		//int condCoNum = Aig_ManCoNum(F_man)-1;

		// Aig_ManPrintStats( F_man );
		// F_man = compressAigByNtkMultiple(F_man, 3);
	//	cout << "Faig" << endl;
	//	printAig(F_man);
	//	cout << "F_d_aig" << endl;
	//	printAig(F_d_man);

        //Aig_ObjDeletePo(F_man, Aig_ManCo(F_man, 2));
        //Aig_ObjDeletePo(F_man, Aig_ManCo(F_man, 1));
        assert (Aig_ManCoNum(checkA) == 1);

		Cnf_Dat_t* FCnf = Cnf_DeriveSimple(checkA, Aig_ManCoNum(checkA));
		int cond_var = getCnfCoVarNum(FCnf, checkA, 0);

        int numforall = varsXAig.size() + varsYAig.size();
        Vec_Int_t * vVarMap, * vForAlls, * vExists;
        vVarMap = Vec_IntStart( FCnf->nVars );
        Aig_ManForEachCi(checkA, pObj, i )
        {
            if ( i < numforall )        //Assumption varsXAig and varsYAig occur before varsYCopy. Please check
             Vec_IntWriteEntry( vVarMap, FCnf->pVarNums[Aig_ObjId(pObj)], 1 );
        }
    // create various maps
        vForAlls = Vec_IntAlloc( numforall );
        vExists = Vec_IntAlloc( FCnf->nVars - numforall );
        int Entry;
        Vec_IntForEachEntry( vVarMap, Entry, i )
        {
            if (i > 0 ) 
            {
                if ( Entry )
                    Vec_IntPush( vForAlls, i );
                else
                    Vec_IntPush( vExists, i );
             }
        }
    // generate CNF
		char filename[checkA_filename.size()];
		strcpy(filename, checkA_filename.c_str());
		Cnf_DataWriteIntoFile( FCnf, filename, 1, vForAlls, vExists); // check fReadable arg
		string assert_true = to_string(cond_var) + " 0\n";
		//cout<<"assert final CNF is true: "<<assert_true<<endl;
		ofstream ofs(filename, ios_base::app);
		ofs<<assert_true;
		cout<<"Written check (a) into file "<<checkA_filename<<endl;
        Aig_ManStop (checkA);
	}

//This is one direction of check (b) Def 4 Section iV of the paper
bool verifyConversion(Aig_Man_t* F_d_man, Aig_Man_t* F_man, vector<Aig_Obj_t*> varsXAig, vector<Aig_Obj_t *> varsYAig, string baseFileName) {

//		cout<<"num cis in Fd_man "<<Aig_ManCiNum(F_d_man)<<endl;
		assert(Aig_ManCiNum(F_man) == Aig_ManCiNum(F_d_man));

//		if (Aig_ObjIsConst1(Aig_Regular(Aig_ObjChild0(Aig_ManCo(F_d_man,0)))))
//			cout << " F_d_man points to const0/1 " << endl;
/**Transfer will transfer F_d_man to F_man  */

		Aig_Obj_t * Fd_node = Aig_Transfer(F_d_man, F_man, Aig_ObjChild0(Aig_ManCo(F_d_man,0)), Aig_ManCiNum(F_man));
        Aig_Obj_t * FdCo = Aig_ObjCreateCo(F_man, Fd_node);

        int numY = varsYAig.size();
        vector<int> varIdVec;
        vector<Aig_Obj_t* > funcVec;
        vector<Aig_Obj_t* > varsYCopy;
        funcVec.resize(0);
        for(int i = 0; i < numY; ++i) {
            Aig_Obj_t* ci = Aig_ObjCreateCi(F_man);
            funcVec.push_back(ci);
            varsYCopy.push_back(ci);
        }

        for (int i = 0; i < numY; i++)
            varIdVec.push_back (Aig_ObjId(varsYAig[i])); 

        Aig_Obj_t* FCo = Aig_ManCo(F_man, 0);
		Aig_Obj_t* F_node = Aig_ObjChild0(FCo);
		Aig_Obj_t* F_node_neg = Aig_Not(F_node);

        Aig_Obj_t* Fprime_node = Aig_SubstituteVec(F_man, F_node, varIdVec, funcVec); //F prime Y
        Aig_Obj_t* FprimeCo = Aig_ObjCreateCo(F_man, Fprime_node);

		//Aig_Obj_t* Fdpos_Fneg = Aig_And(F_man, Fd_node, F_node_neg);
        //Aig_Obj_t* Fprime_Fdpos_Fneg = Aig_And (F_man, Fdpos_Fneg, Fprime_Fdpos_Fneg);
        //Aig_ObjCreateCo(F_man, Fprime_Fdpos_Fneg);


        int condCoNum = Aig_ManCoNum(F_man)-1;
        
      //  Aig_ManPrintStats( F_man );
	//	F_man = compressAigByNtkMultiple(F_man, 3);
		assert(F_man != NULL);
		Aig_ManPrintStats( F_man );

		sat_solver* pSat = sat_solver_new();
		Cnf_Dat_t* FCnf = Cnf_Derive(F_man, Aig_ManCoNum(F_man));
		addCnfToSolver(pSat, FCnf);

//Check condition A -- note that the function writeConversionCheckA changes F_man

		vector<int> assumptions;

//TODO:: Push Set appropriate cos to true
		assumptions.push_back(toLitCond(getCnfCoVarNum(FCnf, F_man,  0),1)); // set to false      // (\neg F(X, Y) \and F~(X, Y') \and F(X, Y')
		assumptions.push_back(toLitCond(getCnfCoVarNum(FCnf, F_man, 1) ,0)); // set to true
		assumptions.push_back(toLitCond(getCnfCoVarNum(FCnf, F_man, 2),0)); // set to true
		
		cout << "Calling SAT Solver..." << endl;
		int status = sat_solver_solve(pSat, &assumptions[0], &assumptions[0] + assumptions.size(), (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0);
		if (status == l_False) {
			cout << "Conversion verified!" << endl;
		}
		else if (status == l_True) {
			cout << "Conversion not verified!" << endl;
			cerr << "Conversion not verified!" << endl;
		}
		sat_solver_delete(pSat);
		Cnf_DataFree(FCnf);

        string checkA_filename = baseFileName +".checkA.qdimacs";
        writeConversionCheckA(F_man, varsXAig, varsYAig, varsYCopy, checkA_filename);

		if (status == l_False)
			return true; // verified
		else
			return false; 

	}
    /*

bool semCheck (Aig_Man_t *synAig,  vector<int>& semCheckVar, vector<int> &varsXF, vector<int>& varsYF)
{

		// Aig_ManPrintStats( synAig );
		// F_man = compressAigByNtkMultiple(synAig, 3);

		sat_solver* pSat = sat_solver_new();
		Cnf_Dat_t* SCnf = Cnf_Derive(synAig, Aig_ManCoNum(synAig));
		addCnfToSolver(pSat, FCnf);
	    int numCnfVars = SCnf->nVars;
	    Cnf_Dat_t* SCnf_copy1 = Cnf_DataDup(SCnf);
 	    Cnf_DataLift (SCnf_copy1, numCnfVars);	
	    addCnfToSolver(pSat, SCnf_copy1);
	     numCnfVars +=SCnf->nVars;
	    Cnf_Dat_t* SCnf_copy2 = Cnf_DataDup(SCnf);
 	    Cnf_DataLift (SCnf_copy2, numCnfVars);	
	    addCnfToSolver(pSat, SCnf_copy2);
	     numCnfVars +=SCnf->nVars;
	    Cnf_Dat_t* SCnf_copy3 = Cnf_DataDup(SCnf);
 	    Cnf_DataLift (SCnf_copy3, numCnfVars);	
	    addCnfToSolver(pSat, SCnf_copy3);
	    numCnfVars +=SCnf->nVars;

        //Creating 4 copies of the formula.

	lit Lits[3];
    
	for(int i = 0; i < varsXF.size(); ++i)
    {
		Lits[0] = toLitCond( SCnf->pVarNums[ varsXF[i]], 0 );
		Lits[1] = toLitCond( SCnf_copy1->pVarNums[ varsXF[i]], 1 );
	//	cout << " Adding clause " <<  Lits [0] << " " << Lits [1] << endl;
        	if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            		assert( 0 );

		Lits[1] = toLitCond( SCnf_copy2->pVarNums[ varsXF[i]], 1 );
	//	cout << " Adding clause " <<  Lits [0] << " " << Lits [1] << endl;
        	if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            		assert( 0 );
		Lits[1] = toLitCond( SCnf_copy3->pVarNums[ varsXF[i]], 1 );
        	if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            		assert( 0 );

		Lits[0] = toLitCond( SCnf->pVarNums[ varsXF[i]], 1 );
		Lits[1] = toLitCond( SCnf_copy1->pVarNums[ varsXF[i]], 0 );
        	if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            		assert( 0 );

		Lits[1] = toLitCond( SCnf_copy2->pVarNums[ varsXF[i]], 0 );
	//	cout << " Adding clause " <<  Lits [0] << " " << Lits [1] << endl;
        	if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            		assert( 0 );
		Lits[1] = toLitCond( SCnf_copy3->pVarNums[ varsXF[i]], 0 );
        	if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            		assert( 0 );
	}
    int numY = varsYF.size();
	int cont_Vars[semCheckVars.size()]; //control Variables
    int contIndex = 0;
	for(int i = 0; i < numY; ++i) {

        if (find (semCheckVars.begin(), semCheckvars.end(), varsYF[i]) != semCheckVars.end())
        {
            cont_Vars[contIndex++] = sat_solver_addvar (pSat);
            Lits[0] = toLitCond( SCnf->pVarNums[ varsYF[i]], 0 );
            Lits[1] = toLitCond( SCnf_copy1->pVarNums[ varsYF[i]], 1 );
            Lits[2] = Abc_Var2Lit (cont_Vars[i], 0);
    //		cout << " Adding clause for " << i << " "  <<  Lits [0] << " " << Lits [1] << " " << Lits [2] << endl;
                if ( !sat_solver_addclause( pSat, Lits, Lits+3 ) )
                        assert( 0 );

            Lits[1] = toLitCond( SCnf_copy2->pVarNums[ varsYF[i]], 1 );
                if ( !sat_solver_addclause( pSat, Lits, Lits+3 ) )
                        assert( 0 );

            Lits[1] = toLitCond( SCnf_copy3->pVarNums[ varsYF[i]], 1 );
                if ( !sat_solver_addclause( pSat, Lits, Lits+3 ) )
                        assert( 0 );

            Lits[0] = toLitCond( SCnf->pVarNums[ varsYF[i]], 1 );
            Lits[1] = toLitCond( SCnf_copy1->pVarNums[ varsYF[i]], 0 );
                if ( !sat_solver_addclause( pSat, Lits, Lits+3 ) )
                        assert( 0 );
            Lits[1] = toLitCond( SCnf_copy2->pVarNums[ varsYF[i]], 0 );
                if ( !sat_solver_addclause( pSat, Lits, Lits+3 ) )
                        assert( 0 );

            Lits[1] = toLitCond( SCnf_copy3->pVarNums[ varsYF[i]], 0 );
                if ( !sat_solver_addclause( pSat, Lits, Lits+3 ) )
                        assert( 0 );
           }
           else //just equate the Y variables.
           {
            Lits[0] = toLitCond( SCnf->pVarNums[ varsYF[i]], 0 );
            Lits[1] = toLitCond( SCnf_copy1->pVarNums[ varsYF[i]], 1 );
    //		cout << " Adding clause for " << i << " "  <<  Lits [0] << " " << Lits [1] << " " << Lits [2] << endl;
                if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
                        assert( 0 );

            Lits[1] = toLitCond( SCnf_copy2->pVarNums[ varsYF[i]], 1 );
                if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
                        assert( 0 );

            Lits[1] = toLitCond( SCnf_copy3->pVarNums[ varsYF[i]], 1 );
                if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
                        assert( 0 );

            Lits[0] = toLitCond( SCnf->pVarNums[ varsYF[i]], 1 );
            Lits[1] = toLitCond( SCnf_copy1->pVarNums[ varsYF[i]], 0 );
                if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
                        assert( 0 );
            Lits[1] = toLitCond( SCnf_copy2->pVarNums[ varsYF[i]], 0 );
                if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
                        assert( 0 );

            Lits[1] = toLitCond( SCnf_copy3->pVarNums[ varsYF[i]], 0 );
                if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
                        assert( 0 );

          }
	}
	Aig_Obj_t * pCo = Aig_ManCo(synAig, 0);
	addVarToSolver(pSat, SCnf->pVarNums[pCo->Id], 0);
	addVarToSolver(pSat, SCnf_copy1->pVarNums[pCo->Id], 1);
	addVarToSolver(pSat, SCnf_copy2->pVarNums[pCo->Id], 1);
	addVarToSolver(pSat, SCnf_copy3->pVarNums[pCo->Id], 1);

    for (auto &it : semCheckVars)   //Check only for these wars
    {
        
			yIndex  = SCnf->pVarNums[it];
                toLitCond(SCnf->pVarNums[varsYF[i]], 1); //check the 0 and 1's
				LA[1] = toLitCond(SCnf_copy->pVarNums[varsYF[i]], 0);
				//cout << "Printing assumptions for pos unate : " << LA [0] << " " << LA [1] << " " << LA [2] << " " << LA [3] << endl;
				status = sat_solver_solve(pSat, LA, LA+2+numY, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0);
				//cout <<];


    }


}
*/

int main(int argc, char *argv[])
{
	// parseOptions(argc, (char**)argv);

	if(argc!=3 and argc!=4) {
		cerr << "Incorrect Arguments. Run as ./verify benchmark.qdimacs benchmark.syn.blif [varsFile = benchmark_vartoelim.txt]";
	}
	string benchmarkFile(argv[1]);
	string synFile(argv[2]);
	string varsOrderFile;
	if(argc == 4)
		varsOrderFile = string(argv[3]);
	else
		varsOrderFile = benchmarkFile.substr(0,benchmarkFile.find_last_of('.')) + "_varstoelim.txt";

	string fileName = benchmarkFile;
	string baseFileName = fileName.substr(fileName.find_last_of("/") + 1);  //Get the file name;
	baseFileName.erase(baseFileName.find (".qdimacs"), string::npos); //This contains the code for the raw file name;
	cout << "BaseName:     " << baseFileName << endl;

	Aig_Man_t* FAig;

    cout << "reading qdimacs file" << endl;
    const char* inputfilename = benchmarkFile.c_str();
    cout << "Building FAig..." << endl;
    vector<Aig_Obj_t*> varsXAig, varsYAig;
    FAig = QdimacsToAig(inputfilename, varsXAig, varsYAig);


    cout << " Built FAig " << endl;

    cout << "Building Syn_Aig..." << endl;;
	Abc_Ntk_t* FNtk = NULL;
    FNtk = blifToNtk(synFile);
    Aig_Man_t *synAig = Abc_NtkToDar(FNtk, 0, 0);

    cout << "Built Syn Aig " << endl;

    Aig_Man_t *synAigDup =  Aig_ManDupSimple(synAig);
    vector<int> semCheckVar;
    ifstream ifs(baseFileName + "semprop.txt");
    int var;
    if (ifs)
    {        
            while ( ifs >> var )
            {
                cout << "Sem property to be checked for " << var;
                semCheckVar.push_back (var);
            }
    }
    ifs.close();


	/*Nnf_Man nnfNew;
	nnfNew.init(synAig);
	Aig_Man_t* SAig = nnfNew.createAigWithoutClouds();

    for (int i = 0; 

    semCheck (synAig,  semCheckVar);
*/
	verifyConversion(synAig, FAig, varsXAig, varsYAig,  baseFileName);
    Aig_ManStop (FAig);
    Aig_ManStop (synAig);
}



