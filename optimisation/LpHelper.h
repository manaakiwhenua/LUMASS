// LpHelper.h:
//
//////////////////////////////////////////////////////////////////////

#ifndef HLPHELPER_H
#define HLPHELPER_H

#include <string>
#include <sstream>

#include "nmlog.h"
#include "lp_lib.h"

// define the context string for this
// class (used by nmlog macros)
#define ctxLpHelper "HLpHelper"

class HLpHelper
{
public:

	int GetLastReturnFromSolve();
	//Construction
	HLpHelper();
	virtual ~HLpHelper();

	//Implementation
	std::string GetReport();
	


	//lp_solve function wrappers
	bool MakeLp(int rows, int columns);
	bool DeleteLp();					//deletes the current problem
	bool CheckLp();						//returns wether the m_pLp member points to a problem
	bool ReadLP(std::string sLpFilePath, int verbose, std::string sLpName);	//reads an LP-Problem from Textfile (path)
	int Solve();	//solves the current problem
	bool WriteLp(std::string sFileName);	//wraps write_lp function form lp_solve5.1
    bool WriteMps(std::string sFileName);
	
	bool GetPtrConstraints(double **ppConstraints); //get the rh (right hand) values of the constraints after solution
	bool GetPtrVariables(double **ppVariables);		//get the values of all variables after solution
	bool GetVariables(double* pVariables);
	bool GetPtrSensitivityRHS(double** ppDuals, double** ppDualsFrom,
			double** ppDualsTill);
    bool AddColumn(double* column);
    bool AddColumnEx(int count, double *column, int *rowno);
	bool AddConstraintEx(int count, double *row, int *colno, int constr_type, double rh);
	bool SetColumnEx(int col_no, int count, double *column, int *rowno);
	bool SetRowEx(int row_no, int count, double *row, int *colno);
	bool SetObjFnEx(int count, double *row, int *colno);
	bool SetAddRowmode(bool turnon);
	bool SetColName(int column, std::string new_name);
	bool SetLpName(std::string sLpName);
	bool SetRowName(int row, std::string new_name);
	bool SetInt(int column, bool must_be_int);
	bool SetBinary(int column, bool must_be_bin);

	bool IsMaxim();
	bool IsNegative(int column);
	bool IsFeasible(double *values, double threshold);
    bool IsBreakAtFirst();

	void SetTimeout(int secs);
    void SetBreakAtFirst(bool breakAtFirst);
    void SetPresolve(int presolveFlags, int maxloops=-1);

	void SetAbortFunc(void *owningObject, int (*abortfunc)(lprec*,void*));
    void SetLogFunc(void *owningObject, void (*logfunc)(lprec*, void*, char*));
	void SetMinim();
	void SetMaxim();

	int GetSolutionCount();
    int GetNOrigColumns();
	int GetNColumns();
	int GetNRows();
	int GetNameIndex(std::string sName, bool bIsRow);
	/*! get the comparison operator for the given constraint
	 *  return values: 1: '<=' | 2: '>=' | 3: '=' | -1: error */
	int GetConstraintType(int row);
	
	double GetObjective();

	std::string GetLpName();
	std::string GetStatusText();	//gets the status code after solving the problem
    std::string GetOrigColName(int column);
    std::string GetColName(int column);
    std::string GetOrigRowName(int row);
	std::string GetRowName(int row);

private:
	lprec *m_pLp;
	std::string m_sReport;
	int m_iLastReturnFromSolve;
};

#endif // LPHELPER_H
