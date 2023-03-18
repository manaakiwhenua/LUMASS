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
#include "utils/muParser/muParser.h"
#include "utils/muParser/muParserDef.h"
#include <random>
//#include "otbsuppl/filters/otbMultiParserImpl.h"

#include "nmotbsupplfilters_export.h"

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
class NMOTBSUPPLFILTERS_EXPORT MultiParser : public itk::LightObject
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
    typedef MultiParser                     ParserType;
    typedef mu::value_type                  ValueType;
    typedef mu::string_type                 StringType;
    typedef mu::string_type::value_type     CharType;


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
    void DefineFun(const mu::string_type& sName, T funPtr, bool bIsOptimisable=true)
    {
        m_InternalMultiParser.DefineFun(sName, funPtr, bIsOptimisable);
    }

    /** set of valid name charcters, i.e. variables, constants **/
    const CharType* ValidNameChars() const;

    /** Clear all the defined variables */
    void ClearVar();

    /** Return the expression to be parsed */
    const std::string& GetExpr() const;

    /** Return the list of variables */
    const std::map<std::string, MultiParser::ValueType*>& GetVar() const;

    /** Return a map of function names and associated number of arguments */
    //FunctionMapType GetFunList() const;

    /**  Check Expression **/
    bool CheckExpr();

    /** Get the number of results returned by the parser */
    int GetNumResults();

    static ValueType rnum(ValueType lower, ValueType upper)
    {
        static std::random_device rand_rd;
        static std::mt19937 rand_mt(rand_rd());
        static std::uniform_int_distribution<int> rand_dist_int(lower, upper);
        return rand_dist_int(rand_mt);
    }

    static ValueType calcMod(ValueType numer, ValueType denom)
    {
        return std::fmod(numer, denom);
    }

    static ValueType unifdist_int(ValueType lower, ValueType upper)
    {
        static std::random_device unifdist_int_rd;
        static std::mt19937 unifdist_int_mt(unifdist_int_rd());
        static std::uniform_int_distribution<int> unifdist_int(lower, upper);
        return unifdist_int(unifdist_int_mt);
    }

    static ValueType unifdist_real(ValueType lower, ValueType upper)
    {
        static std::random_device unifdist_dbl_rd;
        static std::mt19937 unifdist_dbl_mt(unifdist_dbl_rd());
        static std::uniform_real_distribution<double> unifdist_dbl(lower, upper);
        return unifdist_dbl(unifdist_dbl_mt);
    }


    static ValueType lndist(ValueType m, ValueType s)
    {
        static std::random_device lndist_rd;
        static std::mt19937 lndist_mt(lndist_rd());
        static std::lognormal_distribution<double> lndist(m, s);
        return lndist(lndist_mt);
    }

    static ValueType normdist(ValueType m, ValueType s)
    {
        static std::random_device normdist_rd;
        static std::mt19937 normdist_mt(normdist_rd());
        static std::normal_distribution<double> normdist(m, s);
        return normdist(normdist_mt);
    }


protected:
    MultiParser();
    virtual ~MultiParser();
    virtual void PrintSelf(std::ostream& os, itk::Indent indent) const;


private:
    MultiParser(const Self &);             //purposely not implemented
    void operator =(const Self &);    //purposely not implemented

    //    typedef itk::SmartPointer<mu::Parser> MultiParserImplPtr;
    //    MultiParserImplPtr m_InternalMultiParser;

    mu::Parser m_InternalMultiParser;

}; // end class

}//end namespace otb


#endif
