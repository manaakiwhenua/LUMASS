/******************************************************************************
* Created by Alexander Herzig
* Copyright 2016 Landcare Research New Zealand Ltd
*
* This file is part of 'LUMASS', which is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation, either version 3 of the License,
* or (at your option) any later version.
*
* This programs distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
/*
*  NMTableReader
*
*  Created on: 27/06/2016
*      Author: alex
*/


#ifndef otbNMTableReader_H_
#define otbNMTableReader_H_

#include "itkProcessObject.h"
#include "itkDataObject.h"

#include "nmotbsupplcore_export.h"

namespace otb
{

class NMOTBSUPPLCORE_EXPORT NMTableReader : public itk::ProcessObject
{
public:
    typedef NMTableReader                  Self;
    typedef itk::ProcessObject             Superclass;
    typedef itk::SmartPointer< Self >      Pointer;
    typedef itk::SmartPointer< const Self> ConstPointer;
    //typedef typename Self::DataObjectPointerArraySizeType DataObjectPointerArraySizeType;

    itkNewMacro(Self)
    itkTypeMacro(NMTableReader, Superclass)

    itkGetMacro(FileName, std::string)
    itkSetMacro(FileName, std::string)

    itkGetMacro(TableName, std::string)
    itkSetMacro(TableName, std::string)

    itkGetMacro(RowIdColname, std::string)
    itkSetMacro(RowIdColname, std::string)

    itkGetMacro(CreateTable, bool)
    itkSetMacro(CreateTable, bool)

    DataObjectPointer MakeOutput(DataObjectPointerArraySizeType idx);
    void GenerateData();

    itk::DataObject* GetOutput(unsigned int idx=0);

protected:
    NMTableReader();
    ~NMTableReader();

    bool m_CreateTable;
    std::string m_FileName;
    std::string m_TableName;
    std::string m_RowIdColname;


};

}   // otb namespace

#endif // include guard
