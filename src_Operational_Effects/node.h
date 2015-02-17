// --------------------------------------------------------------
//    NETSCORE Version 1
//    node.h -- Definition of node functions
//    2009-2010 (c) Eduardo Ibáñez
//    2011-2014 (c) Venkat Krishnan (VK)
// --------------------------------------------------------------

#ifndef _NODE_H_
#define _NODE_H_

// Declare class type to hold node information
class Node {
    public:
        Node();
        Node(const Node& rhs);
        ~Node();
        Node& operator=(const Node& rhs);
		
        string Get(const string& selector) const;
        double GetDouble(const string& selector) const;
        vector<string> GetVecStr() const;
        void Set(const string& selector, const string& input);
        void Multiply(const string& selector, const double value);
	  int Time() const;

        string NodeNames() const;
        string NodeUDColumns() const;
 	  string NodePeakRows() const;
        string NodeRMColumns() const;
        string NodeRMBounds() const;

	// INV June 22 Venkat
        string NodeRMrcColumns() const;
        string NodeRMrcBounds() const;

	// INV June 29 Venkat Ramps - 10, 30 , 60 mins
        string NodeRMrp10Columns() const;
        string NodeRMrp30Columns() const;
        string NodeRMrp60Columns() const;
        string NodeRMrp10Bounds() const;
        string NodeRMrp30Bounds() const;
        string NodeRMrp60Bounds() const;

        string NodeRhs() const;
		
	 // May 20 Contingency reserve variables - Venkat Krishnan
	 string CRRhs() const;
	 // May 27 Regulation variables - Venkat Krishnan INV
	 string RegRhs() const; 

	 // July 4 Replacement reserve variables - Venkat Krishnan
	 //string RRRhs() const;
	 //string SRRhs() const;


        string DCNodesBounds() const;
  	  bool isDCelect() const;
	  bool isDCflow() const;
	  bool isFirstinYear() const;
		
    private:
        vector<string> Properties;
};

// Find the index for a node property selector
int FindNodeSelector(const string& selector);

#endif  // _NODE_H_
