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
 * otbSQLiteProcessor.h
 *
 *  Created on: 08/11/2015
 *      Author: alex
 *
 */

#ifndef OTBSQLITEPROCESSOR_H_
#define OTBSQLITEPROCESSOR_H_

#include <string>

#include "itkImageToImageFilter.h"
#include "otbImage.h"
#include "otbSQLiteTable.h"

#include "otbsupplfilters_export.h"

/*!
 *  \brief SQLiteProcessor enables SQL processing with otb::SQLiteTable's
 *
 */

namespace otb
{

template< class TInputImage, class TOutputImage = TInputImage >
class OTBSUPPLFILTERS_EXPORT SQLiteProcessor
        : public itk::ImageToImageFilter< TInputImage, TOutputImage >
{
public:

    /** Convenient typedefs for simplifying declarations. */


    typedef TInputImage  InputImageType;
    typedef TOutputImage OutputImageType;

    typedef SQLiteProcessor                 Self;
    typedef itk::ImageToImageFilter< InputImageType, OutputImageType> Superclass;
    typedef itk::SmartPointer<Self>         Pointer;
    typedef itk::SmartPointer<const Self>   ConstPointer;

    itkNewMacro(Self)
    itkTypeMacro(SQLiteProcessor, itk::ImageToImageFilter)


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


    itkSetMacro(SQLStatement, std::string)

    void SetImageNames(std::vector<std::string> names) {m_ImageNames = names;}

    AttributeTable::Pointer getRAT(unsigned int idx);
    void setRAT(unsigned int idx, AttributeTable::Pointer);

    virtual void ResetPipeline();


protected:
    SQLiteProcessor();
    SQLiteProcessor(const Self&);
    void operator=(const Self&);

    void GenerateData();

    std::vector<std::string>  m_ImageNames;
    std::string m_SQLStatement;

    std::vector<SQLiteTable::Pointer> m_vRAT;
    long long m_ProcessedPixel;

private:
    static const std::string ctx;

};

}   // END NAMESPACE OTB

template< class TInputImage, class TOutputImage>
const std::string otb::SQLiteProcessor<TInputImage, TOutputImage>::ctx = "otb::SQLiteProcessor";

#include "otbSQLiteProcessor_ExplicitInst.h"

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbSQLiteProcessor.txx"
#endif

#endif // HEADER DEFINITION

