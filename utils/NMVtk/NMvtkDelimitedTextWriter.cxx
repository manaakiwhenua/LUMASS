/*=========================================================================

  Program:   ParaView
  Module:    vtkDelimitedTextWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------
 *  Adapted Superclass function for use in LUMASS
 *  by Alex Herzig on 2020-02-11
 *
 *  - subclassed vtkDelimitedTextWriter
 *  - adapted ::WriteData() -- added max precision floating point number
 *    formatting for file output
 *
 *  Copyright 2020 Manaaki Whenua - Landcare Research
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
 *
 */

#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "NMvtkDelimitedTextWriter.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkTable.h"

#include "vtkSmartPointer.h"


#include <vector>
#include <sstream>
#include <limits>

vtkStandardNewMacro(NMvtkDelimitedTextWriter);

//-----------------------------------------------------------------------------
template <class iterT>
void NMvtkDelimitedTextWriterGetDataString(
  iterT* iter, vtkIdType tupleIndex, ostream* stream, NMvtkDelimitedTextWriter* writer,
  bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex* numComps;
  for (int cc=0; cc < numComps; cc++)
  {
    if ((index+cc) < iter->GetNumberOfValues())
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
      (*stream) << iter->GetValue(index+cc);
    }
    else
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
    }
  }
}

//-----------------------------------------------------------------------------
template<>
void NMvtkDelimitedTextWriterGetDataString(
  vtkArrayIteratorTemplate<vtkStdString>* iter, vtkIdType tupleIndex,
  ostream* stream, NMvtkDelimitedTextWriter* writer, bool* first)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex* numComps;
  for (int cc=0; cc < numComps; cc++)
  {
    if ((index+cc) < iter->GetNumberOfValues())
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
      (*stream) << writer->GetString(iter->GetValue(index+cc));
    }
    else
    {
      if (*first == false)
      {
        (*stream) << writer->GetFieldDelimiter();
      }
      *first = false;
    }
  }
}
//-----------------------------------------------------------------------------


void NMvtkDelimitedTextWriter::WriteTable(vtkTable *table)
{
    vtkIdType numRows = table->GetNumberOfRows();
    vtkDataSetAttributes* dsa = table->GetRowData();
    if (!this->OpenStream())
    {
      return;
    }

    // set max precision for output
    (*this->Stream) << std::defaultfloat << std::setprecision(std::numeric_limits<long double>::digits10);

    std::vector<vtkSmartPointer<vtkArrayIterator> > columnsIters;

    int cc;
    int numArrays = dsa->GetNumberOfArrays();
    bool first = true;
    // Write headers:
    for (cc=0; cc < numArrays; cc++)
    {
      vtkAbstractArray* array = dsa->GetAbstractArray(cc);
      for (int comp=0; comp < array->GetNumberOfComponents(); comp++)
      {
        if (!first)
        {
          (*this->Stream) << this->FieldDelimiter;
        }
        first = false;

        std::ostringstream array_name;
        array_name << array->GetName();
        if (array->GetNumberOfComponents() > 1)
        {
          array_name << ":" << comp;
        }
        (*this->Stream) << this->GetString(array_name.str());
      }
      vtkArrayIterator* iter = array->NewIterator();
      columnsIters.push_back(iter);
      iter->Delete();
    }
    (*this->Stream) << "\n";

    for (vtkIdType index=0; index < numRows; index++)
    {
      first = true;
      std::vector<vtkSmartPointer<vtkArrayIterator> >::iterator iter;
      for (iter = columnsIters.begin(); iter != columnsIters.end(); ++iter)
      {
        switch ((*iter)->GetDataType())
        {
          vtkArrayIteratorTemplateMacro(
            NMvtkDelimitedTextWriterGetDataString(static_cast<VTK_TT*>(iter->GetPointer()),
              index, this->Stream, this, &first));
          case VTK_VARIANT:
          {
            NMvtkDelimitedTextWriterGetDataString(static_cast<vtkArrayIteratorTemplate<vtkVariant>*>(iter->GetPointer()),
              index, this->Stream, this, &first);
            break;
          }
        }
      }
      (*this->Stream) << "\n";
    }

    if (this->WriteToOutputString)
    {
      std::ostringstream *ostr =
        static_cast<std::ostringstream*>(this->Stream);

      delete [] this->OutputString;
      size_t strLen = ostr->str().size();
      this->OutputString = new char[strLen+1];
      memcpy(this->OutputString, ostr->str().c_str(), strLen+1);
    }
    delete this->Stream;
    this->Stream = nullptr;
}

