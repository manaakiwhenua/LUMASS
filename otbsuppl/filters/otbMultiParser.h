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
#ifndef __otbMultiParser_h
#define __otbMultiParser_h

#include "itkLightObject.h"
#include "itkObjectFactory.h"
#include "muParserDef.h"
#include "otbMultiParserImpl.h"

#include "otbsupplfilters_export.h"

namespace otb
{

//class MultiParserImpl;


/** \class MultiParser
 * \brief  Definition of the standard floating point parser.
 * Standard implementation of the mathematical expressions parser.
 *
 * \sa BandMathImageFilter
 *
 */
class OTBSUPPLFILTERS_EXPORT MultiParser : public itk::LightObject
{
public:
  /** Standard class typedefs. */
  typedef MultiParser                                   Self;
  typedef itk::LightObject                         Superclass;
  typedef itk::SmartPointer<Self>                  Pointer;
  typedef itk::SmartPointer<const Self>            ConstPointer;

  /** New macro for creation of through a Smart Pointer */
  itkNewMacro(Self);
  
  /** Run-time type information (and related methods) */
  itkTypeMacro(MultiParser, itk::LightObject);
  
  /** Convenient type definitions */
  typedef MultiParser                              ParserType;
  typedef typename mu::value_type                  ValueType;
  typedef typename mu::string_type                 StringType;
  typedef typename mu::string_type::value_type     CharType;


  /** Type for function/number of arguments map */
  typedef std::map<std::string, int>               FunctionMapType;

  /** Set the expression to be parsed */
  virtual void SetExpr(const std::string & Expression);
  
  /** Trigger the parsing */
  ValueType Eval();

  /** in case we've got multiple expressions to parse */
  ValueType* Eval(int& nNum);

  /** Define a variable */
  void DefineVar(const StringType &sName, ValueType *fVar);

  /** Define a constant */
  void DefineConst(const StringType& sName, const ValueType& val);
  void DefineStrConst(const StringType& sName, const StringType &sVal);

  template <typename T>
  void DefineFun(const StringType& sName, T funPtr, bool bIsOptimisable=true)
  {
      m_InternalMultiParser->DefineFun(sName, funPtr, bIsOptimisable);
  }
  
  /** Clear all the defined variables */
  void ClearVar();

  /** Return the expression to be parsed */
  const std::string& GetExpr() const;

  /** Return the list of variables */
  const std::map<std::string, MultiParser::ValueType*>& GetVar() const;

  /** Return a map of function names and associated number of arguments */
  FunctionMapType GetFunList() const;

  /**  Check Expression **/
  bool CheckExpr();

  /** Get the number of results returned by the parser */
  int GetNumResults();

protected:
  MultiParser();
  virtual ~MultiParser();
  virtual void PrintSelf(std::ostream& os, itk::Indent indent) const;


private:
  MultiParser(const Self &);             //purposely not implemented
  void operator =(const Self &);    //purposely not implemented

  typedef itk::SmartPointer<MultiParserImpl> MultiParserImplPtr;
  MultiParserImplPtr m_InternalMultiParser;
}; // end class

}//end namespace otb


#endif
