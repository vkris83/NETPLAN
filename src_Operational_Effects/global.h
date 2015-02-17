// --------------------------------------------------------------
//    NETSCORE Version 1
//    global.h -- Definition of global variables and functions
//    2009-2010 (c) Eduardo Ibáñez
//    2011-2014 (c) Venkat Krishnan (VK)
// --------------------------------------------------------------

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <string>
#include <sstream>
#include "step.h"
#include "index.h"

// Type definitions
typedef vector<string> VectorStr;
typedef vector<VectorStr> MatrixStr;

// Global variables
extern const int CharLine;
extern string SName;
extern Step SLength;
extern bool useDCflow, useBenders, reportSolution;
extern string DefStep, StorageCode, DCCode, TransStep, TransDummy, TransCoal;
extern int Npopsize, Nngen, Nobj, Nevents;
extern string Npcross_real, Npmut_real, Neta_c, Neta_m, Npcross_bin, Npmut_bin, Nstages;
extern double Np_start, ELReg,ELReg10, ELReg30, ELReg60, Wbase, Lbase, Peak1 , Peak10 , Peak30 , Peak60, CycleReg, CycleCO2, CycleSO2, CycleNOx; // May 30 2011 - regulation variables INV// Aug 10 cycles
extern vector<string> ArcProp, ArcDefault, NodeProp, NodeDefault, TransInfra, TransComm, StepHours, SustObj, SustMet, Regnode, Regnode10, Regnode30, Regnode60; // May 28; INV
extern int NodePropOffset, ArcPropOffset, outputLevel;
extern vector<double> Regvalue, Regvalue10, Regvalue30, Regvalue60; // May 28 Regulation VK INV

// Store indices to recover data after optimization
extern Index IdxNode, IdxUd, IdxRm, IdxArc, IdxInv, IdxCap, IdxUb, IdxEm, IdxDc, IdxRmrc, IdxRmrp10, IdxRC10, IdxRC30, IdxRmrp30,IdxRmrp60, IdxRC60, IdxCR, IdxCRop, IdxRU, IdxRD; // , IdxSR, IdxRR,IdxCyE, IdxCyCR, IdxCyRU, IdxCyRD, IdxCySR, IdxCyRR; // May 23 2011 VK - Comntingency Reserve; May 27 - Regulation INV // Aug 11 cycling // June 05 2012 CRop

// Print error messages
void printError(const string& selector, const char* fileinput);
void printError(const string& selector, const string& field);

// Print header at the beginning of execution
void printHeader(const string& selector);

// Remove comments and end of line characters
void CleanLine(char* line);

// Convert a value to a string
template <class T>
string ToString(T t) {
	stringstream ss;
	ss << t;
	return ss.str();
}

#endif  // _GLOBAL_H_
