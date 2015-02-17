// --------------------------------------------------------------
//    NETSCORE Version 1
//    arc.h -- Definition of node functions
//    2009-2010 (c) Eduardo Ibáñez
//    2011-2014 (c) Venkat Krishnan (VK)
// --------------------------------------------------------------

#ifndef _ARC_H_
#define _ARC_H_

#include <math.h>
// Declare class to store arc information
class Arc {
    public:
        Arc();
        Arc(const Arc& rhs);
		Arc(const Arc& rhs, const bool reverse);
        ~Arc();
        Arc& operator=(const Arc& rhs);

        string Get(const string& selector) const;
		string GetYear() const;
        double GetDouble(const string& selector) const;
        bool GetBool(const string& selector) const;
        vector<string> GetVecStr(const string& selector) const;
        void Set(const string& selector, const string& input);
        void Set(const string& selector, const bool input);
        void Add(const string& selector, const string& input);
        void Multiply(const string& selector, const double value);
        int Time() const;
		
        string WriteEnergy2Trans() const;
        string WriteTrans2Energy() const;

        string ArcUbNames() const;
        string ArcCapNames() const;
        string ArcDcNames() const;
        string ArcColumns() const;
		
	// May 20 Contingency reserve variables - Venkat Krishnan
	string ArcCRColumns() const;
	string ArcCRopColumns() const; // June 05 2012 - CRop.

	// May 27 Regulation variables - Venkat Krishnan
	string ArcRUColumns() const;
	string ArcRDColumns() const;

	// July 4 SR and RR variables - Venkat Krishnan
	//string ArcSRColumns() const;
	//string ArcRRColumns() const;

	string ArcRamp10Columns() const; // INV June 29 10 min. Ramp
	string ArcRamp30Columns() const; // INV June 29 30 min. Ramp
	string ArcRamp60Columns() const; // INV June 29 60 min. Ramp

      // Aug 10 2011 - Cycling
	/*string ArcCyEColumns() const;
	string ArcCyCRColumns() const;
	string ArcCyRUColumns() const;
	string ArcCyRDColumns() const;
	string ArcCySRColumns() const;
	string ArcCyRRColumns() const;*/


        string InvArcColumns() const;
        string CapArcColumns(int selector) const;
		vector<string> Events() const;
        string ArcRhs() const;
        string ArcBounds() const;
        string ArcInvBounds() const;
		
		bool InvAllowed() const;
		bool isFirstinYear() const;
		bool InvArc() const;
		bool InvertEff() const;
		bool isDCflow() const;
		bool isStorage() const;
		bool isBidirect() const;
		bool isFirstBidirect() const;
		bool isTransport() const;
		bool isFirstTransport() const;

    private:
        vector<string> Properties, Trans2Energy;
        bool Energy2Trans;
};

// Find the index for a arc property selector
int FindArcSelector(const string& selector);

#endif  // _NODE_H_
