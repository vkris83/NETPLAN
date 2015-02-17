// --------------------------------------------------------------
//    NETSCORE Version 1
//    postprocess.cpp - Reading in and optimizing a problem, with minimum investment
//    2009-2010 (c) Eduardo Ibáñez
//    2011-2014 (c) Venkat Krishnan (VK)
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "netscore.h"
#include "solver.h"
#include <ilcplex/ilocplex.h>
ILOSTLBEGIN

int main () {
	printHeader("postprocessor");
	
	// Read global parameters
	ReadParameters("data/parameters.csv");
	
	// Set output level so that Benders steps are reported on screen
	if (outputLevel == 2 ) outputLevel = 1;
	
	// Enable reporting solution files
	reportSolution = true;
	
	// Import indices to export data
	ImportIndices();
	
	// Declare variables to store the optimization model
	CPLEX netplan;
	
	// Read master and subproblems
	netplan.LoadProblem();
	
	// Vector of capacity losses for events
	double events[(SLength[0] + IdxCap.GetSize()) * (Nevents+1)];
	ReadEvents(events, "prepdata/bend_events.csv");
	
	// Import minimum (not tested)
	// ImportMin( argv[1], model, var )
	//RandomMin( startInv );
	/*int inv = IdxCap.GetSize();
	if (useBenders) inv += SLength[0];
	for (int i=0; i < IdxInv.GetSize(); ++i) {
		model[0].add( var[0][inv] >= 2 );
		++inv;
	}*/
	
	// Solve problem
	double objective[Nobj];
	netplan.SolveIndividual( objective, events );
	
	cout << "- Values returned:" << endl;	
	for (int k = 0; k < Nobj; ++k){
		cout << "\t" << objective[k] << endl;
		FinalObjectives.push_back(objective[k]);
	}

	WriteOutput("prepdata/FinalObjectives.csv", FinalObjectives, "Cost/CO2/Resiliency"); // Write objectives into a file

	printHeader("completed");
	return 0;
}
