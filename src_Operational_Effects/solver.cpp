// --------------------------------------------------------------
//    NETSCORE Version 1
//    solver.cpp -- Implementation of solver functions
//    2009-2010 (c) Eduardo Ibáñez
//    2011-2014 (c) Venkat Krishnan (VK)
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include "index.h"
#include "read.h"
#include "write.h"
#include "solver.h"

// Loads the problem from MPS files into memory
void CPLEX::LoadProblem() {
	cout << "- Reading problem..." << endl;
	
	try {
		int nyears = SLength[0];
		
		for (int i=0; i <= nyears ; ++i) {
			model.add( IloModel(env) );// what?
			cplex.add ( IloCplex(env) );
			obj.add( IloObjective(env) );
			var.add( IloNumVarArray(env) );
			rng.add( IloRangeArray(env) );
		}
		
		// Read MPS files
		for (int i=0; i <= nyears ; ++i) {
			string file_name = "";
			if ( !useBenders && (i == 0) ) {
				file_name = "prepdata/netscore.mps";
			} else {
				file_name = "prepdata/bend_" + ToString<int>(i) + ".mps";
			}
			if (i!=0) {
				//cplex[i].setParam(IloCplex::PreInd,0);
				//cplex[i].setParam(IloCplex::ScaInd,-1); 
				cplex[i].setParam(IloCplex::RootAlg, IloCplex::Dual); // what?
			}
			if (outputLevel > 0 ) {
				cplex[i].setOut(env.getNullStream()); // what?
			} else {
				cout << "Reading " << file_name << endl;
			}
			cplex[i].importModel(model[i], file_name.c_str(), obj[i], var[i], rng[i]); // what?
			cplex[i].extract(model[i]); // what?
		}
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

// Solves current model
void CPLEX::SolveIndividual( double *objective, const double events[], string & returnString ) {
	int nyears = SLength[0];
	
	try { 
		// Store temporary master cuts
		IloRangeArray MasterCuts(env, 0);
		
		// Constraints to apply capacities to subproblems
		IloArray<IloRangeArray> CapCuts(env, 0);
		for (int i=1; i <= nyears; ++i) {
			IloRangeArray tempcon(env, 0);
			CapCuts.add( tempcon );
		}
		// Store constraints that later will be used to apply capacities what??
		vector<int> copied(nyears, 0);
		for (int i=0; i < IdxCap.GetSize(); ++i) {
			int year = IdxCap.GetYear(i);
			CapCuts[year-1].add( var[year][copied[year-1]] <= 0 );
			++copied[year-1];
		}
		for (int i=1; i<= nyears; ++i)
			model[i].add( CapCuts[i-1] );
		
		// Keep track of solution
		bool optimal = true; // IloNumArray solution(env, 0);
		
		if ( !useBenders ) {
			// Only one file
			if (outputLevel < 2 ) cout << "- Solving problem" << endl;
			
			if ( cplex[0].solve() ) {
				optimal = true;
				objective[0] = cplex[0].getObjValue();
				cplex[0].getValues(solution, var[0]);
			} else {
				optimal = false;
			}
		} else {
			// Use Benders decomposition
			int OptCuts = 1, FeasCuts = 1, iter = 0;
			IloNumArray mastersol(env);
			
			// Temporary variables to store dual information
			IloArray<IloNumArray> dualcap(env, 0);
			for (int i=1; i <= nyears; ++i) {
				IloNumArray temparr(env, 0);
				dualcap.add(temparr);
			}
			
			while ((OptCuts+FeasCuts > 0) && ( iter <= 1000)) {		
				++iter; OptCuts = 0; FeasCuts = 0;
				
				// Keep track of necessary cuts
				bool status[nyears];
				IloExprArray expr_cut(env, nyears);
				IloNumArray dual(env);
				
				// cplex[0].exportModel("temp.lp");
				
				// Solve master problem. If master is infeasible, exit loop
				if (outputLevel < 2 ) cout << "- Solving master problem (Iteration #" << iter << ")" << endl;
				if ( !cplex[0].solve() ) {
					break;
				}
				
				// Recover variables (first nyears are estimated obj. val)
				cplex[0].getValues(mastersol, var[0]);
				
				// Store capacities as constraints
				CapacityConstraints(CapCuts, events, 0, mastersol, nyears);
				
				// Start subproblems
				if (outputLevel < 2 ) cout << "- Solving subproblems" << endl << "  ";
				
				for (int j=1; j <= nyears; ++j) {
					// Solve subproblem
					cplex[j].solve();
						
					if (cplex[j].getCplexStatus() != CPX_STAT_OPTIMAL) {
						// If subproblem is infeasible, create feasibility cut
						++FeasCuts; status[j-1] = true;
						
						// Change solver properties to find dual unbouded ray
						cplex[j].setParam(IloCplex::PreInd,0);
						cplex[j].setParam(IloCplex::ScaInd,-1); 
						cplex[j].setParam(IloCplex::RootAlg, IloCplex::Primal);
						cplex[j].solve();
						
						IloExpr temp(env); expr_cut[j-1] = temp;
						cplex[j].getDuals(dual, rng[j]);
						for (int k=0; k < dual.getSize(); ++k)
							expr_cut[j-1] += dual[k] * rng[j][k].getUB();
						cplex[j].getDuals(dualcap[j-1], CapCuts[j-1]);
						
						if (outputLevel < 2 ) cout << "f" << j << " ";
					} else if (mastersol[j-1] <= cplex[j].getObjValue() * 0.999 ) {
						// If cost is underestimated, create optimality cut
						++OptCuts; status[j-1] = true;
						expr_cut[j-1] = - var[0][j-1];
						cplex[j].getDuals(dual, rng[j]);
						for (int k=0; k < dual.getSize(); ++k)
							expr_cut[j-1] += dual[k] * rng[j][k].getUB();
						cplex[j].getDuals(dualcap[j-1], CapCuts[j-1]);
						
						if (outputLevel < 2 ) cout << j << " ";
					} else {
						status[j-1] = false;
					}
				}
				
				if (OptCuts+FeasCuts > 0) {
					// Finalize cuts
					vector<int> copied(nyears, 0);
					for (int i=0; i < IdxCap.GetSize(); ++i) {
						int year = IdxCap.GetYear(i);
						if ( status[year-1] )
							expr_cut[year-1] += dualcap[year-1][copied[year-1]] * var[0][nyears + i];
						++copied[year-1];
					}
					
					// Apply cuts to master
					for (int j=1; j <= nyears; ++j) {
						if (status[j-1]) {
							MasterCuts.add( expr_cut[j-1] <= 0 );
							string constraintName = "Cut_y" + ToString<int>(j) + "_iter" + ToString<int>(iter);
							MasterCuts[MasterCuts.getSize()-1].setName( constraintName.c_str() );
							model[0].add( MasterCuts[MasterCuts.getSize()-1] );
						}
						// Reset solver properties
						if (cplex[j].getCplexStatus() != CPX_STAT_OPTIMAL) {
							cplex[j].setParam(IloCplex::PreInd,1);
							cplex[j].setParam(IloCplex::ScaInd,0); 
							cplex[j].setParam(IloCplex::RootAlg, IloCplex::Dual);
						}
					}
				} else {
					// Store solution if optimal solution found
					StoreSolution();
				}
				
				if (outputLevel < 2 ) {
					if (OptCuts+FeasCuts == 0) cout << "No cuts - Optimal solution found!";
					cout << endl;
				}
			}
			
			if (cplex[0].getCplexStatus() == CPX_STAT_OPTIMAL) {
				optimal = true;
				objective[0] = cplex[0].getObjValue();
			} else {
				optimal = false;
			}
		}
		
		if ( !optimal ) {
			// Solution not found, return very large values
			cout << "\tProblem infeasible!" << endl;
			for (int i=0; i < Nobj; ++i )
				objective[i] = 1.0e30;
		} else {
			if ( reportSolution )
				cout << "\tCost: " << objective[0] << endl;
			
			int startCap = 0;
			int startInv = startCap + IdxCap.GetSize();
			int startEm = startInv + IdxInv.GetSize();
			int startRm = startEm + IdxEm.GetSize();

                  // INV new variable June 22
			int startRmrc = startRm + IdxRm.GetSize();

                  // INV new variable June 29 - Ramps - 10, 30, 60 mins.
			int startRmrp10 = startRmrc + IdxRmrc.GetSize();
			int startRmrp30 = startRmrp10 + IdxRmrp10.GetSize();
			int startRmrp60 = startRmrp30 + IdxRmrp30.GetSize();

			int startArc = startRmrp60 + IdxRmrp60.GetSize(); // INV
			int startUd = startArc + IdxArc.GetSize();

  			int startRC10 = startUd + IdxUd.GetSize(); // INV June 29 Ramps
			int startRC30 = startRC10 + IdxRC10.GetSize(); // INV June 29 Ramps
			int startRC60 = startRC30 + IdxRC30.GetSize(); // INV June 29 Ramps

			// Insert new vaiable index - July 4 2011 Venkat Krishnan
                  int startCR = startRC60 + IdxRC60.GetSize(); // Contingency Reserve
				  int startCRop = startCR + IdxCR.GetSize(); // June 05 2012 - CRop 
                  int startRU = startCRop + IdxCRop.GetSize(); // Regulation Up - May 30 // June 05 2012 - CRop
                  int startRD = startRU + IdxRU.GetSize(); // Regulation Down - May 30 
			//int startSR = startRD + IdxRD.GetSize(); 
			//int startRR = startSR + IdxSR.GetSize(); 

			/*int startCyE = startRR + IdxRR.GetSize(); // Cycling Aug 12 2011
			int startCyCR = startCyE + IdxCyE.GetSize(); 
			int startCyRU = startCyCR + IdxCyCR.GetSize(); 
			int startCyRD = startCyRU + IdxCyRU.GetSize(); 
			int startCySR = startCyRD + IdxCyRD.GetSize(); 
			int startCyRR = startCySR + IdxCySR.GetSize();*/

			int startDc = startRD + IdxRD.GetSize();
			
			// Sustainability metrics
			vector<double> emissions = SumByRow(solution, IdxEm, startEm);
			for (int i=0; i < SustObj.size(); ++i) {
				// Print results on screen
				if ( reportSolution ) {
					if (SustObj[i] == "EmCO2" || SustObj[i] == "CO2") {
						cout << "\t" << SustObj[i] << ": ";
						cout << EmissionIndex(solution, startEm + SLength[0]*i);
						cout << " (Sum: " << emissions[i] << ")" << endl;
					} else {
						cout << "\t" << SustObj[i] << ": " << emissions[i] << endl;
					}
				}
				
				// Return sustainability metric
				objective[1+i] = emissions[i];
			}
			
			// Report solutions (for postprocessor)
			if ( reportSolution ) {
				vector<string> solstring(0);
				for (int i=0; i < solution.getSize(); ++i)
					solstring.push_back( ToString<IloNum>(solution[i]) );
				
				WriteOutput("prepdata/post_emissions.csv", IdxEm, solstring, startEm, "% Emissions");
				WriteOutput("prepdata/post_node_rm.csv", IdxRm, solstring, startRm, "% Reserve margins");
				
				WriteOutput("prepdata/post_node_rmrc.csv", IdxRmrc, solstring, startRmrc, "% Ramp margins"); // INV

				WriteOutput("prepdata/post_arc_inv.csv", IdxInv, solstring, startInv, "% Investments");
				WriteOutput("prepdata/post_arc_cap.csv", IdxCap, solstring, startCap, "% Capacity");
				WriteOutput("prepdata/post_arc_flow.csv", IdxArc, solstring, startArc, "% Arc flows");
				WriteOutput("prepdata/post_node_ud.csv", IdxUd, solstring, startUd, "% Demand not served at nodes");
				
				// Insert new variable - June 29 2011 Venkat Krishnan
				WriteOutput("prepdata/post_node_RC10.csv", IdxRC10, solstring, startRC10, "% 10m Ramp margins"); // INV jUNE 29
				WriteOutput("prepdata/post_node_RC30.csv", IdxRC30, solstring, startRC30, "% 30m Ramp margins"); // INV jUNE 29
				WriteOutput("prepdata/post_node_RC60.csv", IdxRC60, solstring, startRC60, "% 60m Ramp margins"); // INV jUNE 29
				WriteOutput("prepdata/post_node_rmrp10.csv", IdxRmrp10, solstring, startRmrp10, "% 10 min. Ramp margins"); // INV
				WriteOutput("prepdata/post_node_rmrp30.csv", IdxRmrp30, solstring, startRmrp30, "% 30 min. Ramp margins"); // INV
				WriteOutput("prepdata/post_node_rmrp60.csv", IdxRmrp60, solstring, startRmrp60, "% 60 min. Ramp margins"); // INV

				
				// Insert new variable - May 23 2011 Venkat Krishnan
                   	WriteOutput("prepdata/post_arc_cr.csv", IdxCR, solstring, startCR, "% Generator Contingency Reserve");
					WriteOutput("prepdata/post_arc_crop.csv", IdxCRop, solstring, startCRop, "% Generator Operational Contingency Reserve");
				WriteOutput("prepdata/post_arc_ru.csv", IdxRU, solstring, startRU, "% Generator Up Regulation"); // May 30
				WriteOutput("prepdata/post_arc_rd.csv", IdxRD, solstring, startRD, "% Generator Down Regulation");
				//WriteOutput("prepdata/post_arc_sr.csv", IdxSR, solstring, startSR, "% Generator 10 min. Spinning Reserve"); // July 4 OP INV
				//WriteOutput("prepdata/post_arc_rr.csv", IdxRR, solstring, startRR, "% Generator Replacemenr Reserve");

				// Insert new variable - Aug 12 2011 Venkat Krishnan
                   	/*WriteOutput("prepdata/post_arc_cye.csv", IdxCyE, solstring, startCyE, "% Generator cycling due to Energy");
                   	WriteOutput("prepdata/post_arc_cycr.csv", IdxCyCR, solstring, startCyCR, "% Generator cycling due to CR");
                   	WriteOutput("prepdata/post_arc_cyru.csv", IdxCyRU, solstring, startCyRU, "% Generator cycling due to RU");
                   	WriteOutput("prepdata/post_arc_cyrd.csv", IdxCyRD, solstring, startCyRD, "% Generator cycling due to RD");
                   	WriteOutput("prepdata/post_arc_cysr.csv", IdxCySR, solstring, startCySR, "% Generator cycling due to SR");
                   	WriteOutput("prepdata/post_arc_cyrr.csv", IdxCyRR, solstring, startCyRR, "% Generator cycling due to RR");*/
			}
			
			// Write emissions and investments in output string (only for NSGA-II postprocessor
			if (returnString != "skip") {
				returnString = "";
				
				// Write total emissions
				for (int i=0; i < SustMet.size(); ++i)
					returnString += "," + ToString<double>( emissions[i] );
				
				// Write investments
				vector<double> Investments = SumByRow(solution, IdxInv, startInv);
				for (int j=0; j < Investments.size(); ++j)
					returnString += "," + ToString<double>( Investments[j] );
			}
			
			
			// Resiliency calculations
			if (Nevents > 0) {
				bool ResilOptimal = true;
				double ResilObj[Nevents], resiliency = 0;
				int startPos = IdxCap.GetSize() * (Nevents + 1);
				
				// Evaluate all the events and obtain operating cost
				if (outputLevel < 2 ) cout << "- Solving resiliency..." << endl;
				
				// Initialize operational cost
				for (int event=1; event <= Nevents; ++event)
					ResilObj[event-1] = 0;
				
				for (int j=1; j <= nyears; ++j) {
					if ( events[startPos + (j-1) * (Nevents+1)] == 1 ) {
						// If Benders is used, the operational cost is already available
						if (!useBenders) {
							// Solve subproblem
							CapacityConstraints(CapCuts, events, 0, solution, 0);
							cplex[j].solve();
						}
						
						for (int event=1; event <= Nevents; ++event)
							if ( events[startPos + (j-1) * (Nevents+1) + event] == 1 )
								ResilObj[event-1] -= cplex[j].getObjValue();
					}
				}
				
				for (int event=1; event <= Nevents; ++event) {
					bool current_feasible = true;
					
					// Store capacities as constraints
					CapacityConstraints(CapCuts, events, event, solution, 0);
					
					for (int j=1; (j <= nyears) & (current_feasible); ++j) {
						if ( events[startPos + (j-1) * (Nevents+1) + event] == 1 ) {
							// Solve subproblem
							cplex[j].solve();
							
							if (cplex[j].getCplexStatus() != CPX_STAT_OPTIMAL) {
								// If subproblem is infeasible
								ResilObj[event-1] = 1.0e10;
								ResilOptimal = false;
								current_feasible = false;
								if (outputLevel < 2 ) cout << "\t\tEv: " << event << "\tYr: " << j << "\tInfeasible!" << endl;
							} else {
								// If subproblem is feasible
								ResilObj[event-1] += cplex[j].getObjValue();
								if (outputLevel < 2 ) cout << "\t\tEv: " << event << "\tYr: " << j << "\tCost: " << cplex[j].getObjValue() << endl;
							}
						}
					}
				}
				
				if (ResilOptimal) {
					// Calculate resiliency results
					for (int j = 0; j < Nevents; ++j) {
						resiliency += ResilObj[j];
					}
					objective[SustObj.size() + 1] = resiliency / Nevents;
					if ( reportSolution )
						cout << "\tResiliency: " << resiliency / Nevents << endl;
				} else {
					objective[SustObj.size() + 1] = 1.0e9;
					if ( reportSolution )
						cout << "\tResiliency infeasible!" << endl;
				}
				
				// Report resiliency results
				if (returnString != "skip") {
					string tempString = "";
					
					for (int event=0; event < Nevents; ++event)
						tempString += "," + ToString<double>( ResilObj[event] );
					
					returnString = tempString + returnString;
				}
			}
		}
		
		// Erase cuts created with Benders
		model[0].remove( MasterCuts );
		MasterCuts.end();
		
		// Erase capacities from subproblems
		for (int j=1; j <= nyears; ++j)
			model[j].remove( CapCuts[j-1] );
		CapCuts.end();
		
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

void CPLEX::SolveIndividual( double *objective, const double events[] ) {
	string skipString = "skip";
	SolveIndividual( objective, events, skipString );
}

// Store complete solution vector
void CPLEX::StoreSolution() {
	int nyears = SLength[0];
	solution.clear();
	
	try {
		if ( !useBenders ) {
			// Only one file
			cplex[0].getValues(solution, var[0]);
		} else {
			// Multiple files (Benders decomposition)
			IloArray<IloNumArray> varsol(env, 0);
			for (int i=0; i <= nyears; ++i) {
				IloNumArray temp(env);
				cplex[i].getValues(temp, var[i]);
				varsol.add(temp);
			}
			
			// The following array keeps track of what has already been copied
			vector<int> position(nyears+1, 0);
			position[0] = nyears;
			
			// Recover capacities
			for (int j = 0; j < IdxCap.GetSize(); ++j) {
				int tempYear = IdxCap.GetYear(j);
				solution.add( varsol[0][position[0]] );
				++position[0]; ++position[tempYear];
			}
			
			// Recover investments
			for (int j = 0; j < IdxInv.GetSize(); ++j) {
				solution.add( varsol[0][position[0]] );
				++position[0];
			}
			
			// Recover sustainability metrics
			for (int j = 0; j < IdxEm.GetSize(); ++j) {
				int tempYear = IdxArc.GetYear(j);
				solution.add( varsol[tempYear][position[tempYear]] );
				++position[tempYear];
			}
			
			// Recover reserve margin
			for (int j = 0; j < IdxRm.GetSize(); ++j) {
				solution.add( varsol[0][position[0]] );
				++position[0];
			}
			
			// Recover flows
			for (int j = 0; j < IdxArc.GetSize(); ++j) {
				int tempYear = IdxArc.GetYear(j);
				solution.add( varsol[tempYear][position[tempYear]] );
				++position[tempYear];
			}
			
			// Recover unserved demand
			for (int j = 0; j < IdxUd.GetSize(); ++j) {
				int tempYear = IdxUd.GetYear(j);
				solution.add( varsol[tempYear][position[tempYear]] );
				++position[tempYear];
			}
			
// Insert new variables - May 23 2011 Venkat Krishnan

	/*		// Recover Contingency reserves - May 23 2011
			for (int j = 0; j < IdxCR.GetSize(); ++j) {
				int tempYear = IdxCR.GetYear(j);
				solution.add( varsol[tempYear][position[tempYear]] );
				++position[tempYear];
			}

			// Up Regulation - May 30 2011
			for (int j = 0; j < IdxRU.GetSize(); ++j) {
				int tempYear = IdxRU.GetYear(j);
				solution.add( varsol[tempYear][position[tempYear]] );
				++position[tempYear];
			}

			// Down Regulation - May 30 2011
			for (int j = 0; j < IdxRD.GetSize(); ++j) {
				int tempYear = IdxRD.GetYear(j);
				solution.add( varsol[tempYear][position[tempYear]] );
				++position[tempYear];
			} */

			// Recover DC angles
			for (int j = 0; j < IdxDc.GetSize(); ++j) {
				int tempYear = IdxDc.GetYear(j);
				solution.add( varsol[tempYear][position[tempYear]] );
				++position[tempYear];
			}
		}
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

// Function called by the NSGA-II method. It takes the minimum investement (x) and calculates the metrics (objective)
void CPLEX::SolveProblem(double *x, double *objective, const double events[]) {
	// Start of investment variables
	int startInv = IdxCap.GetSize();
	if ( useBenders ) startInv += SLength[0];
	
	// Force minimum investment (x) as lower bound
	IloRangeArray ConstrLB(env, 0);
	for (int i = 0; i < IdxInv.GetSize(); ++i)
		ConstrLB.add( var[0][startInv + i] >= x[i] );
	model[0].add( ConstrLB );
	
	// Solve problem
	SolveIndividual( objective, events );
	
	// Eliminate lower bound constraints
	model[0].remove( ConstrLB );
	ConstrLB.end();
}

// Apply capacities from master to subproblems
void CapacityConstraints(IloArray<IloRangeArray>& Cuts, const double events[], const int event, const IloNumArray mastersol, const int offset) {
	int nyears = SLength[0];
	
	try {
		// Apply capacities and store in the constraint arrays
		vector<int> copied(nyears, 0);
		for (int i=0; i < IdxCap.GetSize(); ++i) {
			int year = IdxCap.GetYear(i);
			IloNum rhs = events[i * (Nevents+1) + event] * mastersol[offset + i];
			Cuts[year-1][copied[year-1]].setUB( rhs );
			++copied[year-1];
		}
	} catch (IloException& e) {
		cerr << "Concert exception caught: " << e << endl;
	} catch (...) {
		cerr << "Unknown exception caught" << endl;
	}
}

double EmissionIndex(const IloNumArray& v, const int start) {
	// This function calculates an emission index
	double em_zero = v[start], max = v[start], min = v[start], reduction = 0.02 * v[start], increase = 0.02, sum = 0;
	int first_year = 5, j = 0;
	vector<double> index(0);
	
	for (int i = 1; i < SLength[0]; ++i) {
		// max: worst case scenario emissions
		max = max * (1 + increase);
		// min: best case scenario emissions
		min -= reduction;
		if (i > first_year) {
			// Find index for the emissions at year i and carry a sum
			sum += (v[start+i]-min)/(max-min);
			++j;
		}
	}
	// Find average value for the index
	return sum/j;	
}

vector<double> SumByRow(const IloNumArray& v, Index Idx, const int start) {
	// This function sums each row for an index across years
	int last_index = -1, j=0;
	double sum = 0;
	vector<double> result(0);
	
	for (int i = 0; i < Idx.GetSize(); ++i) {
		if ( (last_index != Idx.GetPosition(i)) && (last_index != -1) ) {
			result.push_back(sum);
			sum = 0; j=0;
			last_index = Idx.GetPosition(i);
		} else {
			if (last_index == -1)
				last_index = Idx.GetPosition(i);
			sum += v[start + i];
			++j;
		}
	}
	result.push_back(sum);
	
	return result;	
}


// Resets models to improve memory management
/* void ResetProblem(IloArray<IloModel>& model, IloArray<IloCplex>& cplex) {
	for (int i=0; i <= SLength[0]; ++i)
		cplex[i].clearModel();
	system("free");
	for (int i=0; i <= SLength[0]; ++i)
		cplex[i].extract(model[i]);
	system("free");
} */


/*
// Import Minimum investment into the model from file (not tested)
void ImportMin( const char* filename, const int MstartInv ) {
	int inv = MstartInv;
	
	FILE *file;
	char line [ 200 ];
	
	file = fopen(filename, "r");
	if ( file != NULL ) {
		for (;;) {
			// Read a line from the file and finish if empty is read
			if ( fgets(line, sizeof line, file) == NULL )
				break;
			double d1;
			d1 = strtod(line, NULL);
			model[0].add( var[0][inv] >= d1 );
			++inv;
		}
	}
}

*/
