/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2016 Landcare Research New Zealand Ltd
 *
 * This file is part of 'LUMASS', which is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/
/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkNMScriptableKernelFilter2.txx,v $
  Language:  C++
  Date:      $Date: 2008-10-16 18:05:25 $
  Version:   $Revision: 1.16 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __otbNMScriptableKernelFilter2_txx
#define __otbNMScriptableKernelFilter2_txx


// First make sure that the configuration is available.
// This line can be removed once the optimized versions
// gets integrated into the main directories.
//#include "itkConfigure.h"
//
//#ifdef ITK_USE_CONSOLIDATED_MORPHOLOGY
//#include "itkOptNMScriptableKernelFilter2.txx"
//#else

#include "otbNMScriptableKernelFilter2.h"

#include <algorithm>
#include <stack>

#include "itkConstNeighborhoodIterator.h"
#include "itkNeighborhoodInnerProduct.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkZeroFluxNeumannBoundaryCondition.h"
#include "itkOffset.h"
#include "itkProgressReporter.h"
#include "utils/muParser/muParserError.h"
#include "otbKernelScriptParserError.h"
#include "otbAttributeTable.h"

//#include "valgrind/callgrind.h"


//namespace itk
//{

//KernelScriptParserError::KernelScriptParserError()
//    : ExceptionObject()
//{}

//KernelScriptParserError::KernelScriptParserError(const char* file, unsigned int lineNumber)
//    : ExceptionObject(file, lineNumber)
//{}


//KernelScriptParserError::KernelScriptParserError(const std::string& file, unsigned int lineNumber)
//    : ExceptionObject(file, lineNumber)
//{}

//KernelScriptParserError &
//KernelScriptParserError::operator=(const KernelScriptParserError& orig)
//{
//    ExceptionObject::operator=(orig);
//    return *this;
//}

//}


namespace otb
{

template <class TInputImage, class TOutputImage>
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::NMScriptableKernelFilter2()
{
    m_Radius.Fill(0);

    // <Square> and <Circle>
    m_KernelShape = "Square";

    m_PixelCounter = 0;

    m_OutputVarName = "out";

    m_Nodata = itk::NumericTraits<OutputPixelType>::NonpositiveMin();

    m_This = static_cast<double>(reinterpret_cast<uintptr_t>(this));

    // just for debug
    //this->SetNumberOfThreads(1);
}

template <class TInputImage, class TOutputImage>
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::~NMScriptableKernelFilter2()
{
    this->Reset();
}

template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::SetRadius(const int *radius)
{
    typename InputImageType::SizeValueType svt[TInputImage::ImageDimension];
    for (int i=0; i < TInputImage::ImageDimension; ++i)
    {
        svt[i] = static_cast<typename InputImageType::SizeValueType>(radius[i]);
    }
    m_Radius.SetSize(svt);
    this->Modified();
}

template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::SetNodata(const double &nodata)
{
    m_Nodata = static_cast<OutputPixelType>(nodata);
    this->Modified();
}

template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::Reset()
{
    m_KernelShape = "Square";
    m_ActiveKernelOffsets.clear();

    m_PixelCounter = 0;
    m_NumPixels = 0;
    m_NumOverflows.clear();
    m_NumUnderflows.clear();

    m_mapNameImgValue.clear();
    m_mapNameAuxValue.clear();
    m_mapNameImg.clear();
    m_mapNameTable.clear();
    m_vOutputValue.clear();
    m_vecBlockLen.clear();

    m_vecParsers.clear();
    m_mapParserName.clear();

    this->Modified();
}


template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::SetInputNames(const std::vector<std::string> &inputNames)
{
    m_DataNames.clear();
    for (int n=0; n < inputNames.size(); ++n)
    {
        m_DataNames.push_back(inputNames.at(n));
    }
    this->Modified();
}


template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::SetFilterInput(const unsigned int& idx, itk::DataObject* dataObj)
{
    itk::ProcessObject::SetNthInput(idx, dataObj);
}

template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::setRAT(unsigned int idx, AttributeTable::Pointer table)
{
    // NOTE: here we rely on the fact that the NMProcess wrapper
    // class links parameters (i.e. DataNames) before it
    // links input data (i.e. images and tables);
    // furthermore, if an image with an associated RAT is
    // provided, we've got only one name associated with it
    // (since the UserID of the reader component is forwarded,
    // and stored in m_DataNames), so we just append '_t' to the
    // image name, to distinguish between the two in the parser formula

    std::vector<std::string>::iterator nameIt = m_DataNames.begin();
    if (idx == this->GetNumberOfIndexedInputs()-1)
    {
        std::string name = m_DataNames.at(idx);
        name += "_t";
        m_DataNames.insert(nameIt+idx+1, name);
        this->SetFilterInput(idx+1, table.GetPointer());
    }
    else
    {
        this->SetFilterInput(idx, table.GetPointer());
    }

    this->Modified();
}

template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::BeforeThreadedGenerateData()
{
    // make sure all images share the same size
    int fstImg = 0;
    typename InputImageType::SizeValueType refSize[TInputImage::ImageDimension];
    for (int i=0; i < this->GetNumberOfIndexedInputs(); ++i)
    {
        InputImageType* img = dynamic_cast<InputImageType*>(this->GetIndexedInputs().at(i).GetPointer());
        if (img != 0)
        {
            if (fstImg == 0)
            {
                for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
                {
                    refSize[d] = img->GetLargestPossibleRegion().GetSize(d);
                }
                fstImg = 1;
            }
            else
            {
                for (unsigned int d=0; d < TInputImage::ImageDimension; ++d)
                {
                    if (refSize[d] != img->GetLargestPossibleRegion().GetSize(d))
                    {
                        KernelScriptParserError e;
                        e.SetLocation(ITK_LOCATION);
                        e.SetDescription("Input images don't have the same size!");
                        throw e;
                    }
                }
            }
        }
    }

    // initiate the parser admin structures if
    // we've just started working on this image ...
    if (m_PixelCounter == 0)
    {
        // create kernel image vlaue store for this object
        std::vector<std::map<std::string, std::vector<ParserValue> > > mapNameImgVal;
        m_mapNameImgValue[m_This] = mapNameImgVal;

        // create table value stor vor this object
        std::map<std::string, std::vector<std::vector<ParserValue> > > mapNameTable;
        m_mapNameTable[m_This] = mapNameTable;


        m_thid.clear();

        const ParserValue nv = itk::NumericTraits<ParserValue>::NonpositiveMin();
        for (int th=0; th < this->GetNumberOfThreads(); ++th)
        {
            std::vector<ParserPointerType> mpvec;
            m_vecParsers.push_back(mpvec);

            m_vOutputValue.push_back(nv);

            std::map<std::string, std::vector<ParserValue> > mapnameval;
            m_mapNameImgValue[m_This].push_back(mapnameval);

            std::map<std::string, ParserValue> mapnamauxval;
            m_mapNameAuxValue.push_back(mapnamauxval);

            m_thid.push_back(th);

        }

        // update input data
        this->CacheInputData();

        // parse the script only once at the
        // beginning
        this->ParseScript();

    }

    m_vthPixelCounter.clear();
    m_vthPixelCounter.resize(this->GetNumberOfThreads(), 0);

    m_NumOverflows.clear();
    m_NumOverflows.resize(this->GetNumberOfThreads(), 0);

    m_NumUnderflows.clear();
    m_NumUnderflows.resize(this->GetNumberOfThreads(), 0);

    // if we've got a shaped neighbourhood iterator,
    // determine the active offsets
    /// ToDo: later ...




}

template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::ParseCommand(const std::string &expr)
{
    std::string name = "";

    // extract newly defined variables (i.e. lvalues)
    size_t pos = std::string::npos;
    if ((pos = expr.find('=')) != std::string::npos)
    {
        name = expr.substr(0, pos);

        // trim off whitespaces at the beginning and end
        name.erase(0, name.find_first_not_of(' '));
        name.erase(name.find_last_not_of(' ')+1);
    }

    // set the RHS as the expression
    std::string theexpr = expr.substr(pos+1);
    theexpr.erase(0, theexpr.find_first_not_of(' '));
    theexpr.erase(theexpr.find_last_not_of(' ')+1);

    if (name.empty())
    {
        std::stringstream sstemp;
        sstemp.str("");
        sstemp << "v" << m_vecParsers.at(0).size();
        name = sstemp.str();
    }

    // allocte a new parser for this command, set the expression and
    // define all previously defined auxillirary and img variables
    // for this parser
    // note: this could possibly be made smarter such that we only define
    // those variables which actually feature in this expression,
    // but performance gains would probably be minimal to non-existant
    // anyway(?), so we don't bother for now
    for (int th=0; th < this->GetNumberOfThreads(); ++th)
    {

        ParserPointerType parser = ParserType::New();
        parser->DefineVar("thid", static_cast<ParserValue*>(&m_thid[th]));
        parser->DefineVar("addr", static_cast<ParserValue*>(&m_This));
        parser->DefineFun("kwinVal", (mu::strfun_type4)kwinVal, true);
        parser->DefineFun("tabVal", (mu::strfun_type4)tabVal, true);

        // enter new variables into the name-variable map
        // note: this only applies to 'auxillary' data and
        // not to any of the pre-defined inputs ...
        //if (name.compare(m_OutputVarName) != 0)
        {
            if (m_mapNameAuxValue.at(th).find(name) == m_mapNameAuxValue.at(th).end())
            {
                m_mapNameAuxValue[th][name] = itk::NumericTraits<ParserValue>::NonpositiveMin();
            }
        }

        // define all previous variables for this parser
        std::map<std::string, ParserValue>::iterator extIter = m_mapNameAuxValue.at(th).begin();
        while (extIter != m_mapNameAuxValue.at(th).end())
        {
            parser->DefineVar(extIter->first, &extIter->second);
            ++extIter;
        }

        std::map<std::string, std::vector<ParserValue > >::iterator kernIter =
                m_mapNameImgValue[m_This][th].begin();
        if (m_NumNeighbourPixel)
        {
            while (kernIter != m_mapNameImgValue[m_This][th].end())
            {
                parser->DefineStrConst(kernIter->first, kernIter->first);
                ++kernIter;
            }
        }
        else
        {
            while (kernIter != m_mapNameImgValue[m_This][th].end())
            {
                parser->DefineVar(kernIter->first, &kernIter->second[0]);
                ++kernIter;
            }
        }

        std::map<std::string, std::vector<std::vector<ParserValue> > >::iterator tabIter =
                m_mapNameTable[m_This].begin();
        while(tabIter != m_mapNameTable[m_This].end())
        {
            parser->DefineStrConst(tabIter->first, tabIter->first);
            ++tabIter;
        }

        // we also need to define a variable for the output image
        //parser->DefineVar(m_OutputVarName, &m_vOutputValue[th]);

        parser->SetExpr(theexpr);

        m_vecParsers[th].push_back(parser);
        m_mapParserName[parser.GetPointer()] = name;
    }
}

template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::ParseScript()
{
    if (m_KernelScript.empty())
    {
        KernelScriptParserError eo;
        eo.SetDescription("Parsing Error: Empty KernelScript object!");
        eo.SetLocation(ITK_LOCATION);
        throw eo;
    }

    enum ScriptElem {CMD,
                     FOR_ADMIN,
                     FOR_BLOCK};

    std::stack<int> forLoop;    // parser vector index of 1st for admin cmd
    std::stack<int> bracketOpen;

    std::string script = m_KernelScript;

    // remove all single (') and double quotes (")
    std::string quotes = "\"";
    for (int q=0; q < quotes.size(); ++q)
    {
        script.erase(std::remove(script.begin(), script.end(), quotes.at(q)), script.end());
    }

    size_t pos = 0;
    size_t start = 0;
    size_t next = 0;
    ScriptElem curElem = CMD;

    std::string cmd;
    // we look for sequence points and decide what to do ...
    while(pos < script.size() && pos != std::string::npos)
    {
        const char c = script[pos];
        switch (c)
        {
        case ';':
        {
            cmd = script.substr(start, pos-start);
            start = pos+1;
        }
            break;

        case 'f':
        {
            if (    script.find("for(", pos) == pos
                    ||  script.find("for ", pos) == pos
                    )
            {
                next = script.find('(', pos+3);
                if (next != std::string::npos)
                {
                    // store the parser idx starting this for loop
                    forLoop.push(m_vecParsers.at(0).size());

                    curElem = FOR_ADMIN;
                    bracketOpen.push(next);
                    pos = next;
                    start = pos+1;
                }
                else
                {
                    /// ToDo: throw exception
                    std::stringstream exsstr;
                    exsstr << "Malformed for-loop near pos "
                           << pos << ". Missing '('.";
                    KernelScriptParserError pe;
                    pe.SetDescription(exsstr.str());
                    pe.SetLocation(ITK_LOCATION);
                    throw pe;
                }
            }
        }
            break;

        case '(':
        {
            if (curElem == FOR_ADMIN)
            {
                bracketOpen.push(pos);
            }
        }
            break;

        case ')':
        {
            if (curElem == FOR_ADMIN)
            {
                bracketOpen.pop();
                if (bracketOpen.size() == 0)
                {
                    cmd = script.substr(start, pos-start);

                    next = script.find('{', pos+1);
                    if (next != std::string::npos)
                    {
                        start = next+1;
                        curElem = FOR_BLOCK;
                    }
                    else
                    {
                        /// ToDo: throw exception
                        std::stringstream exsstr;
                        exsstr << "Malformed for-loop near pos "
                               << pos << "!";
                        KernelScriptParserError pe;
                        pe.SetDescription(exsstr.str());
                        pe.SetLocation(ITK_LOCATION);
                        throw pe;
                    }
                }
            }
        }
            break;

        case '}':
        {
            if (curElem == FOR_BLOCK)
            {
                if (forLoop.empty())
                {
                    /// ToDo: throw exception
                    std::stringstream exsstr;
                    exsstr << "Parsing error! For loop without head near pos "
                           << pos << "!";
                    KernelScriptParserError pe;
                    pe.SetDescription(exsstr.str());
                    pe.SetLocation(ITK_LOCATION);
                    throw pe;
                }

                int idx = forLoop.top();
                forLoop.pop();
                m_vecBlockLen.at(idx) = m_vecParsers[0].size() - idx;

                if (forLoop.empty())
                {
                    curElem = CMD;
                }
                start = pos+1;
            }
        }
            break;
        }


        if (!cmd.empty())
        {
            this->ParseCommand(cmd);
            m_vecBlockLen.push_back(1);
            cmd.clear();
        }

        ++pos;
    }
}


template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::CacheInputData()
{
    for (int n=0; n < m_DataNames.size(); ++n)
    {
        const std::string& name = m_DataNames.at(n);

        if (n < this->GetNumberOfIndexedInputs())
        {
            itk::DataObject* dataObject = this->GetIndexedInputs().at(n).GetPointer();
            AttributeTable* tab = dynamic_cast<otb::AttributeTable*>(dataObject);
            InputImageType* img = dynamic_cast<TInputImage*>(dataObject);

            const ParserValue nv = static_cast<ParserValue>(m_Nodata);
            long underflows = 0;
            long overflows = 0;
            // if we've got a table here, we haven't dealt with, we
            // just copy the content into a matrix type mup::Value
            if (tab != 0 && m_mapNameTable[m_This].find(name) == m_mapNameTable[m_This].end())
            {
                // the row, maxrow thing below is weired, I know, but it's because otb::SQLiteTable's row index
                // (i.e. primary key) is not necessarily in the range [0, nrows-1]; however
                // we do assume that it is stored in 1-increments; if not, we get (nodata) values

                int minrow = tab->GetMinPKValue();
                int maxrow = tab->GetMaxPKValue();
                int nrows = maxrow - minrow + 1;

                int ncols = tab->GetNumCols();

                // note: access is rows, columns
                std::vector<std::vector<ParserValue> > tableCache(ncols);

                for (int col = 0; col < ncols; ++col)
                {
                    std::vector<ParserValue> colCache(nrows);
                    for (int row = minrow, rowidx=0; row <= maxrow; ++row, ++rowidx)
                    {
                        switch(tab->GetColumnType(col))
                        {
                        case AttributeTable::ATTYPE_INT:
                        {
                            const double lv = static_cast<ParserValue>(tab->GetIntValue(col, row));
                            if (lv < (itk::NumericTraits<ParserValue>::NonpositiveMin()))
                            {
                                ++underflows;
                            }
                            else if (lv > (std::numeric_limits<ParserValue>::max()))
                            {
                                //long long lmax = static_cast<long long>(std::numeric_limits<ParserValue>::max());
                                //if (lv > lmax)
                                    ++overflows;
                                //else
                            }
                            else
                            {
                                colCache[rowidx] = static_cast<ParserValue>(lv);
                            }
                        }
                            break;
                        case AttributeTable::ATTYPE_DOUBLE:
                        {
                            const double dv = tab->GetDblValue(col, row);
                            if (dv < static_cast<double>(itk::NumericTraits<ParserValue>::NonpositiveMin()))
                            {
                                ++underflows;
                            }
                            else if (dv > static_cast<double>(std::numeric_limits<ParserValue>::max()))
                            {
                                ++overflows;
                            }
                            else
                            {
                                colCache[rowidx] = static_cast<ParserValue>(dv);
                            }
                        }
                            break;
                        case AttributeTable::ATTYPE_STRING:
                            colCache[rowidx] = nv;
                            break;
                        }
                    }
                    tableCache[col] = colCache;
                }

                // report conversion errors
                if (overflows > 0 || underflows > 0)
                {
                    std::stringstream sstr;
                    sstr << "Data conversion errors: " << overflows
                         << " overflows and " << underflows << " underflows; table "
                         << name
                         << "'s data values are outside the parser's value range!";

                    KernelScriptParserError oe;
                    oe.SetLocation(ITK_LOCATION);
                    oe.SetDescription(sstr.str());
                    throw oe;
                }

                m_mapNameTable[m_This][name] = tableCache;
            }

            // for images, we just prepare the map and put some placeholders in
            // we'll redefine once we know the actual values
            else if (img != 0)
            {
                if (m_mapNameImgValue[m_This].at(0).find(name) == m_mapNameImgValue[m_This].at(0).end())
                {
                    for (int th=0; th < this->GetNumberOfThreads(); ++th)
                    {
                        int nkernpix = m_NumNeighbourPixel > 0 ? m_NumNeighbourPixel : 1;
                        std::vector<ParserValue> mtype(nkernpix, nv);
                        m_mapNameImgValue[m_This].at(th)[name] = mtype;
                    }
                    m_mapNameImg.insert(std::pair<std::string, InputImageType*>(name, img));
                }
                else
                {
                    std::stringstream sstr;
                    sstr << "Image name conflict error: The name '"
                         << name << "' has already been defined!";
                    KernelScriptParserError kspe;
                    kspe.SetLocation(ITK_LOCATION);
                    kspe.SetDescription(sstr.str());
                    throw kspe;
                }
            }
        }
    }
}

template <class TInputImage, class TOutputImage>
void 
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::GenerateInputRequestedRegion() throw (itk::InvalidRequestedRegionError)
{
    // call the superclass' implementation of this method
    Superclass::GenerateInputRequestedRegion();

    // determine kernel size
    m_NumNeighbourPixel = 1;
    for (int d=0; d < m_Radius.GetSizeDimension(); ++d)
    {
        if (m_Radius[d] > 0)
        {
            m_NumNeighbourPixel *= (m_Radius[d] * 2 + 1);
        }
    }
    m_NumNeighbourPixel = m_NumNeighbourPixel == 1 ? 0 : m_NumNeighbourPixel;

    m_NumPixels = -1;
    InputImageType* inputPtr = 0;

    int cnt = 0;
    while (m_NumPixels < 0 && cnt < this->GetNumberOfIndexedInputs())
    {
        inputPtr = dynamic_cast<InputImageType*>(
                        this->GetIndexedInputs().at(cnt).GetPointer());
        if (inputPtr != 0)
        {
            this->m_NumPixels = inputPtr->GetLargestPossibleRegion().GetNumberOfPixels();
        }
        ++cnt;
    }

    // no need to pad the input requested region,
    // if we're not operating on a kernel
    if (!m_NumNeighbourPixel)
    {
        return;
    }

    for (int ip=0; ip < this->GetNumberOfIndexedInputs(); ++ip)
    {
        inputPtr = dynamic_cast<InputImageType*>(
                    this->GetIndexedInputs().at(ip).GetPointer());

        if (inputPtr == 0)
        {
            continue;
        }

        // get a copy of the input requested region (should equal the output
        // requested region)
        typename TInputImage::RegionType inputRequestedRegion;
        inputRequestedRegion = inputPtr->GetRequestedRegion();

        // pad the input requested region by the operator radius
        inputRequestedRegion.PadByRadius( m_Radius );

        // crop the input requested region at the input's largest possible region
        if ( inputRequestedRegion.Crop(inputPtr->GetLargestPossibleRegion()) )
        {
            inputPtr->SetRequestedRegion( inputRequestedRegion );
        }
        else
        {
            // Couldn't crop the region (requested region is outside the largest
            // possible region).  Throw an exception.

            // store what we tried to request (prior to trying to crop)
            inputPtr->SetRequestedRegion( inputRequestedRegion );

            // build an exception
            itk::InvalidRequestedRegionError e(__FILE__, __LINE__);
            e.SetLocation(ITK_LOCATION);
            e.SetDescription("Requested region is (at least partially) outside the largest possible region.");
            e.SetDataObject(inputPtr);
            throw e;
        }
    }
}


template< class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter2< TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                       itk::ThreadIdType threadId)
{
//    CALLGRIND_START_INSTRUMENTATION;

    // allocate the output image
    typename OutputImageType::Pointer output = this->GetOutput();

    // support progress methods/callbacks
    itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());

    if (m_NumNeighbourPixel)
    {
        // set up the neighborhood iteration, e.g. create a list of boundary faces
        itk::ZeroFluxNeumannBoundaryCondition<InputImageType> nbc;
        typename std::map<std::string, InputImageType*>::const_iterator inImgIt = m_mapNameImg.begin();
        typename itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType>::FaceListType faceList;
        itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType> bC;
        faceList = bC(inImgIt->second, outputRegionForThread, m_Radius);
        typename itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType>::FaceListType::iterator fit;


        // initialise the neighborhood iterators and the corresponding
        // paraser arrays
        //std::vector<InputShapedIterator> vInputIt(m_mapNameImg.size());
        std::vector<InputNeighborhoodIterator> vInputIt(m_mapNameImg.size());
        OutputRegionIterator outIt;

        // Process each of the boundary faces.  These are N-d regions which border
        // the edge of the buffer.
        for (fit=faceList.begin(); fit != faceList.end(); ++fit)
        {
            // create an iterator for each input image and keep it
            inImgIt = m_mapNameImg.begin();
            int cnt=0;
            while (inImgIt != m_mapNameImg.end())
            {
                // we set all indices to active per default
                //vInputIt[cnt] = InputShapedIterator(m_Radius, inImgIt->second, *fit);
                vInputIt[cnt] = InputNeighborhoodIterator(m_Radius, inImgIt->second, *fit);
                vInputIt[cnt].OverrideBoundaryCondition(&nbc);
                vInputIt[cnt].GoToBegin();
                //                while (!vInputIt[cnt].IsAtEnd())
                //                {
                //                    vInputIt[cnt].ActivateIndex(vInputIt[cnt].GetIndex());
                //                    ++vInputIt[cnt];
                //                }
                //                vInputIt[cnt].GoToBegin();

                ++cnt;
                ++inImgIt;
            }

            //unsigned int neighborhoodSize = vInputIt[0].Size();
            outIt = OutputRegionIterator(output, *fit);
            outIt.GoToBegin();

            while (!outIt.IsAtEnd() && !this->GetAbortGenerateData())
            {
                // set the neigbourhood values of all input images
                inImgIt = m_mapNameImg.begin();
                cnt=0;
                while (inImgIt != m_mapNameImg.end())
                {

                    for (int ncnt=0; ncnt < m_NumNeighbourPixel; ++ncnt)
                    {
                        bool bDataTypeRangeError = false;
                        const InputPixelType pv = vInputIt[cnt].GetPixel(ncnt);
                        if (pv < itk::NumericTraits<ParserValue>::NonpositiveMin())
                        {
                            bDataTypeRangeError = true;
                        }
                        else if (pv > itk::NumericTraits<ParserValue>::max())
                        {
                            bDataTypeRangeError = true;
                        }

                        if (!bDataTypeRangeError)
                        {
                            m_mapNameImgValue[m_This][threadId][inImgIt->first][ncnt] = static_cast<ParserValue>(pv);
                        }
                        else
                        {
                            std::stringstream sstr;
                            sstr << "Data type range error: Image " << inImgIt->first
                                 << "'s value is out of the parser's data type range!";
                            KernelScriptParserError dre;
                            dre.SetLocation(ITK_LOCATION);
                            dre.SetDescription(sstr.str());
                            throw dre;
                        }
                    }
                    ++cnt;
                    ++inImgIt;
                }


                // let's run the script now
                //try
                {
                    for (int p=0; p < m_vecParsers[threadId].size(); ++p)
                    {
                        const ParserPointerType& exprParser = m_vecParsers[threadId][p];
                        ParserValue& exprVal = m_mapNameAuxValue[threadId][m_mapParserName[exprParser.GetPointer()]];
                        exprVal = exprParser->Eval();

                        if (m_vecBlockLen[p] > 1)
                        {
                            Loop(p, threadId);
                            p += m_vecBlockLen[p]-1;
                        }
                    }
                }
                //                catch (mu::ParserError& evalerr)
                //                {
                //                    KernelScriptParserError kse;
                //                    kse.SetDescription(evalerr.GetMsg());
                //                    kse.SetLocation(ITK_LOCATION);
                //                    throw kse;
                //                }

                // now we set the result value for the
                //const ParserValue outValue = m_vOutputValue[threadId];
                const ParserValue outValue = m_mapNameAuxValue[threadId][m_OutputVarName];
                if (outValue < itk::NumericTraits<OutputPixelType>::NonpositiveMin())
                {
                    ++m_NumUnderflows[threadId];
                }
                else if (outValue > itk::NumericTraits<OutputPixelType>::max())
                {
                    ++m_NumOverflows[threadId];
                }
                else
                {
                    outIt.Set(static_cast<OutputPixelType>(outValue));
                }

                // prepare everything for the next pixel
                for (int si=0; si < vInputIt.size(); ++si)
                {
                    ++vInputIt[si];
                }
                ++outIt;
                ++m_vthPixelCounter[threadId];
                progress.CompletedPixel();
            }
        }
    }
    // we've got no kernel
    else
    {
        std::vector<InputRegionIterator> vInputIt(m_mapNameImg.size());
        OutputRegionIterator outIt;

        const ParserValue noval = static_cast<ParserValue>(m_Nodata);
        // itk::NumericTraits<ParserValue>::NonpositiveMin();

        // create an iterator for each input image and keep it

        typename std::map<std::string, InputImageType*>::const_iterator inImgIt = m_mapNameImg.begin();
        int cnt=0;
        while (inImgIt != m_mapNameImg.end())
        {
            // we set all indices to active per default
            vInputIt[cnt] = InputRegionIterator(inImgIt->second, outputRegionForThread);
            ++cnt;
            ++inImgIt;
        }
        outIt = OutputRegionIterator(output, outputRegionForThread);

        while (!outIt.IsAtEnd() && !this->GetAbortGenerateData())
        {
            // set the neigbourhood values of all input images
            inImgIt = m_mapNameImg.begin();
            cnt=0;
            while (inImgIt != m_mapNameImg.end())
            {
                bool bDataTypeRangeError = false;
                const InputPixelType pv = vInputIt[cnt].Get();
                if (pv < itk::NumericTraits<ParserValue>::NonpositiveMin())
                {
                    bDataTypeRangeError = true;
                }
                else if (pv > itk::NumericTraits<ParserValue>::max())
                {
                    bDataTypeRangeError = true;
                }

                if (!bDataTypeRangeError)
                {
                    m_mapNameImgValue[m_This][threadId][inImgIt->first][0] = static_cast<ParserValue>(pv);
                }
                else
                {
                    std::stringstream sstr;
                    sstr << "Data type range error: Image " << inImgIt->first
                         << "'s value is out of the parser's data type range!";
                    KernelScriptParserError dre;
                    dre.SetLocation(ITK_LOCATION);
                    dre.SetDescription(sstr.str());
                    throw dre;
                }
                ++cnt;
                ++inImgIt;
            }

            // let's run the script now
            //try
            {
                for (int p=0; p < m_vecParsers[threadId].size(); ++p)
                {
                    const ParserPointerType& exprParser = m_vecParsers[threadId][p];
                    ParserValue& exprVal = m_mapNameAuxValue[threadId][m_mapParserName[exprParser.GetPointer()]];
                    exprVal = exprParser->Eval();

                    if (m_vecBlockLen[p] > 1)
                    {
                        Loop(p, threadId);
                        p += m_vecBlockLen[p]-1;
                    }
                }
            }
            //            catch (mu::ParserError& evalerr)
            //            {
            //                KernelScriptParserError kse;
            //                kse.SetDescription(evalerr.GetMsg());
            //                kse.SetLocation(ITK_LOCATION);
            //                throw kse;
            //            }

            // now we set the result value for the
            //const ParserValue outValue = m_vOutputValue[threadId];
            const ParserValue outValue = m_mapNameAuxValue[threadId][m_OutputVarName];
            if (outValue < itk::NumericTraits<OutputPixelType>::NonpositiveMin())
            {
                ++m_NumUnderflows[threadId];
                outIt.Set(m_Nodata);
            }
            else if (outValue > itk::NumericTraits<OutputPixelType>::max())
            {
                ++m_NumOverflows[threadId];
                outIt.Set(m_Nodata);
            }
            else
            {
                outIt.Set(static_cast<OutputPixelType>(outValue));
            }

            // prepare everything for the next pixel
            for (int si=0; si < vInputIt.size(); ++si)
            {
                ++vInputIt[si];
            }
            ++outIt;
            ++m_vthPixelCounter[threadId];
            progress.CompletedPixel();
        }
    }

//    CALLGRIND_STOP_INSTRUMENTATION;
//    CALLGRIND_DUMP_STATS;
}


template< class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter2< TInputImage, TOutputImage>
::AfterThreadedGenerateData()
{
    long long nover = 0;
    long long nunder = 0;
    for (int th=0; th < this->GetNumberOfThreads(); ++th)
    {
        nover += m_NumOverflows.at(th);
        nunder += m_NumUnderflows.at(th);
        m_PixelCounter += m_vthPixelCounter.at(th);
    }

    if (nover || nunder)
    {
        itkWarningMacro(<< nover << " overflows and "
                        << nunder << " underflows detected! "
                        << "Double check your results!");
    }

    if (m_PixelCounter >= m_NumPixels)
    {
        Reset();
    }
}

/**
 * Standard "PrintSelf" method
 */
template <class TInputImage, class TOutputImage>
void
NMScriptableKernelFilter2<TInputImage, TOutputImage>
::PrintSelf(
        std::ostream& os,
        itk::Indent indent) const
{
    Superclass::PrintSelf( os, indent );
    int nimgs = m_mapNameImg.size();
    os << indent << "Radius:    " << m_Radius << std::endl;
    os << indent << "KernelShape: " << m_KernelShape << std::endl;
    os << indent << "No. Parser: " << m_mapParserName.size()  - nimgs << std::endl;
    os << indent << "Images: ";

    typename std::map<std::string, InputImageType*>::const_iterator imgIt =
            m_mapNameImg.begin();
    while (imgIt != m_mapNameImg.end())
    {
        os << imgIt->first << " ";
        ++imgIt;
    }
    os << std::endl;

	if (m_mapNameAuxValue.size() > 0)
	{
		os << indent << "Tables: ";
        std::map<std::string, ParserValue>::const_iterator auxIt =
				m_mapNameAuxValue.at(0).begin();

		while (auxIt != m_mapNameAuxValue.at(0).end())
		{
			os << auxIt->first << " ";
			++auxIt;
		}
		os << std::endl;
	}

}

} // end namespace otb

//#endif


#endif
