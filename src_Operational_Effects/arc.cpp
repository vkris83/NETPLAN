// --------------------------------------------------------------
//    NETSCORE Version 1
//    arc.cpp -- Implementation of arc functions
//    2009-2010 (c) Eduardo Ibáñez
//    2011-2014 (c) Venkat Krishnan (VK)
// --------------------------------------------------------------

using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include "arc.h"

// Contructors and destructor for the Arc class
Arc::Arc() :
    Properties(ArcDefault),
    Energy2Trans(false),
    Trans2Energy(0) {}

Arc::Arc(const Arc& rhs) :
    Properties(rhs.GetVecStr("Properties")),
    Energy2Trans(rhs.GetBool("Energy2Trans")),
    Trans2Energy(rhs.GetVecStr("Trans2Energy")) {}

// This constructor creates an arc going in the opposite direction
Arc::Arc(const Arc& rhs, const bool reverse) :
    Properties(rhs.GetVecStr("Properties")),
    Energy2Trans(rhs.GetBool("Energy2Trans")),
    Trans2Energy(rhs.GetVecStr("Trans2Energy")) {
		if (reverse) {
			if ( !isTransport() ) {
				string temp = Get("To");
				Set("To", Get("From"));
				Set("From", temp);
			} else {
				string temp = Get("From");
				Set( "From", temp.substr(0,2) + temp.substr(4,2) + temp.substr(2,2) );
				temp = Get("To");
				Set( "To", temp.substr(0,2) + temp.substr(4,2) + temp.substr(2,2) );
			}
		}
	}
	
Arc::~Arc() {}

Arc& Arc::operator=(const Arc& rhs) {
    Properties = rhs.GetVecStr("Properties");
    Energy2Trans = rhs.GetBool("Energy2Trans");
    Trans2Energy = rhs.GetVecStr("Trans2Energy");
    return *this;
}

// Read a property in string format
string Arc::Get(const string& selector) const {
    string temp_output;
	int index = FindArcSelector(selector);
    if (index >= 0) temp_output = Properties[index];
	else {
        temp_output = "ERROR";
        printError("arcread", selector);
    }
    return temp_output;
}

// Read the year from the step
string Arc::GetYear() const {
	int temp = Str2Step(Get("FromStep"))[0];
	return SName.substr(0,1) + ToString<int>(temp);
}

// Read a property and convert to double
double Arc::GetDouble(const string& selector) const {
	double output;
	output = (Get(selector) != "X") ? atof( Get(selector).c_str() ) : 0;
	return output;
}

// Read a boolean property
bool Arc::GetBool(const string& selector) const {
    bool temp_output;
    if (selector == "Energy2Trans") temp_output = Energy2Trans;
    else printError("arcread", selector);
	
    return temp_output;
};

// Read an entire vector of string properties
vector<string> Arc::GetVecStr(const string& selector) const {
    vector<string> temp_output;
    if (selector == "Properties") temp_output = Properties;
    else if (selector == "Trans2Energy") temp_output = Trans2Energy;
    else printError("arcread", selector);
    return temp_output;
};

// Modify a propery
void Arc::Set(const string& selector, const string& input){
	int index = FindArcSelector(selector);	
    if (index >= 0) Properties[index] = input;
    else printError("arcwrite", selector);
};

// Modify a boolean property
void Arc::Set(const string& selector, const bool input){
    if (selector == "Energy2Trans") Energy2Trans = input;
    else printError("arcwrite", selector);
};

// Add a new line to the Trans2Energy vector, where the consumption of energy
// for a transportation link is stored
void Arc::Add(const string& selector, const string& input) {
    if (selector == "Trans2Energy") Trans2Energy.push_back(input);
    else printError("arcwrite", selector);
};

// Multiply a value or a vector by a given value
void Arc::Multiply(const string& selector, const double value) {
	int index = FindArcSelector(selector);
	if (selector == "Trans2Energy") {
		// Adjust values
		for (unsigned int i = 1; i < Trans2Energy.size(); i += 2) {
			Trans2Energy[i] = ToString<double>( value * atof(Trans2Energy[i].c_str()) );
		}
	} else if (index >= 0) {
		double actual = GetDouble(selector);
		if (actual != 0) {
			Set(selector, ToString<double>(actual * value));
		}
	} else printError("arcwrite", selector);
};


// ****** MPS output functions ******
string Arc::ArcUbNames() const {
    string temp_output = "";
    // Create upper bound constraint
	if ( isTransport()  && Get("TransInfr") == "" ) {
		// Transportation arc
		if ( Get("OpMax") != "Inf" ) {
			temp_output += " L ub" + Get("Code") + "\n";
		} else {
			temp_output += " N ub" + Get("Code") + "\n";
		}
	} else if ( !isTransport() ) {
		// Energy arc
		if ( Get("OpMax") != "Inf" ) {
			temp_output += " L ub" + Get("Code") + "\n";
			if (Get("RampRate")!="0"){         //May 19 - addedd reserve - Venkat Krishnan
				temp_output += " L ubR" + Get("Code") + "\n"; // CR (May 19) and RU (May 27), and RR, SR (July 4) added to energy
				temp_output += " G lbR" + Get("Code") + "\n"; // RD (May 27) added to energy
				
				temp_output += " L ubRop" + Get("Code") + "\n"; // CR op. < CR allocated - June 05 2012

				temp_output += " L U1CR" + Get("Code") + "\n"; // Ramp rate constraint for CR
				temp_output += " L U2CR" + Get("Code") + "\n"; // Ramp rate constraint for CR

				temp_output += " L U1RU" + Get("Code") + "\n"; // Ramp rate constraint for Up Regulation - May 27
				temp_output += " L U1RD" + Get("Code") + "\n"; // Ramp rate constraint for Down Regulation - May 27

				/*temp_output += " L U1SR" + Get("Code") + "\n"; // Ramp rate constraint for SR
				temp_output += " L U2SR" + Get("Code") + "\n"; // Ramp rate constraint for SR*/

				/*temp_output += " L U1RR" + Get("Code") + "\n"; // Ramp rate constraint for RR
				temp_output += " L U2RR" + Get("Code") + "\n"; // Ramp rate constraint for RR*/
			}

			if (Get("RampRate")!="0" && isFirstinYear()){         //June 29 - Ramping limit - Venkat Krishnan INV
				temp_output += " L L10" + Get("Code") + "\n"; // 10 min. Ramping limited to capacity 
				temp_output += " L L30" + Get("Code") + "\n"; // 30 min. Ramping limited to capacity 
				temp_output += " L L60" + Get("Code") + "\n"; // 60 min. Ramping limited to capacity
				temp_output += " L LCAP10" + Get("Code") + "\n"; // 10 min. Ramping limited to capacity
				temp_output += " L LCAP30" + Get("Code") + "\n"; // 30 min. Ramping limited to capacity
				temp_output += " L LCAP60" + Get("Code") + "\n"; // 60 min. Ramping limited to capacity 
			}

			/*if (Get("Cycles")!="0"){ // Aug 10 2011 - cycling
				temp_output += " E Cy" + Get("Code") + "\n"; // Cycles due to energy flow
				temp_output += " E CyCR" + Get("Code") + "\n"; // Cycles due to 10 min. CR
				temp_output += " E CyRU" + Get("Code") + "\n"; // Cycles due to 1 min. RU
				temp_output += " E CyRD" + Get("Code") + "\n"; // Cycles due to 1 min. RD
				temp_output += " E CySR" + Get("Code") + "\n"; // Cycles due to 10 min. SR
				temp_output += " E CyRR" + Get("Code") + "\n"; // Cycles due to 30 min. RR
			}*/

		}
	}
    return temp_output;
}


string Arc::ArcCapNames() const {
    string temp_output = "";
	// Create capacity-investment constraint for arcs with valid investment
	if ( isFirstinYear() && isTransport()  && Get("TransInfr") == "" ) {
		// Transportation arc
		if ( Get("OpMax") != "Inf" ) {
			temp_output += " E inv2cap" + Get("Code") + "\n";
		}
	} else if ( isFirstinYear() && !isTransport() ) {
		// Energy arc
		if ( Get("OpMax") != "Inf" ) {
			temp_output += " E inv2cap" + Get("Code") + "\n";
		}
	}
    return temp_output;
}

string Arc::ArcDcNames() const {
    string temp_output = "";
    // Create a constraint for DC power flow branches
    if ( isDCflow() && (Get("From") < Get("To")) ) {
        temp_output += " E dcpf" + Get("Code") + "\n";
    }
    return temp_output;
}

string Arc::ArcColumns() const {
    string temp_output = "";
    string temp_code;
	if ( !isTransport() || Get("TransInfr") != "" ) {
		
		if ( Get("OpCost") != "0" ) {
			
			// Cost objective function
			temp_output += "    " + Get("Code") + " obj " + Get("OpCost") + "\n";
				

			/*if ( Get("RampRate") != "0" ) { // June 05 2012 - account for CF in gen. op. cost
				Step stepg = Str2Step( Get("ToStep") ); // June 05 2012 - req. CR op.
				//temp_output += "    " + Get("Code") + " obj " + ToString<double>(GetDouble("Eff")*GetDouble("OpCost")) + "\n";
				temp_output += "    " + Get("Code") + " obj -" + ToString<double>(GetDouble("OpCost")*GetDouble("Eff")*GetDouble("FOR")*GetDouble("AvgGWFOR")*atof(Step2Hours(stepg).c_str())/GetDouble("BaseGWhFOR")) + "\n"; 
			}*/
		}
			
		
		
		// Sustainability metrics
		for (int j = 0; j < SustMet.size(); ++j){		
			if ( Get("Op" + SustMet[j]) != "0" ){
				/*if ( j==0 && Get("Cycles") != "0" )  // Aug 11 - Cycle
					temp_output += "    " + Get("Code") + " " + SustMet[j] + GetYear() + " " + ToString<double>(GetDouble("Op" + SustMet[j])*(1+GetDouble("Cycles")*CycleCO2/100)) + "\n";
				else*/
					temp_output += "    " + Get("Code") + " " + SustMet[j] + GetYear() + " " + Get("Op" + SustMet[j]) + "\n";	
			}
		}
	}
	
	if ( !isTransport() ) {
		// Put arc in the constraint of the origin node
		if ( Get("From")[0] != 'X' ) {
			if ( InvertEff() ) {
				string inverse_eff = ToString<double>( 1 / GetDouble("Eff") );
				temp_output += "    " + Get("Code") + " " + Get("From") + Get("FromStep") + " -" + inverse_eff + "1\n";
			} else {
				temp_output += "    " + Get("Code") + " " + Get("From") + Get("FromStep") + " -1\n";
			}
		}
		// Put arc in the constraint of the destination node
		if ( Get("To")[0] != 'X' ) {
			if ( InvertEff() ) {
				temp_output += "    " + Get("Code") + " " + Get("To") + Get("ToStep") + " 1\n";
			} else {
				temp_output += "    " + Get("Code") + " " + Get("To") + Get("ToStep") + " " + Get("Eff") + "\n";
			}
		}
		// Upper limit for flows
		if ( Get("OpMax") != "Inf" ) {
			temp_output += "    " + Get("Code") + " ub" + Get("Code") + " 1\n";
		}

		// May 20 Venkat Krishnan ---> Energy + Contingen reserve (capacity) < capacity OPINV - July 3
		if ( Get("RampRate") != "0" ) {
			temp_output += "    " + Get("Code") + " ubR" + Get("Code") + " 1\n";
			temp_output += "    " + Get("Code") + " lbR" + Get("Code") + " 1\n"; // May 27 VK--> E - D.reg > min
			Step stepg = Str2Step( Get("ToStep") ); // June 05 2012 - req. CR op.
			temp_output += "    " + Get("Code") + " RCRop" + Get("To") + Get("ToStep") + " -" + ToString<double>(GetDouble("Eff")*GetDouble("FOR")*GetDouble("AvgGWFOR")*atof(Step2Hours(stepg).c_str())/GetDouble("BaseGWhFOR")) + "\n"; 
		}

		// Aug 10 2011 Venkat Krishnan ---> Cycling - Cycling cost?
		/*if ( Get("Cycles") != "0" ) {
			temp_output += "    " + Get("Code") + " Cy" + Get("Code") + " -" + ToString<double>(GetDouble("Cycles")*GetDouble("CyclesCost"))+ "\n";
		}*/

	} else if ( Get("TransInfr") != "" ) {
		// Put arc in the constraint of the destination node
		if ( Get("To")[0] != 'X' ) {
			temp_output += "    " + Get("Code") + " " + Get("To") + Get("ToStep") + " 1\n";
		}
		// Upper limit due to fleet
		if ( Get("OpMax") != "Inf" ) {
			string fleetcode = Get("From") + Get("FromStep");
			fleetcode[1] = fleetcode[0];
			temp_output += "    " + Get("Code") + " ub" + fleetcode + " 1\n";
		}		
		// Upper limit due to infrastructure
		if ( Get("OpMax") != "Inf" ) {
			string infcode = Get("From") + Get("FromStep");
			infcode[0] = Get("TransInfr")[0];
			infcode[1] = Get("TransInfr")[0];
			temp_output += "    " + Get("Code") + " ub" + infcode + " 1\n";
		}
	}
	if ( !isTransport() || Get("TransInfr") != "" ) {
		temp_output += WriteEnergy2Trans();
		temp_output += WriteTrans2Energy();
	}
    // Put arc in DC power flow constraint if appropriate
    if ( isDCflow() ) {
        if ( Get("From") < Get("To") ) {
            temp_output += "    " + Get("Code") + " dcpf" + Get("Code") + " -1\n";
        } else {
            temp_code = Get("To") + Get("ToStep") + "_" + Get("From") + Get("FromStep");
            temp_output += "    " + Get("Code") + " dcpf" + temp_code + " 1\n";
        }
    }
    return temp_output;
}


// June 29 10-min. Ramp variables - Venkat Krishnan INV
string Arc::ArcRamp10Columns() const {
    string temp_output = "";
    // If ramp rate is allowed, write the appropriate cost
    if ( Get("RampRate") != "0" && isFirstinYear()) {
            temp_output += "    CAP10" + Get("Code") + " rp10" + Get("To")+ Get("ToStep") + " " + ToString<double>(GetDouble("RampRate")*10) + "\n";
		temp_output += "    CAP10" + Get("Code") + " L10" + Get("Code") + " 1\n";
		temp_output += "    CAP10" + Get("Code") + " LCAP10" + Get("Code") + " 1\n";
    }

   return temp_output;
}

// June 29 30-min. Ramp variables - Venkat Krishnan INV
string Arc::ArcRamp30Columns() const {
    string temp_output = "";
    // If ramp rate is allowed, write the appropriate cost
    if ( Get("RampRate") != "0" && isFirstinYear()) {
            temp_output += "    CAP30" + Get("Code") + " rp30" + Get("To")+ Get("ToStep") + " " + ToString<double>(GetDouble("RampRate")*30) + "\n";
		temp_output += "    CAP30" + Get("Code") + " L30" + Get("Code") + " 1\n";
		temp_output += "    CAP30" + Get("Code") + " LCAP30" + Get("Code") + " 1\n";
    }

   return temp_output;
}

// June 29 60-min. Ramp variables - Venkat Krishnan INV
string Arc::ArcRamp60Columns() const {
    string temp_output = "";
    // If ramp rate is allowed, write the appropriate cost
    if ( Get("RampRate") != "0" && isFirstinYear()) {
            temp_output += "    CAP60" + Get("Code") + " rp60" + Get("To")+ Get("ToStep") + " " + ToString<double>(GetDouble("RampRate")*60) + "\n";
		temp_output += "    CAP60" + Get("Code") + " L60" + Get("Code") + " 1\n";
		temp_output += "    CAP60" + Get("Code") + " LCAP60" + Get("Code") + " 1\n";
    }

   return temp_output;
}


// May 20 Contingency reserve variables - Venkat Krishnan
string Arc::ArcCRColumns() const {
    string temp_output = "";
    // If ramp rate is allowed, write the appropriate cost
    if ( Get("RampRate") != "0" ) {
        //temp_output += "    CR_" + Get("Code") + " obj " + ToString<double>(GetDouble("Eff")*GetDouble("OpCost"))+ "\n"; // operational CR - June 05 2012
		temp_output += "    CR_" + Get("Code") + " ubR" + Get("Code") + " 1\n";
		temp_output += "    CR_" + Get("Code") + " RCR" + Get("To") + Get("ToStep") + " 1\n"; //" " + Get("Eff") + "\n"; 2012 June 07
		temp_output += "    CR_" + Get("Code") + " U1CR" + Get("Code") + " 1\n"; //" " + Get("Eff") + "\n"; //Upper Bound by Ramp rates - Cap*rr
		temp_output += "    CR_" + Get("Code") + " U2CR" + Get("Code") + " 1\n"; //" " + Get("Eff") + "\n"; //Upper Bound by Ramp rates - Cap
		temp_output += "    CR_" + Get("Code") + " ubRop" + Get("Code") + " -1\n"; // new - June 05 2012 - CRop's upper limit

        // Aug 10 2011 - Cycling
        /*if ( Get("Cycles") != "0" )
	  	temp_output += "    CR_" + Get("Code") + " CyCR" + Get("Code") + " -" + ToString<double>(GetDouble("Cycles")*GetDouble("CyclesCost")*CycleReg*2/2)+ "\n";*/


	 // May 31 - fuel consumption affected by Reserve - Venkat Krishnan ; XN---EN---EL
	 // similar to -- temp_output += "    " + Get("Code") + " " + Get("To") + Get("ToStep") + " " + Get("Eff") + "\n";
	 // temp_output += "    CR_" + Get("Code") + " " + Get("From") + Get("FromStep") + " -0.08\n"; // EN--EL (add to XN) June 4 2012 - 12.75 gamma

	  /*Sustainability metrics // May 31
	  for (int j = 0; j < (int)SustMet.size(); ++j){		
	 	if ( Get("Op" + SustMet[j]) != "0" ){
				if ( j==0 && Get("Cycles") != "0" )  // Aug 11 - Cycle
					temp_output += "    CR_" + Get("Code") + " " + SustMet[j] + GetYear() + " " + ToString<double>(GetDouble("Op" + SustMet[j])/2*(1+GetDouble("Cycles")*CycleReg*2*CycleCO2/100)) + "\n";
				else 
					temp_output += "    CR_" + Get("Code") + " " + SustMet[j] + GetYear() + " " + ToString<double>(GetDouble("Op"+SustMet[j])/12.75) + "\n";
		}
	  }*/
    }
    return temp_output;
}

// June 05 2012 Contingency reserve operational variables - Venkat Krishnan
string Arc::ArcCRopColumns() const {
    string temp_output = "";
    // If ramp rate is allowed, write the appropriate cost
    if ( Get("RampRate") != "0" ) {
        temp_output += "    CRop_" + Get("Code") + " obj " + ToString<double>(GetDouble("OpCost")+GetDouble("VOCsr"))+ "\n"; // operational CR - June 05 2012
		temp_output += "    CRop_" + Get("Code") + " ubRop" + Get("Code") + " 1\n";
		temp_output += "    CRop_" + Get("Code") + " RCRop" + Get("To") + Get("ToStep") + " 1\n"; //" " + Get("Eff") + "\n"; 2012 june 07 - no eff in resserves

	 // June 05 2012 - fuel consumption affected by Reserve - Venkat Krishnan ; XN---EN---EL
	 // similar to -- temp_output += "    " + Get("Code") + " " + Get("To") + Get("ToStep") + " " + Get("Eff") + "\n";
	  temp_output += "    CRop_" + Get("Code") + " " + Get("From") + Get("FromStep") + " -" + ToString<double>(1*(1+GetDouble("HRDsr")))+ "\n"; //"-1\n"; // EN--EL (add to XN) June 07 2012 - Impact of Heat rate Degradation on fuel

	  	 // Sustainability metrics // operational CR - June 05 2012
	  for (int j = 0; j < (int)SustMet.size(); ++j){		
	 	if ( Get("Op" + SustMet[j]) != "0" ){
				/*if ( j==0 && Get("Cycles") != "0" )  // Aug 11 - Cycle
					temp_output += "    CR_" + Get("Code") + " " + SustMet[j] + GetYear() + " " + ToString<double>(GetDouble("Op" + SustMet[j])/2*(1+GetDouble("Cycles")*CycleReg*2*CycleCO2/100)) + "\n";
				else*/
					temp_output += "    CRop_" + Get("Code") + " " + SustMet[j] + GetYear() + " " + ToString<double>(GetDouble("Op"+SustMet[j])*(1+GetDouble("HRDsr"))) + "\n";
		}
	  }

    }
    return temp_output;
}

// May 27 Regulation variables - Venkat Krishnan
string Arc::ArcRUColumns() const {
    string temp_output = "";
    // If ramp rate is allowed, write the appropriate cost - Regulation Up
    if ( Get("RampRate") != "0" ) {
        temp_output += "    RU_" + Get("Code") + " obj " + ToString<double>((GetDouble("OpCost")+GetDouble("VOCreg"))/3)+ "\n"; // Estimate only 1/3rd allocated reserve (3*sigma) flows
        temp_output += "    RU_" + Get("Code") + " ubR" + Get("Code") + " 1\n";
	  temp_output += "    RU_" + Get("Code") + " RRU" + Get("To") + Get("ToStep") + " 1\n"; //" " + Get("Eff") + "\n";
	  temp_output += "    RU_" + Get("Code") + " U1RU" + Get("Code") + " 1\n"; //" " + Get("Eff") + "\n"; //Upper Bound by Ramp rates - Cap*rr

        // Aug 10 2011 - Cycling
        /*if ( Get("Cycles") != "0" ) 
	  temp_output += "    RU_" + Get("Code") + " CyRU" + Get("Code") + " -" + ToString<double>(GetDouble("Cycles")*GetDouble("CyclesCost")*CycleReg/3)+ "\n";*/

	 // May 31 - fuel consumption affected by Reserve - Venkat Krishnan ; XN---EN---EL
	  temp_output += "    RU_" + Get("Code") + " " + Get("From") + Get("FromStep") + " -" + ToString<double>(0.33333*(1+GetDouble("HRDreg")))+ "\n"; //+ " -0.33333\n"; // EN--EL (add to XN) June 07 2012 - Reg HRD

	 // Sustainability metrics // May 31
	  for (int j = 0; j < (int)SustMet.size(); ++j){		
	 	if ( Get("Op" + SustMet[j]) != "0" ){
				/*if ( j==0 && Get("Cycles") != "0" )  // Aug 11 - Cycle
					temp_output += "    RU_" + Get("Code") + " " + SustMet[j] + GetYear() + " " + ToString<double>(GetDouble("Op" + SustMet[j])/3*(1+GetDouble("Cycles")*(CycleReg/5)*CycleCO2/100)) + "\n";
				else*/
					temp_output += "    RU_" + Get("Code") + " " + SustMet[j] + GetYear() + " " + ToString<double>((GetDouble("Op"+SustMet[j])*(1+GetDouble("HRDreg")))/3) + "\n";
		}
	  }
    }
    return temp_output;
}

string Arc::ArcRDColumns() const {
    string temp_output = "";
    // If ramp rate is allowed, write the appropriate cost - Regulation Down
    if ( Get("RampRate") != "0" ) {
        temp_output += "    RD_" + Get("Code") + " obj -" + ToString<double>((GetDouble("OpCost")+GetDouble("VOCreg"))/3.5)+ "\n"; // Estimate only 1/3rd allocated reserve (3*sigma) flows - // reduce emission June 4 2012
        temp_output += "    RD_" + Get("Code") + " lbR" + Get("Code") + " -1\n"; // lower bound
	  temp_output += "    RD_" + Get("Code") + " RRD" + Get("To") + Get("ToStep") + " 1\n"; //" " + Get("Eff") + "\n";
	  temp_output += "    RD_" + Get("Code") + " U1RD" + Get("Code") + " 1\n"; //" " + Get("Eff") + "\n"; //Upper Bound by Ramp rates - Cap*rr

        // Aug 10 2011 - Cycling
        /*if ( Get("Cycles") != "0" ) 
	  temp_output += "    RD_" + Get("Code") + " CyRD" + Get("Code") + " -" + ToString<double>(GetDouble("Cycles")*GetDouble("CyclesCost")*CycleReg/3) +"\n";*/

	 // May 31 - fuel consumption affected by Reserve - Venkat Krishnan ; XN---EN---EL
	  temp_output += "    RD_" + Get("Code") + " " + Get("From") + Get("FromStep") + " " + ToString<double>(0.285*(1-GetDouble("HRDreg")))+ "\n"; //+ " 0.285\n"; // EN--EL (add to XN) June 07 2012 - Reg HRD

	 // Sustainability metrics // May 31
	  for (int j = 0; j < (int)SustMet.size(); ++j){		
	 	if ( Get("Op" + SustMet[j]) != "0" ){
				/*if ( j==0 && Get("Cycles") != "0" )  // Aug 11 - Cycle
					temp_output += "    RD_" + Get("Code") + " " + SustMet[j] + GetYear() + " " + ToString<double>(GetDouble("Op" + SustMet[j])/3*(1+GetDouble("Cycles")*(CycleReg/5)*CycleCO2/100)) + "\n";
				else*/
					temp_output += "    RD_" + Get("Code") + " " + SustMet[j] + GetYear() + " -" + ToString<double>((GetDouble("Op"+SustMet[j])*(1-GetDouble("HRDreg")))/3.5) + "\n"; // reduce emission June 4 2012
		}
	  }
    }
    return temp_output;
}

//string Arc::ArcSRColumns() const { - both in separate files
//string Arc::ArcRRColumns() const {

// Aug 10 2011 - Cycling
/*string Arc::ArcCyEColumns() const {
    string temp_output = "";
    // 
    if ( Get("Cycles") != "0" ) {
        temp_output += "    C_" + Get("Code") + " obj " + "1\n";
        temp_output += "    C_" + Get("Code") + " Cy" + Get("Code") + " 1\n"; 
    }
    return temp_output;
}*/

// Aug 10 2011 - Cycling
/*string Arc::ArcCyCRColumns() const {
    string temp_output = "";
    // 
    if ( Get("Cycles") != "0" ) {
        temp_output += "    CCR_" + Get("Code") + " obj " + "1\n";
        temp_output += "    CCR_" + Get("Code") + " CyCR" + Get("Code") + " 1\n"; 
    }
    return temp_output;
}*/

// Aug 10 2011 - Cycling
/*string Arc::ArcCyRUColumns() const {
    string temp_output = "";
    // 
    if ( Get("Cycles") != "0" ) {
        temp_output += "    CRU_" + Get("Code") + " obj " + "1\n";
        temp_output += "    CRU_" + Get("Code") + " CyRU" + Get("Code") + " 1\n"; 
    }
    return temp_output;
}*/

// Aug 10 2011 - Cycling
/*string Arc::ArcCyRDColumns() const {
    string temp_output = "";
    // 
    if ( Get("Cycles") != "0" ) {
        temp_output += "    CRD_" + Get("Code") + " obj " + "1\n";
        temp_output += "    CRD_" + Get("Code") + " CyRD" + Get("Code") + " 1\n"; 
    }
    return temp_output;
}*/

// Aug 10 2011 - Cycling
/*string Arc::ArcCySRColumns() const {
    string temp_output = "";
    // 
    if ( Get("Cycles") != "0" ) {
        temp_output += "    CSR_" + Get("Code") + " obj " + "1\n";
        temp_output += "    CSR_" + Get("Code") + " CySR" + Get("Code") + " 1\n"; 
    }
    return temp_output;
}*/

// Aug 10 2011 - Cycling
/*string Arc::ArcCyRRColumns() const {
    string temp_output = "";
    // 
    if ( Get("Cycles") != "0" ) {
        temp_output += "    CRR_" + Get("Code") + " obj " + "1\n";
        temp_output += "    CRR_" + Get("Code") + " CyRR" + Get("Code") + " 1\n"; 
    }
    return temp_output;
}*/



string Arc::InvArcColumns() const {
	string temp_output = "";
	string temp_code;
	// If investment allowed
	if ( InvArc() && Get("TransInfr") == "" ) {
		// Cost of investment
		temp_output += "    inv" + Get("Code") + " obj " + Get("InvCost") + "\n";
		
		// Investment added to the next upper bound contraints
		Step step1, step2, stepguide, maxstep;
		step1 = Str2Step( Get("FromStep") );
		step2 = Str2Step( Get("ToStep") );
		if ( Get("LifeSpan") != "X" ) {
			maxstep = StepSum( step1, Str2Step(Get("LifeSpan")) );
			maxstep = (maxstep > SLength) ? SLength : maxstep;
		} else {
			maxstep = SLength;
		}
		
		stepguide = (step1 > step2) ? step1 : step2;
		while (stepguide <= maxstep) {
			temp_output += "    inv" + Get("Code") + " inv2cap";
			temp_output += Get("From") + Step2Str(step1);
			if ( !isTransport() ) {
				temp_output += "_" + Get("To") + Step2Str(step2);
			}
			temp_output += " -1\n";
			if ( isFirstBidirect() || isFirstTransport() ) {
				Arc Arc2(*this, true);
				temp_output += "    inv" + Get("Code") + " inv2cap";
				temp_output += Arc2.Get("From") + Step2Str(step1);
				if ( !isTransport() ) {
					temp_output += "_" + Arc2.Get("To") + Step2Str(step2);
				}
				temp_output += " -1\n";
			}
			
			// Move to the next year
			++stepguide[0]; ++step1[0]; ++step2[0];
		}
	}
	return temp_output;
}

string Arc::CapArcColumns(int selector) const {
    string temp_output = "";
	
	// If investment is allowed,
    if ( isFirstinYear() && Get("OpMax") != "Inf" && Get("TransInfr") == "" ) {
		if ( selector != 1 ) {
			// Add capacity as an upper bound for flows withing that year
			Step step1, step2, stepguide, maxstep;
			step1 = Str2Step( Get("FromStep") );
			step2 = Str2Step( Get("ToStep") );
			stepguide = (step1 > step2) ? step1 : step2;
			maxstep = ( isStorage() ) ? step1 : stepguide;
			++maxstep[0];
			
			string common = "    cap" + Get("Code") + " ub";
			string ucommon = "    cap" + Get("Code") + " ubR"; // July 4 - Upper limit
			string u1common = "    cap" + Get("Code") + " U1"; // July 4 - Reserve contribution limited by cap*rr
			string u2common = "    cap" + Get("Code") + " U2"; // July 4 - Reserve contribution limited by cap
			string rqcommon = "    cap" + Get("Code") + " R"; // July 4 - Wind Capacity contributing to RU, RD and 10 min. SR


			while (stepguide < maxstep) {
				temp_output += common;
				temp_output += Get("From") + Step2Str(step1);
				if ( !isTransport() ) {
					temp_output += "_" + Get("To") + Step2Str(step2);
				}
				temp_output += " -" + Step2Hours(stepguide) + "\n";
				

				// July 4 - Venkat Krishnan - Energy + Reserves < capacity * hours
				if (Get("RampRate")!="0"){

					// Maximum Flow limit
					temp_output += ucommon;
					temp_output += Get("From") + Step2Str(step1);
					if ( !isTransport() ) {
						temp_output += "_" + Get("To") + Step2Str(step2);
					}
					temp_output += " -" + Step2Hours(stepguide) + "\n";


					// Contingency Reserve
					temp_output += u1common + "CR";
					temp_output += Get("From") + Step2Str(step1);
					if ( !isTransport() ) {
						temp_output += "_" + Get("To") + Step2Str(step2);
					}
					temp_output += " -" + ToString<double>(GetDouble("RampRate") *10* atof(Step2Hours(stepguide).c_str())) + "\n";

					temp_output += u2common + "CR";
					temp_output += Get("From") + Step2Str(step1);
					if ( !isTransport() ) {
						temp_output += "_" + Get("To") + Step2Str(step2);
					}
					temp_output += " -" + Step2Hours(stepguide) + "\n";


					// Regulation Up Reserve
					temp_output += u1common + "RU";
					temp_output += Get("From") + Step2Str(step1);
					if ( !isTransport() ) {
						temp_output += "_" + Get("To") + Step2Str(step2);
					}
					temp_output += " -" + ToString<double>(GetDouble("RampRate") * atof(Step2Hours(stepguide).c_str())) + "\n";

					// Regulation Down Reserve
					temp_output += u1common + "RD";
					temp_output += Get("From") + Step2Str(step1);
					if ( !isTransport() ) {
						temp_output += "_" + Get("To") + Step2Str(step2);
					}
					temp_output += " -" + ToString<double>(GetDouble("RampRate") * atof(Step2Hours(stepguide).c_str())) + "\n";


					// Spinning Reserve
					/*temp_output += u1common + "SR";
					temp_output += Get("From") + Step2Str(step1);
					if ( !isTransport() ) {
						temp_output += "_" + Get("To") + Step2Str(step2);
					}
					temp_output += " -" + ToString<double>(GetDouble("RampRate") *10* atof(Step2Hours(stepguide).c_str())) + "\n";

					temp_output += u2common + "SR";
					temp_output += Get("From") + Step2Str(step1);
					if ( !isTransport() ) {
						temp_output += "_" + Get("To") + Step2Str(step2);
					}
					temp_output += " -" + Step2Hours(stepguide) + "\n";*/


					// Replacement Reserve - 30 min.
					/*temp_output += u1common + "RR";
					temp_output += Get("From") + Step2Str(step1);
					if ( !isTransport() ) {
						temp_output += "_" + Get("To") + Step2Str(step2);
					}
					temp_output += " -" + ToString<double>(GetDouble("RampRate") *30* atof(Step2Hours(stepguide).c_str())) + "\n";

					temp_output += u2common + "RR";
					temp_output += Get("From") + Step2Str(step1);
					if ( !isTransport() ) {
						temp_output += "_" + Get("To") + Step2Str(step2);
					}
					temp_output += " -" + Step2Hours(stepguide) + "\n";*/
				} 


				// May 28 2011 Regulation requirement VK
				if (Get("pRegulation")!="0"){
					temp_output += rqcommon + "RU";
					temp_output += Get("To") + Step2Str(step2);
					for (int creg=0; creg<Regvalue.size(); ++creg){
						if(Get("To")+Step2Str(step2) == Regnode[creg]){
							ELReg = Regvalue[creg];
							break;
						}
					}
					double dsq = pow((GetDouble("pRegulation")*GetDouble("Wbase")/100),2)/(2*GetDouble("Wbase")*ELReg); // Add N at the deno - regulation from load - here (Cap/0.1GW * dsq)
					temp_output += " -" + ToString<double>(3*dsq*atof(Step2Hours(stepguide).c_str())) + "\n"; // N + d/2N --- square root; dsq = sigma_w^2/2N // July 4- 3sigma 
					
					temp_output += rqcommon + "RD";
					temp_output += Get("To") + Step2Str(step2);
					temp_output += " -" + ToString<double>(3.5*dsq*atof(Step2Hours(stepguide).c_str())) + "\n"; // N + d/2N - July 4- 3.5 sigma - Down more
				} 

				/*if (Get("pRamp10")!="0"){
					temp_output += rqcommon + "SR";
					temp_output += Get("To") + Step2Str(step2);
					for (int creg=0; creg<Regvalue10.size(); ++creg){
						if(Get("To")+Step2Str(step2) == Regnode10[creg]){
							ELReg10 = Regvalue10[creg];
							break;
						}
					}
					double dsq10 = pow((GetDouble("pRamp10")*GetDouble("Wbase")/100),2)/(2*GetDouble("Wbase")*ELReg10); 
					temp_output += " -" + ToString<double>(3*dsq10*atof(Step2Hours(stepguide).c_str())) + "\n"; // N + d/2N --- square root; dsq = sigma_w^2/2N // July 4- 3sigma 
			      }*/


				// Move to the next year
				stepguide = NextStep(stepguide);
				if (NextStep(step1) <= stepguide)
					step1 = NextStep(step1);
				if (NextStep(step2) <= stepguide)
					step2 = NextStep(step2);
			}
		}
		if (selector != 2) {
			// Add current investment to the capacity of the arc
			temp_output += "    cap" + Get("Code") + " inv2cap" + Get("Code") + " 1\n";
			
			// Contribution to peak load
			if ( Get("CapacityFactor") != "0" ) {
				temp_output += "    cap" + Get("Code");
				temp_output += " pk" + Get("To") + Get("ToStep") + " " + Get("CapacityFactor") + "\n";
			}

			// Contribution to Ramp 1-min (Reg) /// June 15 2011 - Venkat INV
			if ( Get("RampRate") != "0" ) {
				temp_output += "    cap" + Get("Code");
				double rp1 =  GetDouble("RampRate");
				temp_output += " rc" + Get("To") + Get("ToStep") + " " + ToString<double>(rp1) + "\n"; 
			}

			// Contribution to Ramp 10-min /// June 28 2011 - Venkat INV
			if ( Get("RampRate") != "0" ) {
				temp_output += "    cap" + Get("Code");
				double rp10 =  1/(10*GetDouble("RampRate"));
				temp_output += " L10" + Get("Code") + " -" + ToString<double>(rp10) + "\n";
				temp_output += "    cap" + Get("Code");
				temp_output += " LCAP10" + Get("Code") + " -1\n"; 
			}


			// Contribution to Ramp 30-min /// June 28 2011 - Venkat INV
			if ( Get("RampRate") != "0" ) {
				temp_output += "    cap" + Get("Code");
				double rp30 =  1/(30*GetDouble("RampRate"));
				temp_output += " L30" + Get("Code") + " -" + ToString<double>(rp30) + "\n"; 
				temp_output += "    cap" + Get("Code");
				temp_output += " LCAP30" + Get("Code") + " -1\n"; 
			}


			// Contribution to Ramp 60-min /// June 28 2011 - Venkat INV
			if ( Get("RampRate") != "0" ) {
				temp_output += "    cap" + Get("Code");
				double rp60 =  1/(60*GetDouble("RampRate"));
				temp_output += " L60" + Get("Code") + " -" + ToString<double>(rp60) + "\n"; 
				temp_output += "    cap" + Get("Code");
				temp_output += " LCAP60" + Get("Code") + " -1\n"; 
			}


			// Wind contribution to 1-min Ramp // June 21 2011 - venkat INV
			if (Get("pRegulation")!="0"){	
				// June 22 2011 Investment Regulation requirement VK
					temp_output += "    cap" + Get("Code");
					temp_output += " rc" + Get("To") + Get("ToStep");
					for (int creg=0; creg<Regvalue.size(); ++creg){ // Since no change in monthly demand and yearly investments
						if(Get("To")+Get("ToStep") == Regnode[creg]){
							ELReg = Regvalue[creg];
							break;
						}
					}
					double dsq = pow((GetDouble("pRegulation")*GetDouble("Wbase")/100),2)/(2*GetDouble("Wbase")*ELReg); // Add N at the deno - regulation from load - here (Cap/0.1GW * dsq)
					temp_output += " -" + ToString<double>(dsq*Peak1) + "\n"; // N + d/2N --- square root; dsq = sigma_w^2/2N // May 30 change 2*3sigma // June 15 * sigma, not 3sigma
				}


			
			// Wind contribution to 10-min Ramp // June 28 2011 - venkat INV
			if (Get("pRamp10")!="0"){	
				// June 28 2011 Investment 10-min ramp requirement VK
					temp_output += "    cap" + Get("Code");
					temp_output += " rp10" + Get("To") + Get("ToStep");
					for (int creg=0; creg<Regvalue10.size(); ++creg){ // Since no change in monthly demand and yearly investments
						if(Get("To")+Get("ToStep") == Regnode10[creg]){
							ELReg10 = Regvalue10[creg];
							break;
						}
					}
					double dsq10 = pow((GetDouble("pRamp10")*GetDouble("Wbase")/100),2)/(2*GetDouble("Wbase")*ELReg10); // Add N at the deno - regulation from load - here (Cap/0.1GW * dsq)
					temp_output += " -" + ToString<double>(dsq10*Peak10) + "\n"; // N + d/2N --- square root; dsq = sigma_w^2/2N // May 30 change 2*3sigma // June 15 * sigma, not 3sigma
				}

			// Wind contribution to 30-min Ramp // June 28 2011 - venkat INV
			if (Get("pRamp30")!="0"){	
				// June 28 2011 Investment Ramp30 requirement VK
					temp_output += "    cap" + Get("Code");
					temp_output += " rp30" + Get("To") + Get("ToStep");
					for (int creg=0; creg<Regvalue30.size(); ++creg){ // Since no change in monthly demand and yearly investments
						if(Get("To")+Get("ToStep") == Regnode30[creg]){
							ELReg30 = Regvalue30[creg];
							break;
						}
					}
					double dsq30 = pow((GetDouble("pRamp30")*GetDouble("Wbase")/100),2)/(2*GetDouble("Wbase")*ELReg30); // Add N at the deno - regulation from load - here (Cap/0.1GW * dsq)
					temp_output += " -" + ToString<double>(dsq30*Peak30) + "\n"; // N + d/2N --- square root; dsq = sigma_w^2/2N // May 30 change 2*3sigma // June 15 * sigma, not 3sigma
				}

			// Wind contribution to 60-min Ramp // June 28 2011 - venkat INV
			if (Get("pRamp60")!="0"){	
				// June 28 2011 Investment Ramp60 requirement VK
					temp_output += "    cap" + Get("Code");
					temp_output += " rp60" + Get("To") + Get("ToStep");
					for (int creg=0; creg<Regvalue60.size(); ++creg){ // Since no change in monthly demand and yearly investments
						if(Get("To")+Get("ToStep") == Regnode60[creg]){
							ELReg60 = Regvalue60[creg];
							break;
						}
					}
					double dsq60 = pow((GetDouble("pRamp60")*GetDouble("Wbase")/100),2)/(2*GetDouble("Wbase")*ELReg60); // Add N at the deno - regulation from load - here (Cap/0.1GW * dsq)
					temp_output += " -" + ToString<double>(dsq60*Peak60) + "\n"; // N + d/2N --- square root; dsq = sigma_w^2/2N // May 30 change 2*3sigma // June 15 * sigma, not 3sigma
				}

	
		}
    }
    return temp_output;
}

vector<string> Arc::Events() const {
    vector<string> temp_output(0);
	
	// If investment is allowed,
    if ( isFirstinYear() && Get("OpMax") != "Inf" && Get("TransInfr") == "" ) {
		// Base case
		temp_output.push_back( "1" );
		for (int event = 1; event <= Nevents; ++event) {
			// For events
			string property = "CapacityLoss" + ToString<int>(event);
			temp_output.push_back( Get(property)  );
		}
	}
    return temp_output;
}

string Arc::ArcRhs() const {
    string temp_output = "";
    // RHS in the upper bound constraints, for the capacity existing at t=0
    if ( isFirstinYear() && Get("OpMax") != "Inf"  && Get("TransInfr") == "" ) {
		temp_output += " rhs inv2cap" + Get("Code") + " " + Get("OpMax") + "\n";
    }
    return temp_output;
}

string Arc::ArcBounds() const {
	string temp_output = "";
	// Write minimum for operational flow
	if ( Get("OpMin") != "0"  && Get("TransInfr") == "" ) {
		temp_output += " LO bnd " + Get("Code") + " " + Get("OpMin") + "\n";
	}
	return temp_output;
}

string Arc::ArcInvBounds() const {
	string temp_output = "";
	if ( InvArc()  && Get("TransInfr") == "" ) {
		// Investment min and maximum when investment is allowed
		if ( Get("InvMin") != "0" ) {
			temp_output += " LO bnd inv" + Get("Code") + " " + Get("InvMin") + "\n";
		}
		if ( Get("InvMax") != "Inf" ) {
			temp_output += " UP bnd inv" + Get("Code") + " " + Get("InvMax") + "\n";
		}
	}

	return temp_output;
}

string Arc::WriteEnergy2Trans() const {
	string temp_output = "";
	// Load on the transportation side created by a coal/energy arc
	if (Energy2Trans) {
		temp_output += "    " + Get("Code") + " " + Get("From") + Get("To").substr(2,2) + Get("ToStep") + " -1\n";
	}
	return temp_output;
}

string Arc::WriteTrans2Energy() const {
	string temp_output = "";
	unsigned int k=0;
	// Energy demand for a transportation node that requires it
	while ( k+1 < Trans2Energy.size() ) {
		temp_output += "    " + Get("Code") + " " + Trans2Energy[k] + " -" + Trans2Energy[k+1] + "\n";
		k++; k++;
	}
	return temp_output;
}

// Get what time the arc belongs to (i.e., year)
int Arc::Time() const {
	return Str2Step(Get("FromStep"))[0];
}


// ****** Boolean functions ******
// Investment is allowed if inv. cost is declared and it's the first arc in each investment period
bool Arc::InvAllowed() const {
	return (Get("InvCost") != "X") && isFirstinYear();
}

// Is it the first arc in a year?
bool Arc::isFirstinYear() const {
	bool output = true;
	Step step1, step2, stepguide;
	step1 = Str2Step( Get("FromStep") );
	step2 = Str2Step( Get("ToStep") );
	stepguide = ( (step1 > step2) || isStorage() ) ? step1 : step2;
	for ( unsigned int k = Get("InvStep").size(); k < SName.size(); k++) {
		output = output && ( (stepguide[k]==0) || (stepguide[k]==1) );
	}
	return output;
}

// Investment allowed plus technology is available and it's the first if the arc is bidirectional
bool Arc::InvArc() const {
	bool output;
	output = InvAllowed() && (!isTransport() && (!isBidirect() || isFirstBidirect()) || isFirstTransport() );
	output = output && ( Str2Step(Get("FromStep")) >= Str2Step(Get("InvStart")) );
	return output;
}

// Is efficiency inverted? (Used with electrical generators)
bool Arc::InvertEff() const {
	return ( Get("InvertEff") == "Y" || Get("InvertEff") == "y" );
}

// Is the arc part of DC flow constraints?
bool Arc::isDCflow() const {
	bool output = (Get("From").substr(0,2) == DCCode) && (Get("To").substr(0,2) == DCCode);
	return output && useDCflow;
}

// Is it a storage arc?
bool Arc::isStorage() const {
	return (Get("From").substr(1,1) == StorageCode) && (Get("From") == Get("To"));
}

// Is the arc bidirectional? (excludes storage nodes)
bool Arc::isBidirect() const {
	bool output = Get("From").substr(0,2) == Get("To").substr(0,2);
	output = output && !isDCflow() && !isStorage();
	return output;
}

// Is the arc bidirectional and and the first alphabetically? 
bool Arc::isFirstBidirect() const {
	return isBidirect() && (Get("From") < Get("To") );
}

// Is is a transportation arc?
bool Arc::isTransport() const {
	return (Get("From").size() > 4) && (Get("To").size() > 4);
}

// Is it a transportation arc and the first alphabetically?
bool Arc::isFirstTransport() const {
	return isTransport() && (Get("From").substr(2,2) < Get("From").substr(4,2));
}


// ****** Other functions ******
// Find the index for a arc property selector
int FindArcSelector(const string& selector) {
	int index = -1;
	for (unsigned int k = 0; k < ArcProp.size(); ++k) {
		if (selector == ArcProp[k]) index = k;
	}
	return index;
}
