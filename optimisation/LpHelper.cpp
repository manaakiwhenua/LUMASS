// LpHelper.cpp:
//
//////////////////////////////////////////////////////////////////////

#include "LpHelper.h"

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

HLpHelper::HLpHelper()
{
	m_sReport = "";
	m_pLp = NULL;
	m_iLastReturnFromSolve = -9;
}

HLpHelper::~HLpHelper()
{
	//delete lp
	if (m_pLp != NULL)
		 this->DeleteLp();
}

////////////////////////////////////////////////////////////////////////
//Implementation
////////////////////////////////////////////////////////////////////////

bool HLpHelper::MakeLp(int rows, int columns)
{
	NMDebugCtx(ctxLpHelper, << "...");

	//make the lp file
	if (m_pLp != NULL)
		this->DeleteLp();
	
	this->m_pLp = make_lp(rows, columns);

	NMDebugCtx(ctxLpHelper, << "done!");

	if (this->m_pLp == NULL)
		return false;
	else
		return true;

}

bool HLpHelper::CheckLp()
{
	//check valid problem
	if (this->m_pLp == NULL)
		return false;
	else
		return true;
}

bool HLpHelper::ReadLP(std::string sLpFilePath, int verbose,
	std::string sLpName)
{
	NMDebugCtx(ctxLpHelper, << "...");

	//check, if file name is empty
	if (sLpFilePath.empty())
		return false;
	
	//read the lp file
	if (this->m_pLp != NULL)
		this->DeleteLp();
	this->m_pLp = read_LP(const_cast<char*>(sLpFilePath.c_str()), verbose,
					const_cast<char*>(sLpName.c_str()));

	if (this->m_pLp == NULL)
		return false;

	NMDebugCtx(ctxLpHelper, << "done!");

	//if everything is fine
	return true;
}

bool HLpHelper::DeleteLp()
{
	//check valid lp
	if (!this->CheckLp())
		return false;

	//delete problem
	delete_lp(this->m_pLp);
	this->m_pLp = 0;

	//go back 
	return true;
}

int HLpHelper::Solve()
{
	NMDebugCtx(ctxLpHelper, << "...");

	//check valid lp
	if (!this->CheckLp())
		return -9;

	// set the presovle option for duals
	// since we might have some integer vars in
	// the problem
	set_presolve(this->m_pLp, PRESOLVE_SENSDUALS, get_presolveloops(this->m_pLp));

	//solve the problem
	m_iLastReturnFromSolve = solve(this->m_pLp);

	NMDebugCtx(ctxLpHelper, << "done!");

	return m_iLastReturnFromSolve;
}

int HLpHelper::GetConstraintType(int row)
{
	int ret = -1;

	//check valid lp
	if (!this->CheckLp())
		return ret;

	// get the constraints type
	ret = get_constr_type(this->m_pLp, row);

	return ret;
}

bool HLpHelper::GetPtrConstraints(double **ppConstraints)
{
	NMDebugCtx(ctxLpHelper, << "...");

	//check valid lp
	if (!this->CheckLp())
		return false;

	//get the constraints
	bool ret;
	if (get_ptr_constraints(this->m_pLp, (REAL**)ppConstraints))
		ret = true;
	else
		ret = false;

	NMDebugCtx(ctxLpHelper, << "done!");
	return ret;
}

bool HLpHelper::GetPtrSensitivityRHS(double** ppDuals, double** ppDualsFrom,
		double** ppDualsTill)
{
	//check valid lp
	if (!this->CheckLp())
		return false;

	//get the variables
	bool ret;
	if (get_ptr_sensitivity_rhs(this->m_pLp, (REAL**)ppDuals,
			(REAL**)ppDualsFrom, (REAL**)ppDualsTill))
		ret = true;
	else
		ret = false;

	return ret;
}

std::string HLpHelper::GetOrigRowName(int row)
{
    std::string ret = "error";

    //check valid lp
    if (!this->CheckLp())
        return ret;

    //get the variables
    const char* sr = get_origrow_name(this->m_pLp, row);
    if (sr == NULL)
        return ret;

    return sr;
}

std::string HLpHelper::GetRowName(int row)
{
	std::string ret = "error";

	//check valid lp
	if (!this->CheckLp())
		return ret;

	//get the variables
	const char* sr = get_row_name(this->m_pLp, row);
	if (sr == NULL)
		return ret;

	return sr;
}

bool HLpHelper::GetPtrVariables(double **ppVariables)
{
	NMDebugCtx(ctxLpHelper, << "...");

	//check valid lp
	if (!this->CheckLp())
		return false;

	//get the variables
	bool ret;
	if (get_ptr_variables(this->m_pLp, (REAL**)ppVariables))
		ret = true;
	else
		ret = false;

	NMDebugCtx(ctxLpHelper, << "done!");
	return ret;
}

bool HLpHelper::GetVariables(double* pVariables)
{
	NMDebugCtx(ctxLpHelper, << "...");

	//check valid lp
	if (!this->CheckLp())
		return false;

	//get the variables
	bool ret;
	if (get_variables(this->m_pLp, (REAL*)pVariables))
		ret = true;
	else
		ret = false;

	NMDebugCtx(ctxLpHelper, << "done!");
	return ret;
}
	
int HLpHelper::GetSolutionCount()
{

	//check valid lp
	if (!this->CheckLp())
		return -9;

	return get_solutioncount(this->m_pLp);
}

int HLpHelper::GetNOrigColumns()
{

    //check valid lp
    if (!this->CheckLp())
        return -9;

    return get_Norig_columns(this->m_pLp);
}

int HLpHelper::GetNColumns()
{

	//check valid lp
	if (!this->CheckLp())
		return -9;

	return get_Ncolumns(this->m_pLp);
}

int HLpHelper::GetNRows()
{

	//check valid lp
	if (!this->CheckLp())
		return -9;

	return get_Nrows(this->m_pLp);
}

double HLpHelper::GetObjective()
{
	//check valid lp
	if (!this->CheckLp())
		return -9;

	return (double)get_objective(this->m_pLp);
}

std::string HLpHelper::GetLpName()
{
	//check valid lp
	if (!this->CheckLp())
		return std::string("error");

	return get_lp_name(this->m_pLp);
}

std::string HLpHelper::GetStatusText()
{
	//check valid lp
	if (!this->CheckLp())
		return std::string("error");

	return get_statustext(this->m_pLp, this->m_iLastReturnFromSolve);
}

std::string HLpHelper::GetOrigColName(int column)
{
    //check valid lp
    if (!this->CheckLp())
        return std::string("error");

    char* name = get_origcol_name(this->m_pLp, column);
    if (name == 0)
        return std::string("");
    else
        return std::string(name);

}

std::string HLpHelper::GetColName(int column)
{
	//check valid lp
	if (!this->CheckLp())
		return std::string("error");

    char* name = get_col_name(this->m_pLp, column);
    if (name == 0)
        return std::string("");
    else
        return std::string(name);
}

int HLpHelper::GetNameIndex(std::string sName, bool bIsRow)
{
	//check valid lp
	if (!this->CheckLp())
		return -9;

	return get_nameindex(this->m_pLp,
			const_cast<char*>(sName.c_str()), bIsRow);
}

bool HLpHelper::AddColumn(double* column)
{
    //check valid lp
    if (!this->CheckLp())
        return false;

    if (add_column(this->m_pLp, (REAL*)column))
        return true;
    else
        return false;
}

bool HLpHelper::AddColumnEx(int count, double *column, int *rowno)
{
	//check valid lp
	if (!this->CheckLp())
		return false;

	if (add_columnex(this->m_pLp, count, (REAL*)column, rowno))
		return true;
	else
		return false;
}

bool HLpHelper::AddConstraintEx(int count, double *row, int *colno, int constr_type, double rh)
{
	//check valid lp
	if (!this->CheckLp())
		return false;	
	
	if (add_constraintex(this->m_pLp, count, (REAL*)row, colno, constr_type, (REAL)rh))
		return true;
	else
		return false;
}

bool HLpHelper::SetColumnEx(int col_no, int count, double *column, int *rowno)
{
	//check valid lp
	if (!this->CheckLp())
		return false;	
	
	if (set_columnex(this->m_pLp, col_no, count, (REAL*)column, rowno))
		return true;
	else
		return false;
}

bool HLpHelper::SetRowEx(int row_no, int count, double *row, int *colno)
{
	//check valid lp
	if (!this->CheckLp())
		return false;	
	
	if (set_rowex(this->m_pLp, row_no, count, (REAL*)row, colno))
		return true;
	else
		return false;
}

bool HLpHelper::SetObjFnEx(int count, double *row, int *colno)
{
	//check valid lp
	if (!this->CheckLp())
		return false;	
	
	if (set_obj_fnex(this->m_pLp, count, (REAL*)row, colno))
		return true;
	else
		return false;
}

bool HLpHelper::SetAddRowmode(bool turnon)
{
	//check valid lp
	if (!this->CheckLp())
		return false;	

	if (set_add_rowmode(m_pLp, turnon))
		return true;
	else
		return false;
}

bool HLpHelper::SetInt(int column, bool must_be_int)
{
	//check valid lp
	if (!this->CheckLp())
		return false;	

	if (set_int(m_pLp, column, must_be_int))
		return true;
	else
		return false;	
}

bool HLpHelper::SetBinary(int column, bool must_be_bin)
{
	//check valid lp
	if (!this->CheckLp())
		return false;	

	if (set_binary(this->m_pLp, column, must_be_bin))
		return true;
	else
		return false;	
}

void HLpHelper::SetPresolve(int presolveFlags, int maxloops)
{
    //check valid lp
    if (!this->CheckLp())
        return;

    int nloops = get_presolveloops(this->m_pLp);
    if (maxloops == -1)
    {
        maxloops = nloops;
    }

    set_presolve(this->m_pLp, presolveFlags, maxloops);
}

void HLpHelper::SetScaling(int scalemode)
{
    if (this->CheckLp())
    {
        set_scaling(this->m_pLp, scalemode);
    }
}

void HLpHelper::SetMinim()
{
	//check valid lp
	if (!this->CheckLp())
		return;	

	set_minim(this->m_pLp);
}

void HLpHelper::SetMaxim()
{
	//check valid lp
	if (!this->CheckLp())
		return;	

	set_maxim(this->m_pLp);
}

bool HLpHelper::IsMaxim()
{
	//check valid lp
	if (!this->CheckLp())
		return false;	

	if (is_maxim(this->m_pLp))
		return true;
	else
		return false;
}

bool HLpHelper::IsNegative(int column)
{
	//check valid lp
	if (!this->CheckLp())
		return false;	

	if (is_negative(this->m_pLp, column))
		return true;
	else
		return false;
}

bool HLpHelper::IsFeasible(double *values, double threshold)
{
	//check valid lp
	if (!this->CheckLp())
		return false;	

	if (is_feasible(this->m_pLp, (REAL*)values, (REAL)threshold))
		return true;
	else
		return false;
}

bool HLpHelper::SetColName(int column, std::string new_name)
{
	//check valid lp
	if (!this->CheckLp())
		return false;	

	if (set_col_name(this->m_pLp, column,
			const_cast<char*>(new_name.c_str())))
		return true;
	else
		return false;
}

bool HLpHelper::SetRowName(int row, std::string new_name)
{
	//check valid lp
	if (!this->CheckLp())
		return false;	

	if (set_row_name(this->m_pLp, row,
			const_cast<char*>(new_name.c_str())))
		return true;
	else
		return false;
}

bool HLpHelper::WriteLp(std::string sFileName)
{
	NMDebugCtx(ctxLpHelper, << "...");

	//check valid lp
	if (!this->CheckLp())
		return false;	

	MYBOOL ret = write_lp(this->m_pLp,
			const_cast<char*>(sFileName.c_str()));

	NMDebugCtx(ctxLpHelper, << "done!");
	return ret;
}

bool HLpHelper::WriteMps(std::string sFileName)
{
    NMDebugCtx(ctxLpHelper, << "...");

    //check valid lp
    if (!this->CheckLp())
        return false;

    MYBOOL ret = write_mps(this->m_pLp,
            const_cast<char*>(sFileName.c_str()));

    NMDebugCtx(ctxLpHelper, << "done!");
    return ret;
}

bool HLpHelper::SetLpName(std::string sLpName)
{
	//check valid lp
	if (!this->CheckLp())
		return false;	

	if (set_lp_name(this->m_pLp,
			const_cast<char*>(sLpName.c_str())))
		return true;
	else
		return false;
}

std::string HLpHelper::GetReport()
{
	NMDebugCtx(ctxLpHelper, << "...");
	
	//check valid lp
	if (!this->CheckLp())
		return "";
	
	//check, if model has been solved yet successfully
	std::stringstream ssErrorString;
	ssErrorString << this->GetLpName() << "\n\nThe Problem has not yet been solved or\n"
						"it has no optimal or sub-optimal solution!";
	if (!(this->m_iLastReturnFromSolve == 0 || this->m_iLastReturnFromSolve == 1))
		return ssErrorString.str();
	
	//report variables
	std::string sStatus;
	std::string sLpname;
	REAL rObj;
	REAL *prVars;
	REAL *prCons;
	int iNumSol, iNumCol, iNumCons;
	
	//report values
	sLpname = this->GetLpName();
	sStatus = this->GetStatusText();
	rObj = this->GetObjective();
	this->GetPtrVariables(&prVars);
	this->GetPtrConstraints(&prCons);
	iNumSol = this->GetSolutionCount();
	iNumCol = this->GetNColumns();
	iNumCons = this->GetNRows();
	
	//print report header
	std::stringstream ssReport;
	ssReport <<
		sLpname << "\n\n" <<
		sStatus << "\n" <<
		"Number of Solutions: " << iNumSol << "\n\n" <<
		"Objective function result: " << rObj << std::endl;
		
	//get variables
	ssReport << "\nVARIABLES\n";
	for (int i=0; i < iNumCol-1; i++)
		ssReport << this->GetColName(i+1) << ": " << prVars[i] << std::endl;

	//get constraints
	ssReport << "\nCONSTRAINTS\n";
	for (int i=0; i < iNumCons; i++)
		ssReport << this->GetRowName(i+1) << ": " << prCons[i] << std::endl;
	
	NMDebugCtx(ctxLpHelper, << "done!");

	//if everything is fine
	return ssReport.str();
}


int HLpHelper::GetLastReturnFromSolve()
{
	return m_iLastReturnFromSolve;
}

void HLpHelper::SetTimeout(int secs)
{
	if (secs >= 0)
		set_timeout(this->m_pLp, secs);
}

void HLpHelper::SetBreakAtFirst(bool breakAtFirst)
{
    set_break_at_first(this->m_pLp, breakAtFirst);
}

void HLpHelper::SetBreakAtValue(double breakValue)
{
    set_break_at_value(this->m_pLp, breakValue);
}

bool HLpHelper::IsBreakAtFirst()
{
    return is_break_at_first(this->m_pLp);
}

void HLpHelper::SetAbortFunc(void *owningObject, int (*abortfunc)(lprec*, void*))
{
	put_abortfunc(this->m_pLp, abortfunc, owningObject);
}

void HLpHelper::SetLogFunc(void *owningObject, void (*logfunc)(lprec *, void *, char *))
{
    put_logfunc(this->m_pLp, logfunc, owningObject);
}
