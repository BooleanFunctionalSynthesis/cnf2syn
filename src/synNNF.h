#ifndef SYN_NNF_H
#define SYN_NNF_H

#include "MainSolver.h"
#include <vector>
#include <set>
#include <map>
#include <assert.h>
#include "Basics.h"
#include "DecisionTree.h"
#include "InstanceGraph/InstanceGraph.h"

class synSolver: public CMainSolver
{
    vector <int> varsX;
    vector <int> varsY;
    vector <bool> tseitinClauses;
    vector <int> tseitinVars;
    vector <vector<int> > allClauses;
    vector <vector<int> > workingClauses;
    vector<bool> activeY;
    vector<bool> tseitinY;
    map<int, vector<int> > depAND;
    map<int, vector<int> > depOR;
    map<int, vector<int> > depXOR;
    set<int> depCONST;
    vector <set <int> > leaves; //Stores the leaves that a Tseitin Var depends on -- initially it stores only Y vars; while printing it stores both X and Y

    vector <vector<int> > onlyXClauses; // They do not take part in the decomposition to begin with. Need to be added later on.
//    set <int> activeYVars;
    string baseFileName;
    int numVars; //Sometimes readCnf introduces extraVars - so numVars > originalVarCount
//    int tseitinVars;

public:
    synSolver();
//bool createfromPrep( vector<vector<int> > &clauses, unsigned int nVars, vector<int>& varsY, set<int>& actY);
    void init (bool );
    void CreateSynNNF(vector<vector<int> > &clauses, vector<bool>& tcls, vector<int>& Xvar, vector<int>& Yvar,  int&,  vector<int>&, string, set<int> &, map<int, vector<int> > &, map<int, vector<int> > &, map<int, vector<int> > &, bool solved, bool dumpResult);
    bool recordRemainingComps() override;//made virtual for c2syn - SS
    bool findVSADSDecVar(LiteralIdT &theLit, const CComponentId & superComp) override;
	bool getComp(const VarIdT &theVar, const CComponentId &superComp, viewStateT lookUpCls[], viewStateT lookUpVars[]) override; //made virtual for c2syn - SS 
	bool decide() override;
 //   void getCompInputsAndTseitin(const CComponentId &superComp, viewStateT lookUpCls[], viewStateT lookUpVars[]);
    bool OnlyXandTseitin (const CComponentId & superComp);
//    unsigned int makeVariable(unsigned int VarNum);
    bool createfromPrep( vector<vector<int> > &clauses, unsigned int nVars); // vector<int> &varsY)
    void attachComponent ();
	string writeDTree(ofstream& ofs) ;
    void writeDSharp_rec(DTNode* node, ofstream& ofs, map<int, string> & visited, set<int>&, set<int> &, int &, vector<set <int> >&, map<string, string> &, set<string>& printT) ;
    void writeOPtoBLIF_rec(vector<string> &children, int op, ofstream& ofs, string out) ;
    void printSynNNF(bool);
    bool checkStructProp();

private:
	string getInputName(int var) ;
	string getInputNegName(int var) ;
	void instantiateOnOff(ofstream & ofs) ;
	string getIDName(int id) ;
    string writeOnlyX(ofstream & ofs, map<int, string> & visited, set<int>&);
	void    writeOFF(ofstream& ofs);
    void    writeON(ofstream& ofs);
    void	writeOR(ofstream& ofs);
    void	writeAND(ofstream& ofs);
    void	writeEqual(ofstream& ofs);
    void	writeNEG(ofstream& ofs);
    void	writeXOR(ofstream& ofs);
    void    writeComp(ofstream& ofs, string);
    //void processTseitins (vector < set<int> > & leaves, bool onlyY);
   // void DFS_collectLeaves(vector<set<int> >& graph, int node, vector <set <int> > & leaves, bool visited[], bool onlyY);
    void processTseitins (bool onlyY);
    void DFS_collectLeaves(vector<set<int> >& graph, int node,  bool visited[], bool onlyY);
    void printTseitinModules (ofstream& ofs, vector <set <int> > & leaves);
    void printTseitinModulesForOperators (ofstream & ofs,  vector<set <int> > & leaves, int op, int Tvar, vector <int>& dependents);
    bool checkStructProp_rec ( DTNode *node, map <int, set<int> >& visited, bool, ofstream& ofs);
    void printSet(set <int>  &s, string desc );
    string printTseitinSkolemFunction (ofstream &ofs, int varNum, vector <set <int> >& leaves);
    void printConstsAndTseitins(ofstream &of, vector <set <int> >& leaves);
    string writeClause (ofstream &ofs, int);
    bool containsActiveY (int var);
};
#endif
