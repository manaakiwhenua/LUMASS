/*=========================================================================

  Program:   ORFEO Toolbox
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


  Copyright (c) Centre National d'Etudes Spatiales. All rights reserved.
  See OTBCopyright.txt for details.


     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include "nmlog.h"
#include "otbMath.h"
//#include "otbMacro.h"
#include "otbMultiParser.h"

namespace otb
{

void MultiParser::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}


MultiParser::MultiParser()
{
    // define constants
    m_InternalMultiParser.DefineConst( "e",      CONST_E );
    m_InternalMultiParser.DefineConst( "log2e",  CONST_LOG2E );
    m_InternalMultiParser.DefineConst( "log10e", CONST_LOG10E );
    m_InternalMultiParser.DefineConst( "ln2",    CONST_LN2 );
    m_InternalMultiParser.DefineConst( "ln10",   CONST_LN10 );
    m_InternalMultiParser.DefineConst( "pi",     CONST_PI );
    m_InternalMultiParser.DefineConst( "euler",  CONST_EULER );

    // init the random number generator fun
    //srand(time(0));
    m_InternalMultiParser.DefineFun("rand", (mu::fun_type2)MultiParser::rnum, false);

    // init fmod function
    m_InternalMultiParser.DefineFun("fmod", (mu::fun_type2)MultiParser::calcMod);

    m_InternalMultiParser.DefineFun("unifdist_int", (mu::fun_type2)MultiParser::unifdist_int);
    m_InternalMultiParser.DefineFun("unifdist_real", (mu::fun_type2)MultiParser::unifdist_real);

    m_InternalMultiParser.DefineFun("lndist", (mu::fun_type2)MultiParser::lndist, false);
    m_InternalMultiParser.DefineFun("normdist", (mu::fun_type2)MultiParser::normdist, false);


}

MultiParser::~MultiParser()
{
}

void MultiParser::SetExpr(const std::string & Expression)
{
  m_InternalMultiParser.SetExpr(Expression);
}

MultiParser::ValueType MultiParser::Eval()
{
  return m_InternalMultiParser.Eval();
}

MultiParser::ValueType* MultiParser::Eval(int& nNum)
{
    return m_InternalMultiParser.Eval(nNum);
}

const MultiParser::CharType* MultiParser::ValidNameChars() const
{
    return m_InternalMultiParser.ValidNameChars();
}

void MultiParser::DefineVar(const StringType &sName, MultiParser::ValueType *fVar)
{
  m_InternalMultiParser.DefineVar(sName, fVar);
}

void MultiParser::DefineConst(const StringType &sName, const ValueType &val)
{
    m_InternalMultiParser.DefineConst(sName, val);
}

void MultiParser::DefineStrConst(const StringType &sName, const StringType& sVal)
{
    m_InternalMultiParser.DefineStrConst(sName, sVal);
}

void MultiParser::ClearVar()
{
  m_InternalMultiParser.ClearVar();
}

bool MultiParser::CheckExpr()
{
    try
    {
        m_InternalMultiParser.Eval();
    }
    catch(mu::Parser::exception_type &evalerr)
    {
        NMErr("MultiParser",
              << std::endl
              << "Message:    " << evalerr.GetMsg() << std::endl
              << "Formula:    " << evalerr.GetExpr() << std::endl
              << "Token:      " << evalerr.GetToken() << std::endl
              << "Position:   " << evalerr.GetPos() << std::endl << std::endl;
              )
        return false;
    }

    return true;
}

int MultiParser::GetNumResults()
{
    return m_InternalMultiParser.GetNumResults();
}

const std::string& MultiParser::GetExpr() const
{
  return m_InternalMultiParser.GetExpr();
}

// Get the map with the variables
const std::map<std::string, MultiParser::ValueType*>& MultiParser::GetVar() const
{
  return m_InternalMultiParser.GetVar();
}

//// Get the map with the functions
//MultiParser::FunctionMapType MultiParser::GetFunList() const
//{
//  return m_InternalMultiParser.GetFunList();
//}

}//end namespace otb
