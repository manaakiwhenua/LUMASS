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
#include "otbMath.h"

#include "otbMacro.h"

#include <muParser/muParser.h>
#include "otbMultiParser.h"

namespace otb
{

class ITK_EXPORT MultiParserImpl : public itk::LightObject
{
public:
  /** Standard class typedefs. */
  typedef MultiParserImpl                               Self;
  typedef itk::LightObject                         Superclass;
  typedef itk::SmartPointer<Self>                  Pointer;
  typedef itk::SmartPointer<const Self>            ConstPointer;

  /** New macro for creation of through a Smart Pointer */
  itkNewMacro(Self);

  /** Run-time type information (and related methods) */
  itkTypeMacro(MultiParserImpl, itk::LightObject);

  /** Convenient type definitions */
  typedef double                                   ValueType;
  typedef mu::Parser::exception_type               ExceptionType;

  /** Initialize user defined constants */
  virtual void InitConst()
  {
    m_MuParser.DefineConst( "e",      CONST_E );
    m_MuParser.DefineConst( "log2e",  CONST_LOG2E );
    m_MuParser.DefineConst( "log10e", CONST_LOG10E );
    m_MuParser.DefineConst( "ln2",    CONST_LN2 );
    m_MuParser.DefineConst( "ln10",   CONST_LN10 );
    m_MuParser.DefineConst( "pi",     CONST_PI );
    m_MuParser.DefineConst( "euler",  CONST_EULER );
  }

  /** Initialize user defined functions */
  virtual void InitFun()
  {
    m_MuParser.DefineFun("ndvi", NDVI);
    m_MuParser.DefineFun("NDVI", NDVI);
  }

  /** Set the expression to be parsed */
  virtual void SetExpr(const std::string & Expression)
  {
    m_MuParser.SetExpr(Expression);
  }

  /** Trigger the parsing */
  ValueType Eval()
  {
    MultiParser::ValueType result = 0.0;
    try
      {
      result = m_MuParser.Eval();
      }
    catch(ExceptionType &e)
      {
      ExceptionHandler(e);
      }
    return result;
  }


  /** Trigger the parsing */
  ValueType* Eval(int& nNum)
  {
    MultiParser::ValueType* result = 0;
    try
      {
      result = m_MuParser.Eval(nNum);
      }
    catch(ExceptionType &e)
      {
      ExceptionHandler(e);
      }
    return result;
  }



  /** Define a variable */
  void DefineVar(const std::string &sName, ValueType *fVar)
  {
    try
      {
      m_MuParser.DefineVar(sName, fVar);
      }
    catch(ExceptionType &e)
      {
      ExceptionHandler(e);
      }
  }

  /** Clear all the defined variables */
  void ClearVar()
  {
    m_MuParser.ClearVar();
  }

  /** Return the expression to be parsed */
  const std::string& GetExpr() const
  {
    return m_MuParser.GetExpr();
  }

  /** Return the list of variables */
  const std::map<std::string, ValueType*>& GetVar() const
  {
    return m_MuParser.GetVar();
  }

  /** Return the number of results from the current expression */
  int GetNumResults(void)
  {
	  return m_MuParser.GetNumResults();
  }

  /**  Check Expression **/
  bool CheckExpr()
  {

    try
    {
      m_MuParser.Eval();
    }
   catch(ExceptionType &e)
    {
     ExceptionHandlerDebug(e);
     return false;
    }

    return true;
  }

  // Get the map with the functions
  MultiParser::FunctionMapType GetFunList() const
  {
    MultiParser::FunctionMapType output;
    const mu::funmap_type& funmap = m_MuParser.GetFunDef();

    mu::funmap_type::const_iterator funItem;

    for (funItem = funmap.begin(); funItem != funmap.end(); ++funItem)
      {
      output[funItem->first] = funItem->second.GetArgc();
      }
    return output;
  }

  /** Convert MultiParser specific exception into itk exception */
  virtual void ExceptionHandler(ExceptionType &e)
  {
    itkExceptionMacro(                                     << std::endl
          << "Message:     "   << e.GetMsg()   << std::endl
          << "Formula:     "   << e.GetExpr()  << std::endl
          << "Token:       "   << e.GetToken() << std::endl
          << "Position:    "   << e.GetPos()   << std::endl
                 << std::endl);
  //        << "Errc:        "   << e.GetCode()  << std::endl);
  }

  /** Convert MultiParser specific exception into itk debug macro */
   virtual void ExceptionHandlerDebug(ExceptionType &e)
   {
     otbGenericMsgDebugMacro(                                     << std::endl
           << "Message:     "   << e.GetMsg()   << std::endl
           << "Formula:     "   << e.GetExpr()  << std::endl
           << "Token:       "   << e.GetToken() << std::endl
           << "Position:    "   << e.GetPos()   << std::endl
                  << std::endl);
   //        << "Errc:        "   << e.GetCode()  << std::endl);
   }


protected:
  MultiParserImpl()
  {
    InitFun();
    InitConst();
  }

  virtual ~MultiParserImpl()
  {
  }

  virtual void PrintSelf(std::ostream& os, itk::Indent indent) const
  {
    Superclass::PrintSelf(os, indent);
  }


private:
  MultiParserImpl(const Self &);             //purposely not implemented
  void operator =(const Self &);    //purposely not implemented

  mu::Parser m_MuParser;

  //----------  User Defined Functions  ----------//BEGIN
  static ValueType NDVI(ValueType r, ValueType niri)
  {
    if ( vcl_abs(r + niri) < 1E-6 )
      {
      return 0.;
      }
    return (niri-r)/(niri+r);
  }

  //----------  User Defined Functions  ----------//END
}; // end class

void MultiParser::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}


MultiParser::MultiParser()
: m_InternalMultiParser( MultiParserImpl::New() )
{
}

MultiParser::~MultiParser()
{
}

void MultiParser::SetExpr(const std::string & Expression)
{
  m_InternalMultiParser->SetExpr(Expression);
}

MultiParser::ValueType MultiParser::Eval()
{
  return m_InternalMultiParser->Eval();
}

MultiParser::ValueType* MultiParser::Eval(int& nNum)
{
	return m_InternalMultiParser->Eval(nNum);
}

void MultiParser::DefineVar(const std::string &sName, MultiParser::ValueType *fVar)
{
  m_InternalMultiParser->DefineVar(sName, fVar);
}

void MultiParser::ClearVar()
{
  m_InternalMultiParser->ClearVar();
}

bool MultiParser::CheckExpr()
{
  return m_InternalMultiParser->CheckExpr();
}

int MultiParser::GetNumResults()
{
	return m_InternalMultiParser->GetNumResults();
}

const std::string& MultiParser::GetExpr() const
{
  return m_InternalMultiParser->GetExpr();
}

// Get the map with the variables
const std::map<std::string, MultiParser::ValueType*>& MultiParser::GetVar() const
{
  return m_InternalMultiParser->GetVar();
}

// Get the map with the functions
MultiParser::FunctionMapType MultiParser::GetFunList() const
{
  return m_InternalMultiParser->GetFunList();
}

}//end namespace otb