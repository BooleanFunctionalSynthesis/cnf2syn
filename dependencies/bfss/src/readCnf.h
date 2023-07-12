#ifndef READ_CNF_H
#define READ_CNF_H

#include <iostream>
#include <cstdio>
#include <fstream>
#include <cassert>
#include <string.h>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <algorithm>
#include <boost/range/adaptor/reversed.hpp>

using namespace std;

#define VERILOG_HEADER "// Generated using findDep.cpp \n"
#define MAX_DEP_SIZE 50


extern vector<vector<int> > allClauses;
extern vector<bool> tseitinClauses;
extern vector<bool> prevTseitinClauses;
extern vector<int> varsX;
extern vector<int> varsY;
extern vector<int> tseitinVars;
extern vector<int> unates;
extern int numClauses;

extern int numVars, numClauses;
extern int numA, numE;
extern map<int, vector<int> > depAND;
extern map<int, vector<int> > depOR;
extern map<int, vector<int> > depXOR;
extern int origNumVars;

#endif
