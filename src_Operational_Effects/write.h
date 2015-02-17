// --------------------------------------------------------------
//    NETSCORE Version 1
//    write.h -- Definition of file writing functions
//    2009-2010 (c) Eduardo Ibáñez
//    2011-2014 (c) Venkat Krishnan
// --------------------------------------------------------------

#ifndef _WRITE_H_
#define _WRITE_H_

void WriteOutput(const char* fileinput, Index& idx, vector<string>& values, const string& header);
void WriteOutput(const char* fileinput, Index& idx, vector<string>& values, const int start, const string& header);
void WriteOutput(const char* fileinput, Index& idx, vector<Node>& Nodes, const string& selector, const string& header);
void WriteOutput(const char* fileinput, Index& idx, vector<Arc>& Arcs, const string& selector, const string& header);

// May 28 - Write area Regulation - for wind 
void WriteOutput(const char* fileinput, vector<string>& NodeName, vector<double> Regval, const char* header); 

// June 05 2012 - Write Final objectives 
void WriteOutput(const char* fileinput, vector<double> objective, const char* header);

#endif  // _WRITE_H_
