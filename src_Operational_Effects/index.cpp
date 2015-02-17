// --------------------------------------------------------------
//    NETSCORE Version 1
//    index.cpp -- Implementation of index functions
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

// Contructors and destructor for the Index class
Index::Index() :
	Position(0),
	Column(0),
	Year(0),
	Name(0) {}

Index::Index(const Index& rhs) :
	Position(rhs.GetPosition()),
	Column(rhs.GetColumn()),
	Year(rhs.GetYear()),
	Name(rhs.GetName()) {}

Index::~Index() {}

Index& Index::operator=(const Index& rhs) {
    Position = rhs.GetPosition();
	Column = rhs.GetColumn(),
	Year = rhs.GetYear(),
	Name = rhs.GetName();
    return *this;
}

// Recover information
vector<int> Index::GetPosition() const { return Position; }
vector<int> Index::GetColumn() const { return Column; }
vector<int> Index::GetYear() const { return Year; }
vector<string> Index::GetName() const { return Name; }
int Index::GetPosition(const int idx) const { return Position[idx]; }
int Index::GetColumn(const int idx) const { return Column[idx]; }
int Index::GetYear(const int idx) const { return Year[idx]; }
string Index::GetName(const int idx) const { return Name[idx]; }
int Index::GetSize() const { return Position.size(); }

// Add one more element to the index collection
void Index::Add(const int idx, const int col, const string& name) {
	Position.push_back(idx);
	Column.push_back(col);
	Year.push_back(col);
	Name.push_back(name);
}
void Index::Add(const int idx, const int col, const int year, const string& name) {
	Position.push_back(idx);
	Column.push_back(col);
	Year.push_back(year);
	Name.push_back(name);
}
void Index::Add(const int idx, const Step& col, const string& name) {
	Add(idx, Step2Col(col), col[0], name);
}

// Write a file 
void Index::WriteFile(const char* fileinput) const {
	ofstream myfile;
	myfile.open(fileinput);
	
	for (unsigned int i = 0; i < Position.size(); ++i) {
		myfile << Position[i];
		myfile << "\n";
		myfile << Column[i];
		myfile << "\n";
		myfile << Year[i];
		myfile << "\n";
		myfile << Name[i];
		myfile << "\n";
	}
	myfile.close();
}

// Read a file
Index ReadFile(const char* fileinput) {
	Index TempIndex;
	char line [ 200 ];
	FILE *file = fopen(fileinput, "r");
    
	if ( file != NULL ) {
		int idx, col, year;
		string name;
		for (;;) {
			// Read a line from the file and finish if empty is read
			if ( fgets(line, sizeof line, file) == NULL ) break;
			idx = strtol(line, NULL, 10);
			if ( fgets(line, sizeof line, file) == NULL ) break;
			col = strtol(line, NULL, 10);
			if ( fgets(line, sizeof line, file) == NULL ) break;
			year = strtol(line, NULL, 10);
			if ( fgets(line, sizeof line, file) == NULL ) break;
			CleanLine(line);
			name = string(line);
			
			TempIndex.Add(idx, col, year, name);
		}
	} else {
		printError("error", fileinput);
	}
	return TempIndex;
}

// Loads all necessary index files in memory
void ImportIndices() {
	IdxNode = ReadFile("prepdata/idx_node.csv");
	IdxUd = ReadFile("prepdata/idx_ud.csv");
	IdxRm = ReadFile("prepdata/idx_rm.csv");
	IdxRmrc = ReadFile("prepdata/idx_rmrc.csv"); // INV June 22 Ramp
	IdxArc = ReadFile("prepdata/idx_arc.csv");
	IdxInv = ReadFile("prepdata/idx_inv.csv");
	IdxCap = ReadFile("prepdata/idx_cap.csv");
	IdxUb = ReadFile("prepdata/idx_ub.csv");
	IdxEm = ReadFile("prepdata/idx_em.csv");

 // Insert new variable - May 23 Venkat Krishnan 
	IdxCR = ReadFile("prepdata/idx_cr.csv"); // Contingency reserve
	IdxCRop = ReadFile("prepdata/idx_crop.csv"); // Contingency reserve op. - June 05 2012
// Insert new variable - May 27 Venkat Krishnan 
	IdxRU = ReadFile("prepdata/idx_ru.csv"); // Regulation up and down
	IdxRD = ReadFile("prepdata/idx_rd.csv"); //

// Insert new variables - July 4 Venkat Krishnan 
	//IdxSR = ReadFile("prepdata/idx_sr.csv"); // 10 min. spinning
	//IdxRR = ReadFile("prepdata/idx_rr.csv"); // Replaceent reserve

	IdxRC10 = ReadFile("prepdata/idx_rc10.csv"); // INV June 29 10-min. Ramp
	IdxRC30 = ReadFile("prepdata/idx_rc30.csv"); // INV June 29 30-min. Ramp
	IdxRC60 = ReadFile("prepdata/idx_rc60.csv"); // INV June 29 60-min. Ramp
	IdxRmrp10 = ReadFile("prepdata/idx_rmrp10.csv"); // INV June 29 10-min. Ramp - reserve
	IdxRmrp30 = ReadFile("prepdata/idx_rmrp30.csv"); // INV June 29 30-min. Ramp - reserve
	IdxRmrp60 = ReadFile("prepdata/idx_rmrp60.csv"); // INV June 29 60-min. Ramp - reserve

// Aug 11 CYcling
/*	IdxCyE = ReadFile("prepdata/idx_cye.csv"); // cycling
	IdxCyCR = ReadFile("prepdata/idx_cycr.csv"); // cycling
	IdxCyRU = ReadFile("prepdata/idx_cyru.csv"); // cycling
	IdxCyRD = ReadFile("prepdata/idx_cyrd.csv"); // cycling
	IdxCySR = ReadFile("prepdata/idx_cysr.csv"); // cycling
	IdxCyRR = ReadFile("prepdata/idx_cyrr.csv"); // cycling*/


	IdxDc = ReadFile("prepdata/idx_dc.csv");
}
