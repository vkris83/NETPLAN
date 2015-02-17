// --------------------------------------------------------------
//    NETSCORE Version 1
//    node.cpp -- Implementation of node functions
//    2009-2010 (c) Eduardo Ibáñez
//    2011-2014 (c) Venkat Krishnan (VK)
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include "node.h"

// Contructors and destructor for the Node class
Node::Node() : Properties(NodeDefault) {}

Node::Node(const Node& rhs) : Properties(rhs.GetVecStr()) {}

Node::~Node() {}

Node& Node::operator=(const Node& rhs) {
    Properties = rhs.GetVecStr();
    return *this;
}

// Read a node property in string format
string Node::Get(const string& selector) const {
    string temp_output;
	int index = FindNodeSelector(selector);
    if (index >= 0) temp_output = Properties[index];
    else {
        temp_output = "ERROR";
        printError("noderead", selector);
    };
    return temp_output;
};

// Read a property as a double
double Node::GetDouble(const string& selector) const {
	double output;
	output = (Get(selector) != "X") ? atof( Get(selector).c_str() ) : 0;
	return output;
}

// Copy the entire vector of properties
vector<string> Node::GetVecStr() const {
	return Properties;
};

// Modify a property
void Node::Set(const string& selector, const string& input){
	int index = FindNodeSelector(selector);
    if (index >= 0) Properties[index] = input;
    else printError("nodewrite", selector);
};

// Multiply stored values by 'value'
void Node::Multiply(const string& selector, const double value) {
	int index = FindNodeSelector(selector);
	if (index >= 0) {
		double actual = GetDouble(selector);
		if (actual != 0) {
			Set(selector, ToString<double>(actual * value));
		}
	} else printError("nodewrite", selector);
};

// Get what time the node belongs to (i.e., year)
int Node::Time() const {
	return Str2Step(Get("Step"))[0];
}



// ****** MPS output functions ******
string Node::NodeNames() const {
    string temp_output = "";
    // Create constraint for ach node with a valid demand
    if ( Get("Demand") != "X" && Get("Code")[0] != 'X' ) {
        temp_output += " E " + Get("Code") + "\n";
    } else {
        temp_output += " N " + Get("Code") + "\n";
    }

	//May 19 - addedd reserve - Venkat Krishnan
	if (Get("ContingencyReserve") != "X"){
		temp_output += " G RCR" + Get("Code") + "\n"; // Required Contin.Resrv 
		//temp_output += " G RRR" + Get("Code") + "\n"; // Required Replacement.Resrv 
		temp_output += " G RCRop" + Get("Code") + "\n"; // Required Contin.Resrv for operation - June 05 2012
	}

	//May 27 - Regulation - Venkat Krishnan
	if (Get("Regulation") != "X"){
		temp_output += " G RRU" + Get("Code") + "\n"; // Required Regulation up
	      temp_output += " G RRD" + Get("Code") + "\n"; // Required Regulation down
         } 

	/*if (Get("Ramp10") != "X"){ // July 4
		temp_output += " G RSR" + Get("Code") + "\n"; // Required Spinning Reserve
         } */

    return temp_output;
}

string Node::NodeUDColumns() const {
    string temp_output = "";
    // If unserved demand is allowed, write the appropriate cost
    if ( Get("CostUD") != "X" ) {
        temp_output += "    UD_" + Get("Code") + " obj " + Get("CostUD") + "\n";
        temp_output += "    UD_" + Get("Code") + " " + Get("Code") + " 1\n";
    }
    return temp_output;
}

string Node::NodePeakRows() const {
    string temp_output = "";
    // If peak demand is available, write the appropriate row
    if ( (Get("PeakPower") != "X") && isFirstinYear() ) {
			temp_output += " E pk" + Get("Code") + "\n";
			temp_output += " E rc" + Get("Code") + "\n"; // June 15 2011 - venkat RC1 INV
			temp_output += " E rp10" + Get("Code") + "\n"; // June 29 2011 - venkat RC10 INV
			temp_output += " E rp30" + Get("Code") + "\n"; // June 29 2011 - venkat RC30 INV
			temp_output += " E rp60" + Get("Code") + "\n"; // June 29 2011 - venkat RC60 INV
	}
    return temp_output;
}

string Node::NodeRMColumns() const {
    string temp_output = "";
    // If peak demand is available, write reserve margin variable
    if ( (Get("PeakPower") != "X") && isFirstinYear() ) {
        temp_output += "    RM_" + Get("Code") + " pk" + Get("Code") + " -" + Get("PeakPower") + "\n";
    }
    return temp_output;
}

string Node::NodeRMrcColumns() const { // INV June 22 2011
    string temp_output = "";
    // If peak demand is available, write reserve margin variable
    if ( (Get("PeakPower") != "X") && isFirstinYear() ) {
        temp_output += "    RMrc_" + Get("Code") + " rc" + Get("Code") + " -" + ToString<double>(GetDouble("Regulation")*Peak1) + "\n";
    }
    return temp_output;
}

string Node::NodeRMrp10Columns() const { // INV June 29 2011 ; 10 min. Ramp
    string temp_output = "";
    // If peak demand is available, write reserve margin variable
    if ( (Get("PeakPower") != "X") && isFirstinYear() ) {
        temp_output += "    RMrp10_" + Get("Code") + " rp10" + Get("Code") + " -" + ToString<double>((GetDouble("Ramp10")*Peak10)+ GetDouble("ContingencyReserve")) + "\n";
    }
    return temp_output;
}

string Node::NodeRMrp30Columns() const { // INV June 29 2011 ; 30 min. Ramp
    string temp_output = "";
    // If peak demand is available, write reserve margin variable
    if ( (Get("PeakPower") != "X") && isFirstinYear() ) {
        temp_output += "    RMrp30_" + Get("Code") + " rp30" + Get("Code") + " -" + ToString<double>((GetDouble("Ramp30")*Peak30)+ GetDouble("ContingencyReserve")) + "\n";
    }
    return temp_output;
}

string Node::NodeRMrp60Columns() const { // INV June 29 2011 ; 60 min. Ramp
    string temp_output = "";
    // If peak demand is available, write reserve margin variable
    if ( (Get("PeakPower") != "X") && isFirstinYear() ) {
        temp_output += "    RMrp60_" + Get("Code") + " rp60" + Get("Code") + " -" + ToString<double>(GetDouble("Ramp60")*Peak60) + "\n";
    }
    return temp_output;
}


string Node::NodeRMBounds() const {
    string temp_output = "";
    // If peak demand is available, write lower bound for reserve margin
    if ( (Get("PeakPower") != "X") && isFirstinYear() ) {
        temp_output += " LO bnd RM_" + Get("Code") + " 1\n";
    }
    return temp_output;
}

string Node::NodeRMrcBounds() const { // INV
    string temp_output = "";
    // If peak demand is available, write lower bound for reserve margin
    if ( (Get("PeakPower") != "X") && isFirstinYear() ) {
        temp_output += " LO bnd RMrc_" + Get("Code") + " 1\n";
    }
    return temp_output;
}

string Node::NodeRMrp10Bounds() const { // INV June 29 10 min. Ramp
    string temp_output = "";
    // If peak demand is available, write lower bound for reserve margin
    if ( (Get("PeakPower") != "X") && isFirstinYear() ) {
        temp_output += " LO bnd RMrp10_" + Get("Code") + " 1\n";
    }
    return temp_output;
}

string Node::NodeRMrp30Bounds() const { // INV June 29 30 min. Ramp
    string temp_output = "";
    // If peak demand is available, write lower bound for reserve margin
    if ( (Get("PeakPower") != "X") && isFirstinYear() ) {
        temp_output += " LO bnd RMrp30_" + Get("Code") + " 1\n";
    }
    return temp_output;
}

string Node::NodeRMrp60Bounds() const { // INV June 29 60 min. Ramp
    string temp_output = "";
    // If peak demand is available, write lower bound for reserve margin
    if ( (Get("PeakPower") != "X") && isFirstinYear() ) {
        temp_output += " LO bnd RMrp60_" + Get("Code") + " 1\n";
    }
    return temp_output;
}

string Node::NodeRhs() const {
    string temp_output = "";
    // Demand RHS if it's valid
    if ( (Get("Demand") != "X") && (Get("Demand") != "0") ) {
        temp_output = " rhs " + Get("Code") + " " + Get("Demand") + "\n";
    }
    return temp_output;
}

// May 20 Venkat krishnan - Contingency Reserve req.
string Node::CRRhs() const {
    string temp_output = "";
    // CR RHS if it's valid
    if (Get("ContingencyReserve") != "X") {
		temp_output = " rhs RCR" + Get("Code") + " " + ToString<double>(GetDouble("ContingencyReserve") * GetDouble("StepLength")) + "\n";
    }
    return temp_output;
}

// May 27 Venkat krishnan - Regulation req. OP INV
string Node::RegRhs() const {
    string temp_output = "";
    // Reg up and down RHS if it's valid
    if (Get("Regulation") != "X") {
		temp_output += " rhs RRU" + Get("Code") + " " + ToString<double>(GetDouble("Regulation") * GetDouble("StepLength") * 3) + "\n"; // May 30 - Up Reg - 3sigma // June 15
		temp_output += " rhs RRD" + Get("Code") + " " + ToString<double>(GetDouble("Regulation") * GetDouble("StepLength") * 3.5) + "\n"; // May 30 -  Down Regulation change
    }
    return temp_output;
}


// July 4 Venkat krishnan - Replacement Reserve req. for contingency
/*string Node::RRRhs() const {
    string temp_output = "";
    // CR RHS if it's valid
    if (Get("ContingencyReserve") != "X") {
		temp_output = " rhs RRR" + Get("Code") + " " + ToString<double>(GetDouble("ContingencyReserve") * GetDouble("StepLength")) + "\n";
    }
    return temp_output;
}*/


// July 4 - SR 10 min. Venkat krishnan - Spinning Reserve req. OP INV
/*string Node::SRRhs() const {
    string temp_output = "";
    // SR RHS if it's valid
    if (Get("Ramp10") != "X") {
		temp_output += " rhs RSR" + Get("Code") + " " + ToString<double>(GetDouble("Ramp10") * GetDouble("StepLength") * 3) + "\n"; // July 4 - SR 10 min.
    }
    return temp_output;
}*/


string Node::DCNodesBounds() const {
    string temp_output = "";
    // Write minimum and max for DC Power flow anges (-pi and pi)
    temp_output += " LO bnd th" + Get("Code") + " -3.14\n";
    temp_output += " UP bnd th" + Get("Code") + " 3.14\n";
    return temp_output;
}

// ****** Boolean functions ******
// Is Node a DC node?
bool Node::isDCelect() const {
    return ( Get("ShortCode").substr(0,2) == DCCode );
}

// Is Node and DC node and are we considering DC flow in the model?
bool Node::isDCflow() const {
    return isDCelect() && useDCflow;
}

// Is this the first node in a year?
bool Node::isFirstinYear() const {
	bool output = true;
	Step tempstep = Str2Step( Get("Step") );
	for ( unsigned int k = 1; k < SName.size(); k++) {
		output = output && ( (tempstep[k]==0) || (tempstep[k]==1) );
	}
	return output;
}

// ****** Other functions ******
// Find the index for a node property selector
int FindNodeSelector(const string& selector) {
	int index = -1;
	for (unsigned int k = 0; k < NodeProp.size(); ++k) {
		if (selector == NodeProp[k]) index = k;
	}
	return index;
}
