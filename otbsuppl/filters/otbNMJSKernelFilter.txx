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
  Module:    $RCSfile: itkNMJSKernelFilter.txx,v $
  Language:  C++
  Date:      $Date: 2008-10-16 18:05:25 $
  Version:   $Revision: 1.16 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __otbNMJSKernelFilter_txx
#define __otbNMJSKernelFilter_txx


// First make sure that the configuration is available.
// This line can be removed once the optimized versions
// gets integrated into the main directories.
//#include "itkConfigure.h"
//
//#ifdef ITK_USE_CONSOLIDATED_MORPHOLOGY
//#include "itkOptNMJSKernelFilter.txx"
//#else

#include "otbNMJSKernelFilter.h"

#include <cctype>
#include <algorithm>
#include <stack>
#include <regex>

#include "itkConstNeighborhoodIterator.h"
#include "itkNeighborhoodInnerProduct.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodAlgorithm.h"
#include "itkZeroFluxNeumannBoundaryCondition.h"
#include "itkOffset.h"
#include "itkProgressReporter.h"
//#include "utils/muParser/muParserError.h"
#include "otbKernelScriptParserError.h"
//#include "otbRAMTable.h"'
#include "otbSQLiteTable.h"
//#include "otbAttributeTable.h"

#include <QJSValueIterator>

#include "nmlog.h"

//#include "valgrind/callgrind.h"


namespace otb
{

template <class TInputImage, class TOutputImage>
NMJSKernelFilter<TInputImage, TOutputImage>
::NMJSKernelFilter()
{
    m_Radius.Fill(0);

    // <RECTANGULAR> and <CIRCULAR>
    m_KernelShape = "RECTANGULAR";

    m_PixelCounter = 0;

    m_Nodata = itk::NumericTraits<OutputPixelType>::NonpositiveMin();

    this->SetNumberOfIndexedOutputs(2);

    this->SetNthOutput(1, MakeOutput(1));
}

template <class TInputImage, class TOutputImage>
NMJSKernelFilter<TInputImage, TOutputImage>
::~NMJSKernelFilter()
{
    this->Reset();
}

template <class TInputImage, class TOutputImage>
void
NMJSKernelFilter<TInputImage, TOutputImage>
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
NMJSKernelFilter<TInputImage, TOutputImage>
::SetNumThreads(unsigned int nthreads)
{
    unsigned int nth = std::max(nthreads, (unsigned int)1);
    unsigned int maxthreads = this->GetNumberOfThreads();

    this->SetNumberOfThreads(std::min(nth, maxthreads));
}

template <class TInputImage, class TOutputImage>
void
NMJSKernelFilter<TInputImage, TOutputImage>
::Reset()
{
    m_KernelShape = "RECTANGULAR";
    m_ActiveKernelIndices.clear();

    m_ActiveNeighborhoodSize = 1;
    m_CentrePixelIndex = 0;
    m_PixelCounter = 0;
    m_NumPixels = 0;
    m_NumOverflows.clear();
    m_NumUnderflows.clear();

    m_mapNameImg.clear();


    m_vKernelStore.clear();
    m_vKernelInfo.clear();
    m_vScript.clear();
    m_vJSEngine.clear();
    m_mapNameImgKernel.clear();

    m_vNeighbourDistance.clear();

    m_minVal.clear();
    m_maxVal.clear();
    m_sumVal.clear();

    this->Modified();
}


template <class TInputImage, class TOutputImage>
void
NMJSKernelFilter<TInputImage, TOutputImage>
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
NMJSKernelFilter<TInputImage, TOutputImage>
::SetNthInput(itk::DataObject::DataObjectPointerArraySizeType num, itk::DataObject* input)
{
    InputImageType* img = dynamic_cast<InputImageType*>(input);

    if (img)
    {
        int idx = num >= this->GetNumberOfIndexedInputs() ? this->GetNumberOfIndexedInputs(): num;
        Superclass::SetNthInput(idx, input);
        m_IMGNames.push_back(m_DataNames.at(num));
    }
}

template <class TInputImage, class TOutputImage>
void
NMJSKernelFilter<TInputImage, TOutputImage>
::setRAT(unsigned int idx, AttributeTable::Pointer table)
{
    // NOTE: here we rely on the fact that the NMProcess wrapper
    // class links parameters (i.e. DataNames) before it
    // links input data (i.e. images and tables);
    // furthermore, if an image with an associated RAT is
    // provided, we've got only one name associated with it
    // (since the UserID of the reader component is forwarded,
    // and stored in m_DataNames), so we just append 'Tab' to the
    // image name, to distinguish between the two in the script

    if (idx >= m_IMGNames.size())
    {
        if (idx < m_DataNames.size())
        {
            m_RATNames.push_back(m_DataNames.at(idx));
            m_vRAT.push_back(table);
            this->Modified();
        }
    }
    else
    {
        std::string name = m_IMGNames.at(idx);
        name += "Tab";
        m_RATNames.push_back(name);
        m_vRAT.push_back(table);
        this->Modified();
    }
}

template <class TInputImage, class TOutputImage>
void
NMJSKernelFilter<TInputImage, TOutputImage>
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

    // if we've got a shaped neighbourhood iterator,
    // determine the active offsets
    m_ActiveKernelIndices.clear();
    m_ActiveKernelIndices.resize(m_NumNeighbourPixel);
    if (m_NumNeighbourPixel)
    {
        typedef itk::Neighborhood<int, TInputImage::ImageDimension> NeighbourhoodType;
        NeighbourhoodType neigh;
        neigh.SetRadius(m_Radius);

        typename NeighbourhoodType::OffsetType offset;
        int circ = 0;
        for (int p=0; p < m_NumNeighbourPixel; ++p)
        {
            offset = neigh.GetOffset(static_cast<typename NeighbourhoodType::NeighborIndexType>(p));
            double d2 = 0;
            for (int d=0; d < offset.GetOffsetDimension(); ++d)
            {
                d2 += ((offset[d]*m_Spacing[d]) * (offset[d]*m_Spacing[d]));
            }

            // calucate distances to pixel centre (in pixel)
            // and define cicular neighbourhood, if applicable
            const double dist = ::sqrt(d2);

            // note: for circular neighborhood, we create a square
            // neighborhood first, s. ::GenerateInputRequestedRegion(),
            // so that's why we can just test against the radius of the
            // first dimension

            // NOTE: the circular neighbourhood assumes, we've got
            //       square shaped pixels!

            if (m_KernelShape == "CIRCULAR")
            {
                if  (dist <= (m_Radius[0] * m_Spacing[0]))
                {
                    m_ActiveKernelIndices[circ++] = p;
                    m_vNeighbourDistance.push_back(dist);
                }
            }
            else
            {
                m_ActiveKernelIndices[p] = p;
                m_vNeighbourDistance.push_back(dist);
            }
        }

        if (circ > 0)
        {
            m_ActiveKernelIndices.resize(circ);
        }

        m_ActiveNeighborhoodSize = m_ActiveKernelIndices.size();
        m_CentrePixelIndex = (m_ActiveNeighborhoodSize-1) / 2;
    }
    else
    {
        m_vNeighbourDistance.push_back(0.0);
        m_CentrePixelIndex = 0;
        m_ActiveNeighborhoodSize = 1;
    }

    // initiate scripting env
    // only if we've just started working on
    // this image
    if (m_PixelCounter == 0)
    {
        m_mapNameTable.clear();
        m_mapNameImgNeigValues.clear();
        m_mapNameImgValue.clear();
        m_vScript.clear();
        m_vJSEngine.clear();
        m_vKernelStore.clear();
        m_minVal.clear();
        m_maxVal.clear();
        m_sumVal.clear();
        m_mapStatNameIdx.clear();


        // ----------------------------------------------------------------------
        // prepare kernelScript for 'parsing'
        QString kernelScript = m_KernelScript.c_str();
        if (kernelScript.startsWith("\"") && kernelScript.endsWith("\""))
        {
            kernelScript = kernelScript.mid(1, kernelScript.size()-2);
        }

        // ----------------------------------------------------------------------
        // prepare thread specific objects, i.e. js engine instances, scripts, and
        // 'KernelStore' objects as defined in the initialisation script (if applicable)

        // analyse KernelScript for table expressions
        analyseKernelScript();

        const double nv = static_cast<double>(m_Nodata);
        const unsigned int nthreads = this->GetNumberOfThreads();
        for (int th=0; th < nthreads; ++th)
        {
            // create a js engine
            QSharedPointer<QJSEngine> jsengine(new QJSEngine());
            QJSValue kernelStore = this->RunInitScript(jsengine);

            bool bHaveKernelStore = false;
            if (    !kernelStore.isUndefined()
                 && !kernelStore.isNull()
                 && !kernelStore.isError()
               )
            {
                bHaveKernelStore = true;
                QJSValueIterator init(kernelStore);

                int n=-1;
                while (init.hasNext())
                {
                    init.next();

#ifdef LUMASS_DEBUG
                    QString thename = init.name();
                    QString thevalue = init.value().toString();
                    NMProcDebug(<< thename.toStdString() << " = " << thevalue.toStdString());
#endif

                    if (init.value().isNumber())
                    {
                        ++n;

                        // the stats are across threads, so just need to establish those once
                        if (th == 0)
                        {
                            m_mapStatNameIdx[init.name().toStdString()] = n;
                            m_minVal.push_back(itk::NumericTraits<double>::max());
                            m_maxVal.push_back(itk::NumericTraits<double>::NonpositiveMin());
                            m_sumVal.push_back(0.0);
                        }
                    }
                }
            }

            // define the js neighbourhood kernel buffers
            std::map<std::string, QJSValue> mapNameImgKernel;
            for (int id=0; id < m_IMGNames.size(); ++id)
            {
                QJSValue imgKernel;
                if (m_ActiveNeighborhoodSize > 1)
                {
                    imgKernel = jsengine->newArray(m_ActiveNeighborhoodSize);
                }
                else
                {
                    imgKernel = QJSValue(nv);
                }
                jsengine->globalObject().setProperty(m_IMGNames[id].c_str(), imgKernel);
                mapNameImgKernel.insert(std::pair<std::string, QJSValue>(m_IMGNames[id], imgKernel));
            }
            m_mapNameImgKernel.push_back(mapNameImgKernel);

            // ---------------------------------------
            // 'compile' the kernel script

            QJSValue script = jsengine->evaluate(kernelScript);
            if (    script.isUndefined()
                 || script.isNull()
                 || !script.isCallable()
                 || script.isError()
               )
            {
                std::stringstream errmsg;
                errmsg  << "Reference: Main Kernel Script" << std::endl
                        << "Name: " <<    script.property("name").toString().toStdString() << std::endl
                        << "Message: " << script.property("message").toString().toStdString() << std::endl
                        << "Line number: " << script.property("lineNumber").toInt() << std::endl
                        << "Stack: " << script.property("stack").toString().toStdString();
                NMProcErr( << "NMJSKernelFilter: " << std::endl << errmsg.str());

                KernelScriptParserError kse;
                kse.SetDescription(errmsg.str());
                kse.SetLocation(ITK_LOCATION);
                throw kse;
            }


            // ---------------------------------------------------
            // set some global JS objects meant for read-only access
            QJSValue kernelInfo = jsengine->newObject();
            kernelInfo.setProperty("centre_id", QJSValue(m_CentrePixelIndex));
            kernelInfo.setProperty("pixelCount", QJSValue(m_ActiveNeighborhoodSize));

            QJSValue neigDistance = jsengine->newArray(m_vNeighbourDistance.size());
            for (int n=0; n < m_vNeighbourDistance.size(); ++n)
            {
                neigDistance.setProperty(n, QJSValue(m_vNeighbourDistance[n]));
            }
            kernelInfo.setProperty("distanceTo", neigDistance);

            // set centre pixel coordinates to Null
            kernelInfo.setProperty("x_coord", QJSValue::NullValue);
            kernelInfo.setProperty("y_coord", QJSValue::NullValue);
            if (m_Radius.GetSizeDimension() == 3)
            {
                kernelInfo.setProperty("z_coord", QJSValue::NullValue);
            }

            // --------------------------------------------------------------------
            // put everything neatly away for later use ...
            m_vJSEngine.push_back(jsengine);
            m_vScript.push_back(script);
            m_vKernelInfo.push_back(kernelInfo);
            if (bHaveKernelStore)
            {
                m_vKernelStore.push_back(kernelStore);
            }

            std::map<std::string, double > mapnameimgval;
            m_mapNameImgValue.push_back(mapnameimgval);

            std::map<std::string, InputShapedIterator> mapnameneigs;
            m_mapNameImgNeigValues.push_back(mapnameneigs);

            // update input data
            this->CacheInputData(jsengine, th);
        }

        // make a new AuxTable output
        this->SetNthOutput(1, this->MakeOutput(1));
    }

    m_vthPixelCounter.clear();
    m_vthPixelCounter.resize(this->GetNumberOfThreads(), 0);

    m_NumOverflows.clear();
    m_NumOverflows.resize(this->GetNumberOfThreads(), 0);

    m_NumUnderflows.clear();
    m_NumUnderflows.resize(this->GetNumberOfThreads(), 0);
}

template <class TInputImage, class TOutputImage>
QJSValue NMJSKernelFilter<TInputImage, TOutputImage>
::RunInitScript(QSharedPointer<QJSEngine> jsengine)
{
    // processing of the init script and creation
    // of 'KernelStore' object if script has been specified
    QJSValue initObj;
    if (m_InitScript.empty())
    {
        //NMProcDebug(<< "Skip empty init script");
        return initObj;
    }

    QString script = m_InitScript.c_str();
    if (script.startsWith("\"") && script.endsWith("\""))
    {
        script = script.mid(1, script.size()-2);
    }

    QJSValue initScript = jsengine->evaluate(script);
    if (    initScript.isUndefined()
         || initScript.isNull()
         || !initScript.isCallable()
         || initScript.isError()
       )

    {
        std::stringstream errmsg;
        errmsg  << "Reference: Initialisation Script" << std::endl
                << "Name: " <<    initScript.property("name").toString().toStdString() << std::endl
                << "Message: " << initScript.property("message").toString().toStdString() << std::endl
                << "Line number: " << initScript.property("lineNumber").toInt() << std::endl
                << "Stack: " << initScript.property("stack").toString().toStdString();
        NMProcErr( << "NMJSKernelFilter - InitScript:" << std::endl << errmsg.str());

        KernelScriptParserError kse;
        kse.SetDescription(errmsg.str());
        kse.SetLocation(ITK_LOCATION);
        throw kse;
    }

    initObj = initScript.call();
    if (     initObj.isUndefined()
         ||  initObj.isNull()
         ||  initObj.isError()
       )
    {
        std::stringstream errmsg;
        errmsg  << "Name: " <<    initScript.property("name").toString().toStdString() << std::endl
                << "Message: " << initScript.property("message").toString().toStdString() << std::endl
                << "Line number: " << initScript.property("lineNumber").toInt() << std::endl
                << "Stack: " << initScript.property("stack").toString().toStdString();
        NMProcErr( << "NMJSKernelFilter: " << std::endl << errmsg.str());

        KernelScriptParserError kse;
        kse.SetDescription(errmsg.str());
        kse.SetLocation(ITK_LOCATION);
        throw kse;
    }

#ifdef LUMASS_DEBUG
    QJSValueIterator vit(initObj);
    NMProcDebug(<< "Intitialisation Script: 'KernelStore' object properties ... ");
    while (vit.hasNext())
    {
        vit.next();
        NMProcDebug(<< vit.name().toStdString() << ": " << vit.value().toString().toStdString());
    }
#endif

    return initObj;

}

template <class TInputImage, class TOutputImage>
void
NMJSKernelFilter<TInputImage, TOutputImage>
::analyseKernelScript()
{
    this->m_mapTableColumns.clear();

    std::string input = this->m_KernelScript;
    NMProcDebug(<< "Analysing Kernel Script ...");
    try
    {
        // example string expression:
        //       val myVar = img1Tab.columnA[400];
        // and we want to catch the 'img1' identifier and the column name 'columnA' in speparate
        // groups
        std::regex tableExpression("(?:[^a-zA-Z]*)([a-zA-Z_\\-0-9]+Tab).([a-zA-Z_\\-0-9]+)(?:[^a-zA-Z]*)");

        auto teBegin = std::sregex_iterator(input.begin(), input.end(), tableExpression);
        auto teEnd   = std::sregex_iterator();

        for (std::sregex_iterator i = teBegin; i != teEnd; i++)
        {
            std::smatch m = *i;
            std::string mstr = m.str();
            if (m.size() == 3)
            {
                if (m_mapTableColumns.find(m[1].str()) == m_mapTableColumns.cend())
                {
                    std::vector<std::string> cols = {m[2].str()};
                    m_mapTableColumns.insert(std::pair<std::string, std::vector<std::string> >(m[1].str(), cols));
                }
                else
                {
                    m_mapTableColumns[m[1].str()].push_back(m[2].str());
                }

                NMProcDebug(<< m.str() << "-- table: " << m[1].str() << " | column: " << m[2].str());
            }
        }
    }
    catch(std::regex_error& ree)
    {
        itkExceptionMacro(<< "ERROR: " << ree.what());
    }

}

template <class TInputImage, class TOutputImage>
void
NMJSKernelFilter<TInputImage, TOutputImage>
::CacheInputData(QSharedPointer<QJSEngine> jsengine, int threadId)
{
    std::string name;
    for (int t=0; t < m_RATNames.size(); ++t)
    {
        otb::AttributeTable::Pointer tab = m_vRAT.at(t);
        name = m_RATNames.at(t);

        if (m_mapTableColumns.find(name) == m_mapTableColumns.end())
        {
            continue;
        }

        // if we've got a table here, we haven't dealt with, we
        // just copy the content into a vector of vectors of doubles
        //if (tab.IsNotNull() && m_mapNameTable.find(name) == m_mapNameTable.end())

        if (    tab.IsNotNull()
             && !jsengine->globalObject().hasProperty(name.c_str())
           )
        {
            QJSValue jstab = jsengine->newObject();
            jsengine->globalObject().setProperty(name.c_str(), jstab);

            const OutputPixelType nv = m_Nodata;
            long underflows = 0;
            long overflows = 0;

            // the row, maxrow thing below is weired, I know, but it's because otb::SQLiteTable's row index
            // (i.e. primary key) is not necessarily in the range [0, nrows-1]; however
            // we do assume that it is stored in 1-increments; if not, we get (nodata) values

            int minrow = tab->GetMinPKValue();
            int maxrow = tab->GetMaxPKValue();
            int nrows = maxrow - minrow + 1;

            int ncols = tab->GetNumCols();

            otb::SQLiteTable::Pointer sqlTab = dynamic_cast<otb::SQLiteTable*>(tab.GetPointer());
            if (    tab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_SQLITE && sqlTab.IsNotNull()
                 && threadId == 0
               )
            {
                // note: access is rows, columns
                std::vector<std::string>& tabcols = m_mapTableColumns[name];

                std::vector<std::string> intCols;
                std::vector<std::string> doubleCols;
                std::vector<std::string> stringCols;




                for (int col = 0; col < ncols; ++col)
                {
                    std::string colname = tab->GetColumnName(col);
                    bool bfound = false;
                    for (int nn=0; nn < tabcols.size(); ++nn)
                    {
                        if (colname.compare(tabcols.at(nn)) == 0)
                        {
                            bfound = true;
                        }
                    }

                    QJSValue jscolumn = jsengine->newArray(nrows);
                    jstab.setProperty(colname.c_str(), jscolumn);

                    if (bfound)
                    {
                        switch(sqlTab->GetColumnType(col))
                        {
                        case AttributeTable::ATTYPE_INT:
                        {
                            intCols.push_back(colname);
                            std::map<long long, long long> llvalstore;
                            m_intstore.insert(std::pair<int, std::map<long long, long long> >(col, llvalstore));
                        }
                        break;

                        case AttributeTable::ATTYPE_DOUBLE:
                        {
                            doubleCols.push_back(colname);
                            std::map<long long, double> dblvalstore;
                            m_doublestore.insert(std::pair<int, std::map<long long, double> >(col, dblvalstore));
                        }
                        break;

                        case AttributeTable::ATTYPE_STRING:
                        {
                            stringCols.push_back(colname);
                            std::map<long long, std::string> stringvalstore;
                            m_stringstore.insert(std::pair<int, std::map<long long, std::string> >(col, stringvalstore));
                        }
                        break;
                        }
                    }
                }


                if (m_intstore.size() > 0 && !sqlTab->GreedyNumericFetch(intCols, m_intstore))
                {
                    itkExceptionMacro(<< "JSKernelScript: failed caching integer columns");
                }

                if (m_doublestore.size() > 0 && !sqlTab->GreedyNumericFetch(doubleCols, m_doublestore))
                {
                    itkExceptionMacro(<< "JSKernelScript: failed caching double columns");
                }

                if (m_stringstore.size() > 0 && !sqlTab->GreedyStringFetch(stringCols, m_stringstore))
                {
                    itkExceptionMacro(<< "JSKernelScript: failed caching string columns");
                }
            }

            // define js objects
            if (tab->GetTableType() == otb::AttributeTable::ATTABLE_TYPE_SQLITE && sqlTab.IsNotNull())
            {
                for (int col = 0; col < ncols; ++col)
                {
                    switch(tab->GetColumnType(col))
                    {
                        case AttributeTable::ATTYPE_INT:
                        {
                            std::string colname = tab->GetColumnName(col);
                            QJSValue jscolumn = jsengine->newArray(nrows);
                            jstab.setProperty(colname.c_str(), jscolumn);

                            if (m_intstore.find(col) != m_intstore.end())
                            {
                                for (int row = minrow, rowidx=0; row <= maxrow; ++row, ++rowidx)
                                {
                                    jscolumn.setProperty(rowidx, QJSValue(static_cast<int>(m_intstore[col][row])));
                                }
                            }
                        }
                        break;

                        case AttributeTable::ATTYPE_DOUBLE:
                        {
                            std::string colname = tab->GetColumnName(col);
                            QJSValue jscolumn = jsengine->newArray(nrows);
                            jstab.setProperty(colname.c_str(), jscolumn);

                            if (m_doublestore.find(col) != m_doublestore.end())
                            {
                                for (int row = minrow, rowidx=0; row <= maxrow; ++row, ++rowidx)
                                {
                                    jscolumn.setProperty(rowidx, QJSValue(m_doublestore[col][row]));
                                }
                            }
                        }
                        break;

                        case AttributeTable::ATTYPE_STRING:
                        {
                            std::string colname = tab->GetColumnName(col);
                            QJSValue jscolumn = jsengine->newArray(nrows);
                            jstab.setProperty(colname.c_str(), jscolumn);

                            if (m_stringstore.find(col) != m_stringstore.end())
                            {
                                for (int row = minrow, rowidx=0; row <= maxrow; ++row, ++rowidx)
                                {
                                    jscolumn.setProperty(rowidx, QJSValue(m_stringstore[col][row].c_str()));
                                }
                            }
                        }
                        break;
                    }
                }
            }
            else
            {
                for (int col = 0; col < ncols; ++col)
                {
                    std::string colname = tab->GetColumnName(col);
                    QJSValue jscolumn = jsengine->newArray(nrows);
                    jstab.setProperty(colname.c_str(), jscolumn);

                    for (int row = minrow, rowidx=0; row <= maxrow; ++row, ++rowidx)
                    {
                        switch(tab->GetColumnType(col))
                        {
                        case AttributeTable::ATTYPE_INT:
                            jscolumn.setProperty(rowidx, QJSValue(static_cast<int>(tab->GetIntValue(col, row))));
                            break;

                        case AttributeTable::ATTYPE_DOUBLE:
                            jscolumn.setProperty(rowidx, QJSValue(tab->GetDblValue(col, row)));
                            break;

                        case AttributeTable::ATTYPE_STRING:
                            jscolumn.setProperty(rowidx, QJSValue(tab->GetStrValue(col, row).c_str()));
                            break;
                        }
                    }
                }
            }
        }
    }

    // for images, we just prepare the map and put some placeholders in
    // we'll redefine once we know the actual values
    for (int i=0; i < m_IMGNames.size(); ++i)
    {
        InputImageType* img = dynamic_cast<TInputImage*>(this->GetIndexedInputs().at(i).GetPointer());
        if (img == 0)
        {
            continue;
        }
        name = m_IMGNames.at(i);

        // just doing this once for all threads
        if (threadId == 0)
        {
            if (m_mapNameImg.find(name) == m_mapNameImg.end())
            {
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

        if (m_mapNameImgNeigValues.at(threadId).find(name) == m_mapNameImgNeigValues.at(threadId).end())
        {
            //for (int th=0; th < this->GetNumberOfThreads(); ++th)
            {
                m_mapNameImgNeigValues.at(threadId)[name] = InputShapedIterator();
            }
        }

        if (m_mapNameImgValue.at(threadId).find(name) == m_mapNameImgValue.at(threadId).end())
        {
            //for (int th=0; th < this->GetNumberOfThreads(); ++th)
            {
                m_mapNameImgValue.at(threadId)[name] = static_cast<OutputPixelType>(m_Nodata);
            }
        }
    }
}

template <class TInputImage, class TOutputImage>
void
NMJSKernelFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion() throw ()
{
    // call the superclass' implementation of this method
    Superclass::GenerateInputRequestedRegion();

    // some very basic image information
    // spacing, origin, number of pixel
    // note: since the input may be a otb::AttributeTable,
    // we look for the first image we can find

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
            m_Spacing = inputPtr->GetSignedSpacing(); //GetGetSpacing();
            m_Origin = inputPtr->GetOrigin();
        }
        ++cnt;
    }


    // when we're working on a circular neighbourhood,
    // we make sure the fetched neighbourhood is square
    // rather than only rectangular; thereby, we're taking
    // the biggest radius across all dimension to determine
    // the size of the square;
    if (m_KernelShape == "CIRCULAR")
    {
        int maxRadius = 0;
        for (int d=0; d < m_Radius.GetSizeDimension(); ++d)
        {
            maxRadius = m_Radius[d] > maxRadius ? m_Radius[d] : maxRadius;
        }
        for (int d=0; d < m_Radius.GetSizeDimension(); ++d)
        {
            m_Radius[d] = maxRadius;
        }
    }

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
NMJSKernelFilter< TInputImage, TOutputImage>
::ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                       itk::ThreadIdType threadId)
{
//    CALLGRIND_START_INSTRUMENTATION;

    // allocate the output image
    typename OutputImageType::Pointer output = this->GetOutput();

    // support progress methods/callbacks
    itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());

    std::map<std::string, QJSValue>& mapKernel = m_mapNameImgKernel[threadId];
    if (m_NumNeighbourPixel)
    {
        // set up the neighborhood iteration, e.g. create a list of boundary faces
        itk::ZeroFluxNeumannBoundaryCondition<InputImageType> nbc;
        typename std::map<std::string, InputImageType*>::const_iterator inImgIt = m_mapNameImg.begin();
        typename itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType>::FaceListType faceList;
        itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType> bC;
        faceList = bC(inImgIt->second, outputRegionForThread, m_Radius);
        typename itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<InputImageType>::FaceListType::iterator fit;

        std::map<std::string, InputShapedIterator >& mapInputIter = m_mapNameImgNeigValues[threadId];
        OutputRegionIterator outIt;

        // Process each of the boundary faces.  These are N-d regions which border
        // the edge of the buffer.
        for (fit=faceList.begin(); fit != faceList.end(); ++fit)
        {
            // create an iterator for each input image and keep it
            inImgIt = m_mapNameImg.begin();
            int cnt=0;
            while (inImgIt != m_mapNameImg.end() && !this->GetAbortGenerateData())
            {
                // we set all indices to active per default
                mapInputIter[inImgIt->first] = InputShapedIterator(m_Radius, inImgIt->second, *fit);
                mapInputIter[inImgIt->first].OverrideBoundaryCondition(&nbc);
                mapInputIter[inImgIt->first].SetActiveIndexList(m_ActiveKernelIndices);
                mapInputIter[inImgIt->first].GoToBegin();

                ++cnt;
                ++inImgIt;
            }

            outIt = OutputRegionIterator(output, *fit);
            outIt.GoToBegin();

            while (!outIt.IsAtEnd() && !this->GetAbortGenerateData())
            {
                // prepare the JS objects for this pixel
                inImgIt = m_mapNameImg.begin();
                while(inImgIt != m_mapNameImg.end())
                {
                    InputShapedIterator& sit = mapInputIter[inImgIt->first];
                    typename InputShapedIterator::ConstIterator iit;
                    QJSValue& kernel = mapKernel[inImgIt->first];
                    int kid = 0;
                    for (iit = sit.Begin(); iit != sit.End(); iit++)
                    {
                        const double pixval = static_cast<double>(iit.Get());
                        kernel.setProperty(kid, QJSValue(pixval));
                        ++kid;
                    }
                    // we might as well just tick this one over to the next pixel
                    // since we've got those values in the JS engine now
                    ++sit;
                    ++inImgIt;
                }

                // let's run the script now
                QJSValue kernelInfo = m_vKernelInfo[threadId];

                // spatial location of centre pixel
                const double xcoord = static_cast<double>(m_Origin[0])
                        + static_cast<double>(outIt.GetIndex()[0])
                            * static_cast<double>(m_Spacing[0]);

                const double ycoord = static_cast<double>(m_Origin[1])
                        + static_cast<double>(outIt.GetIndex()[1])
                            * static_cast<double>(m_Spacing[1]);

                kernelInfo.setProperty("x_coord", QJSValue(xcoord));
                kernelInfo.setProperty("y_coord", QJSValue(ycoord));

                if (m_Radius.GetSizeDimension() == 3)
                {
                    const double zcoord = static_cast<double>(m_Origin[2])
                            + static_cast<double>(outIt.GetIndex()[2])
                                * static_cast<double>(m_Spacing[2]);
                    kernelInfo.setProperty("z_coord", QJSValue(zcoord));
                }

                QJSValueList args;
                args << kernelInfo;
                if (threadId < m_vKernelStore.size())
                {
                    args << m_vKernelStore[threadId];
                }

                QJSValue& kernelScript = m_vScript[threadId];
                QJSValue script = kernelScript.call(args);
                if (script.isError())
                {
                    std::stringstream errmsg;
                    errmsg  << "Reference: Neighbourhood Kernel Script execution" << std::endl
                            << "Name: " <<    script.property("name").toString().toStdString() << std::endl
                            << "Message: " << script.property("message").toString().toStdString() << std::endl
                            << "Line number: " << script.property("lineNumber").toInt() << std::endl
                            << "Stack: " << script.property("stack").toString().toStdString();
                    NMProcErr( << "NMJSKernelFilter - KernelScript:" << std::endl << errmsg.str());

                    KernelScriptParserError kse;
                    kse.SetDescription(errmsg.str());
                    kse.SetLocation(ITK_LOCATION);
                    throw kse;
                }

                // now we set the result value for the
                const OutputPixelType outValue = static_cast<OutputPixelType>(script.toNumber());
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
            // spatial location of centre pixel
            QJSValue kernelInfo = m_vKernelInfo[threadId];

            // spatial location of centre pixel
            const double xcoord = static_cast<double>(m_Origin[0])
                    + static_cast<double>(outIt.GetIndex()[0])
                        * static_cast<double>(m_Spacing[0]);

            const double ycoord = static_cast<double>(m_Origin[1])
                    + static_cast<double>(outIt.GetIndex()[1])
                        * static_cast<double>(m_Spacing[1]);

            kernelInfo.setProperty("x_coord", QJSValue(xcoord));
            kernelInfo.setProperty("y_coord", QJSValue(ycoord));

            if (m_Radius.GetSizeDimension() == 3)
            {
                const double zcoord = static_cast<double>(m_Origin[2])
                        + static_cast<double>(outIt.GetIndex()[2])
                            * static_cast<double>(m_Spacing[2]);
                kernelInfo.setProperty("z_coord", QJSValue(zcoord));
            }


            // set the neigbourhood values of all input images
            inImgIt = m_mapNameImg.begin();
            cnt=0;
            while (inImgIt != m_mapNameImg.end())
            {
                bool bDataTypeRangeError = false;
                const double pv = vInputIt[cnt].Get();
                if (pv < itk::NumericTraits<InputPixelType>::NonpositiveMin())
                {
                    bDataTypeRangeError = true;
                }
                else if (pv > itk::NumericTraits<InputPixelType>::max())
                {
                    bDataTypeRangeError = true;
                }

                if (!bDataTypeRangeError)
                {
                    m_vJSEngine[threadId]->globalObject().setProperty(inImgIt->first.c_str(), QJSValue(static_cast<double>(pv)));
                }
                else
                {
                    std::stringstream sstr;
                    sstr << "Data type range error: Image " << inImgIt->first
                         << "'s value is out of the parser's data type range!" << std::endl;
                    NMProcErr(<< "MapKernelScript2: "  << sstr.str())
                    KernelScriptParserError dre;
                    dre.SetLocation(ITK_LOCATION);
                    dre.SetDescription(sstr.str());
                    throw dre;
                }

                ++cnt;
                ++inImgIt;
            }

            // let's run the script now
            QJSValueList args;
            args << kernelInfo;
            if (threadId < m_vKernelStore.size())
            {
                args << m_vKernelStore[threadId];
            }
            QJSValue scriptObj = m_vScript[threadId];//.call(args);
            if (!scriptObj.isCallable() || scriptObj.isError())
            {
                std::stringstream errmsg;
                errmsg  << "Reference: Pixel-based Kernel Script execution" << std::endl
                        << "Name: " <<    scriptObj.property("name").toString().toStdString() << std::endl
                        << "Message: " << scriptObj.property("message").toString().toStdString() << std::endl
                        << "Line number: " << scriptObj.property("lineNumber").toInt() << std::endl
                        << "Stack: " << scriptObj.property("stack").toString().toStdString();
                NMProcErr( << "NMJSKernelFilter: " << std::endl << errmsg.str());

                KernelScriptParserError kse;
                kse.SetDescription(errmsg.str());
                kse.SetLocation(ITK_LOCATION);
                throw kse;
            }

            QJSValue scriptRes = scriptObj.call(args);
            if (scriptRes.isError())
            {
                std::string emsg = scriptRes.property("message").toString().toStdString();
                NMProcErr(<< "NMJSKernelFilter: " << emsg);
                KernelScriptParserError akse;
                akse.SetDescription(emsg.c_str());
                akse.SetLocation(ITK_LOCATION);
                throw akse;
            }

            // now we set the result value for the
            const OutputPixelType outValue = static_cast<OutputPixelType>(scriptRes.toNumber());
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
itk::DataObject::Pointer
NMJSKernelFilter< TInputImage, TOutputImage>
::MakeOutput(unsigned int idx)
{
    switch(idx)
    {
    case 1:
        {
            m_AuxTable = otb::SQLiteTable::New();
            m_AuxTable->SetUseSharedCache(false);
            std::string tabName = m_WorkspacePath + "/aux_" + m_AuxTable->GetRandomString(5) + ".ldb";
            m_AuxTable->CreateTable(tabName);
            return m_AuxTable.GetPointer();
        }
        break;
    default:
        return Superclass::MakeOutput(idx);
        break;
    }
}

template< class TInputImage, class TOutputImage>
void
NMJSKernelFilter< TInputImage, TOutputImage>
::AfterThreadedGenerateData()
{
    // reporting over and under flows during
    // kernel script execution
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
        NMProcWarn(<< nover << " overflows and "
                        << nunder << " underflows detected! "
                        << "Double check your results!");
    }

    bool bSumUp = m_PixelCounter >= m_NumPixels ? true : false;

    // nothing left to do if we're not producing a
    // aux value summary
    if (m_vKernelStore.size() == 0)
    {
        if (bSumUp)
        {
            Reset();
        }
        return;
    }

    // just summing up the aux values across threads
    // i.e. 0: min
    //      1: max
    //      2: mean
    //      3: sum

    int nthreads = this->GetNumberOfThreads();

    // create output table with variable values for further
    // processing
    otb::SQLiteTable::Pointer auxTab;// = otb::SQLiteTable::New();
    auxTab = static_cast<otb::SQLiteTable*>(this->GetAuxOutput().GetPointer());
    bool bTableMagic = false;

    if (bSumUp)
    {
        //if (auxTab->CreateTable("") == otb::SQLiteTable::ATCREATE_CREATED)
        if (auxTab.IsNotNull())
        {
            if (auxTab->GetDbConnection() == nullptr)
            {
                if (!auxTab->openConnection())
                {
                    NMProcErr(<< "failed to open connection to AUX table!");
                }
            }

            bTableMagic = true;
            auxTab->BeginTransaction();
        }
        else
        {
            NMProcErr(<< "failed creating AUX table!");
        }
    }

    std::vector< std::string > colnames;
    std::vector< otb::AttributeTable::TableColumnType > coltypes;
    std::vector< otb::AttributeTable::ColumnValue > colvalues;

    // extract aux values from JS Engine

    for (int t=0; t < nthreads; ++t)
    {
        QJSValueIterator auxIt(m_vKernelStore[t]);
        while (auxIt.hasNext())
        {
            auxIt.next();
            std::string name = auxIt.name().toStdString();
            QJSValue auxVal = auxIt.value();

            // only track numerical values and those that were defined
            // as part of the init script - any additional ones added
            // in the main script are not accounted for
            if (    auxVal.isNumber()
                    && m_mapStatNameIdx.find(name) != m_mapStatNameIdx.end()
                    )
            {
                if (t==0 && bSumUp && bTableMagic && auxTab->ColumnExists(name) < 0)
                {
                    auxTab->AddColumn(name, otb::AttributeTable::ATTYPE_DOUBLE);
                    colnames.push_back(name);
                    coltypes.push_back(otb::AttributeTable::ATTYPE_DOUBLE);
                    otb::AttributeTable::ColumnValue cval;
                    cval.type = otb::AttributeTable::ATTYPE_DOUBLE;
                    colvalues.push_back(cval);
                }
                m_minVal[m_mapStatNameIdx[name]] = std::min(m_minVal[m_mapStatNameIdx[name]], auxVal.toNumber());
                m_maxVal[m_mapStatNameIdx[name]] = std::max(m_maxVal[m_mapStatNameIdx[name]], auxVal.toNumber());
                m_sumVal[m_mapStatNameIdx[name]] += auxVal.toNumber();
            }
        }
    }

    if (bTableMagic)
    {
        auxTab->EndTransaction();

        auxTab->PrepareBulkSet(colnames, true);
        for (int n=0; n < m_minVal.size(); ++n)
        {
            colvalues[n].dval = m_minVal[n];
        }
        auxTab->DoBulkSet(colvalues);

        for (int n=0; n < m_maxVal.size(); ++n)
        {
            colvalues[n].dval = m_maxVal[n];
        }
        auxTab->DoBulkSet(colvalues);

        for (int n=0; n < m_sumVal.size(); ++n)
        {
            colvalues[n].dval = m_sumVal[n] / (double)m_NumPixels;
        }
        auxTab->DoBulkSet(colvalues);

        for (int n=0; n < m_sumVal.size(); ++n)
        {
            colvalues[n].dval = m_sumVal[n];
        }
        auxTab->DoBulkSet(colvalues);
    }

    if (bSumUp)
    {
        Reset();
    }
}

/**
 * Standard "PrintSelf" method
 */
template <class TInputImage, class TOutputImage>
void
NMJSKernelFilter<TInputImage, TOutputImage>
::PrintSelf(
        std::ostream& os,
        itk::Indent indent) const
{
    Superclass::PrintSelf( os, indent );
    int nimgs = m_mapNameImg.size();
    os << indent << "Radius:    " << m_Radius << std::endl;
    os << indent << "KernelShape: " << m_KernelShape << std::endl;
    //os << indent << "No. Parser: " << m_mapParserName.size()  - nimgs << std::endl;
    os << indent << "Images: ";

    typename std::map<std::string, InputImageType*>::const_iterator imgIt =
            m_mapNameImg.begin();
    while (imgIt != m_mapNameImg.end())
    {
        os << imgIt->first << " ";
        ++imgIt;
    }
    os << std::endl;

}

} // end namespace otb

//#endif


#endif
