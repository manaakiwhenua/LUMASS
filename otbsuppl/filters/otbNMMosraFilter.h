/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2015 Landcare Research New Zealand Ltd
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

/*
 * otbNMMosraFilter.h
 *
 *  Created on: 03/03/2017
 *      Author: Alex Herzig
 *
 */

#ifndef OTBNMMOSRAFILTER_H
#define OTBNMMOSRAFILTER_H

#include <string>

#include "itkImageToImageFilter.h"
#include "otbImage.h"
#include "otbAttributeTable.h"
#include "otbSQLiteTable.h"
#include "NMMosra.h"

#include "otbsupplfilters_export.h"

/*!
 *  \brief NMMosraFilter performs spatial optimisation NMMosraDataSet
 *         supported tables / data sets (i.e. vtkDataSet & otb::AttributeTable)
 *
 */

namespace otb
{

template< class TInputImage, class TOutputImage = TInputImage >
class OTBSUPPLFILTERS_EXPORT NMMosraFilter
        : public itk::ImageToImageFilter< TInputImage, TOutputImage >
{
public:

    typedef TInputImage  InputImageType;
    typedef TOutputImage OutputImageType;

    typedef NMMosraFilter                 Self;
    typedef itk::ImageToImageFilter< InputImageType, OutputImageType> Superclass;
    typedef itk::SmartPointer<Self>         Pointer;
    typedef itk::SmartPointer<const Self>   ConstPointer;

    itkNewMacro(Self)
    itkTypeMacro(NMMosraFilter, itk::ImageToImageFilter)


    /** Extract dimension from input and output image. */
    itkStaticConstMacro(InputImageDimension, unsigned int,
                        TInputImage::ImageDimension);
    itkStaticConstMacro(OutputImageDimension, unsigned int,
                        TOutputImage::ImageDimension);

    typedef typename InputImageType::Pointer	InputImagePointerType;
    typedef typename OutputImageType::Pointer OutputImagePointerType;

    /** Image typedef support. */
    typedef typename InputImageType::PixelType   InputPixelType;
    typedef typename OutputImageType::PixelType  OutputPixelType;

    typedef typename InputImageType::RegionType  InputImageRegionType;
    typedef typename OutputImageType::RegionType OutputImageRegionType;

    itkSetMacro(CompName, std::string)
    itkSetMacro(Workspace, std::string)
    itkSetMacro(LosFileName, std::string)
    itkSetMacro(ScenarioName, std::string)
    itkSetMacro(LosSettings, std::string)
    itkSetMacro(GenerateReports, bool)
    itkSetMacro(TimeOut, int)

    void SetImageNames(std::vector<std::string> names) {m_ImageNames = names;}

    AttributeTable::Pointer getRAT(unsigned int idx);
    void setRAT(unsigned int idx, AttributeTable::Pointer);


    void GenerateInputRequestedRegion();

 protected:
    NMMosraFilter();
    NMMosraFilter(const Self&);
    void operator=(const Self&);
    ~NMMosraFilter();

    void GenerateData();
    void GenerateReportData(NMMosra* mosra, char* theTime=nullptr);

    std::vector<std::string>  m_ImageNames;

    std::string m_CompName;
    std::string m_Workspace;
    std::string m_LosFileName;
    std::string m_ScenarioName;
    std::string m_LosSettings;
//    std::string m_ReportFileName;
//    std::string m_LpFileName;
//    std::string

    bool m_GenerateReports;
    int m_TimeOut;

    NMMosraDataSet* m_DataSet;

private:
    static const std::string ctx;

};

} // END NAMESPACE OTB

template< class TInputImage, class TOutputImage>
const std::string otb::NMMosraFilter<TInputImage, TOutputImage>::ctx = "otb::NMMosraFilter";

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbNMMosraFilter.txx"
#endif

#endif // OTBNMMOSRAFILTER_H
