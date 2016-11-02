/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLookupTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/******************************************************************************
* Adapted/Extended by Alexander Herzig
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

#include "NMVtkLookupTable.h"
#include "nmlog.h"

#include "vtkObjectFactory.h"
#include "vtkBitArray.h"
#include "vtkMathConfigure.h"
#include "vtkMath.h"


//bool _mbUseLowerUpperClr = false;
//double _mLower;
//double _mUpper;
//unsigned char _mUpperClr[4];
//unsigned char _mLowerClr[4];

//----------------------------------------------------------------------------
// Data structure for passing data around various internal functions
struct TableParameters {
  double         MaxIndex;
  double         Range[2];
  double         Shift;
  double         Scale;
  bool           UseIdxMap;
  std::multimap<double, vtkIdType>* pIdxMap;
};


template<class T>
void NMVtkLookupTableMapData(NMVtkLookupTable *self, T *input,
    unsigned char *output, int length,
    int inIncr, int outFormat, TableParameters& p);

template<class T>
void NMVtkLookupTableIndexedMapData(
    NMVtkLookupTable* self, T* input, unsigned char* output, int length,
    int inIncr, int outFormat );

template<class T>
unsigned char *NMVtkLinearLookup(NMVtkLookupTable* self,
  T v, unsigned char *table, double maxIndex, double shift, double scale,
  unsigned char *vtkNotUsed(nanColor));

inline double NMVtkApplyLogScale(double v, const double range[2],
                               const double logRange[2]);

//unsigned char*
//NMVtkLinearLookupMain(double v,
//                unsigned char* table,
//                double maxIndex,
//                double shift, double scale);


vtkStandardNewMacro(NMVtkLookupTable);

NMVtkLookupTable::NMVtkLookupTable(int sze, int ext)
    : vtkLookupTable(sze, ext),
      mLutIdxMapping(false)
{
}

NMVtkLookupTable::~NMVtkLookupTable(void)
{
}

void
NMVtkLookupTable::SetMappedTableValue(vtkIdType indx, double mapVal, double r, double g, double b, double a)
{
    double rgba[4];
    rgba[0] = r; rgba[1] = g; rgba[2] = b; rgba[3] = a;
    this->SetTableValue(indx, rgba);
    this->mLutIdxMap.insert(std::pair<double, vtkIdType>(mapVal, indx));
}


//----------------------------------------------------------------------------
inline void NMVtkLookupShiftAndScale(double range[2],
                                   double maxIndex,
                                   double& shift, double& scale)
{
  shift = -range[0];
  if (range[1] <= range[0])
    {
    scale = VTK_DOUBLE_MAX;
    }
  else
    {
    scale = (maxIndex + 1)/(range[1] - range[0]);
    }
}

static void NMVtkLookupTableLogRange(const double range[2], double logRange[2])
{
  double rmin = range[0];
  double rmax = range[1];

  // does the range include zero?
  if ((rmin <= 0 && rmax >= 0) ||
      (rmin >= 0 && rmax <= 0))
    {
    // clamp the smaller value to 1e-6 times the larger
    if (fabs(rmax) >= fabs(rmin))
      {
      rmin = rmax*1e-6;
      }
    else
      {
      rmax = rmin*1e-6;
      }

    // ensure values are not zero
    if (rmax == 0)
      {
      rmax = (rmin < 0 ? -VTK_DBL_MIN : VTK_DBL_MIN);
      }
    if (rmin == 0)
      {
      rmin = (rmax < 0 ? -VTK_DBL_MIN : VTK_DBL_MIN);
      }
    }

  if (rmax < 0) // rmin and rmax have same sign now
    {
    logRange[0] = -log10(-rmin);
    logRange[1] = -log10(-rmax);
    }
  else
    {
    logRange[0] = log10(rmin);
    logRange[1] = log10(rmax);
    }
}

//----------------------------------------------------------------------------
// Apply shift/scale to the scalar value v and return the index.
inline vtkIdType NMVtkLinearIndexLookupMain(double v, const TableParameters & p)
{
  double dIndex;

  if (p.UseIdxMap)
  {
    std::multimap<double, vtkIdType>::const_iterator it =
            p.pIdxMap->find(v);
    if (it == p.pIdxMap->end())
    {
        dIndex = vtkLookupTable::NAN_COLOR_INDEX;
    }
    else
    {
        dIndex = it->second;
    }
  }
  else if (v < p.Range[0])
    {
    dIndex = p.MaxIndex + vtkLookupTable::BELOW_RANGE_COLOR_INDEX + 1.5;
    }
  else if (v > p.Range[1])
    {
    dIndex = p.MaxIndex + vtkLookupTable::ABOVE_RANGE_COLOR_INDEX + 1.5;
    }
  else
    {
    dIndex = (v + p.Shift)*p.Scale;

    // This conditional is needed because when v is very close to
    // p.Range[1], it may map above p.MaxIndex in the linear mapping
    // above.
    dIndex = (dIndex < p.MaxIndex ? dIndex : p.MaxIndex);
    }

  return static_cast<vtkIdType>(dIndex);
}

//----------------------------------------------------------------------------
// Get index and do the table lookup.
inline const unsigned char *NMVtkLinearLookupMain(double v,
                                                unsigned char *table,
                                                const TableParameters & p)
{
  vtkIdType index = NMVtkLinearIndexLookupMain(v, p);
  return &table[4*index];
}

//----------------------------------------------------------------------------
template<class T>
const unsigned char *NMVtkLinearLookup(T v, unsigned char *table, const TableParameters & p)
{
  return NMVtkLinearLookupMain(v, table, p);
}

//----------------------------------------------------------------------------
// Check for not-a-number when mapping double or float
inline const unsigned char *NMVtkLinearLookup(
  double v, unsigned char *table, const TableParameters & p)
{
  if (vtkMath::IsNan(v))
    {
    vtkIdType maxIndex = static_cast<vtkIdType>(p.MaxIndex + 0.5) + 1;
    return table + 4*(maxIndex + vtkLookupTable::NAN_COLOR_INDEX);
    }

  return NMVtkLinearLookupMain(v, table, p);
}

//----------------------------------------------------------------------------
inline const unsigned char *NMVtkLinearLookup(
  float v, unsigned char *table, const TableParameters & p)
{
  return NMVtkLinearLookup(static_cast<double>(v), table, p);
}

void
NMVtkLookupTable::MapScalarsThroughTable2(void *input,
                                             unsigned char *output,
                                             int inputDataType,
                                             int numberOfValues,
                                             int inputIncrement,
                                             int outputFormat)
{
  if ( this->IndexedLookup )
    {
    switch (inputDataType)
      {
      case VTK_BIT:
        {
        vtkIdType i, id;
        vtkBitArray *bitArray = vtkBitArray::New();
        bitArray->SetVoidArray(input,numberOfValues,1);
        vtkUnsignedCharArray *newInput = vtkUnsignedCharArray::New();
        newInput->SetNumberOfValues(numberOfValues);
        for (id=i=0; i<numberOfValues; i++, id+=inputIncrement)
          {
          newInput->SetValue(i, bitArray->GetValue(id));
          }
        NMVtkLookupTableIndexedMapData(this,
                                     static_cast<unsigned char*>(newInput->GetPointer(0)),
                                     output,numberOfValues,
                                     inputIncrement,outputFormat);
        newInput->Delete();
        bitArray->Delete();
        }
        break;

      vtkTemplateMacro(
        NMVtkLookupTableIndexedMapData(this,static_cast<VTK_TT*>(input),output,
                                     numberOfValues,inputIncrement,outputFormat)
        );

      case VTK_STRING:
        NMVtkLookupTableIndexedMapData(this,static_cast<vtkStdString*>(input),output,
                                     numberOfValues,inputIncrement,outputFormat);
        break;

      default:
        vtkErrorMacro(<< "MapScalarsThroughTable2: Unknown input ScalarType");
        return;
      }
    }
  else
    {
    TableParameters p;
    p.MaxIndex = this->GetNumberOfColors() - 1;
    p.UseIdxMap = this->mLutIdxMapping;
    p.pIdxMap = &this->mLutIdxMap;

    switch (inputDataType)
      {
      case VTK_BIT:
        {
        vtkIdType i, id;
        vtkBitArray *bitArray = vtkBitArray::New();
        bitArray->SetVoidArray(input,numberOfValues,1);
        vtkUnsignedCharArray *newInput = vtkUnsignedCharArray::New();
        newInput->SetNumberOfValues(numberOfValues);
        for (id=i=0; i<numberOfValues; i++, id+=inputIncrement)
          {
          newInput->SetValue(i, bitArray->GetValue(id));
          }
        NMVtkLookupTableMapData(this,
                              static_cast<unsigned char*>(newInput->GetPointer(0)),
                              output, numberOfValues,
                              inputIncrement, outputFormat, p);
        newInput->Delete();
        bitArray->Delete();
        }
        break;

      vtkTemplateMacro(
        NMVtkLookupTableMapData(this,static_cast<VTK_TT*>(input),output,
                              numberOfValues, inputIncrement, outputFormat, p)
        );
      default:
        vtkErrorMacro(<< "MapScalarsThroughTable2: Unknown input ScalarType");
        return;
      }
    }
}

//----------------------------------------------------------------------------
template<class T>
void NMVtkLookupTableMapData(NMVtkLookupTable *self, T *input,
                           unsigned char *output, int length,
                           int inIncr, int outFormat, TableParameters & p)
{
  int i = length;
  double *range = self->GetTableRange();
  const unsigned char *cptr;
  double alpha;

  // Resize the internal table to hold the special colors at the
  // end. When this function is called repeatedly with the same size
  // lookup table, memory reallocation will be done only one the first
  // call if at all.
  vtkUnsignedCharArray* lookupTable = self->GetTable();
  vtkIdType numberOfColors = lookupTable->GetNumberOfTuples();
  vtkIdType neededSize = (numberOfColors + vtkLookupTable::NUMBER_OF_SPECIAL_COLORS) *
    lookupTable->GetNumberOfComponents();
  if (lookupTable->GetSize() < neededSize)
    {
    lookupTable->Resize(numberOfColors + vtkLookupTable::NUMBER_OF_SPECIAL_COLORS);
    }
  unsigned char* table = lookupTable->GetPointer(0);

  // Writing directly to the memory location instead of adding them
  // with the InsertNextTupleValue() method does not affect how many
  // tuples lookupTable reports having, and it should be somewhat
  // faster.

  // Below range color
  unsigned char *tptr = table + 4*(numberOfColors + vtkLookupTable::BELOW_RANGE_COLOR_INDEX);
  unsigned char color[4];
  if (self->GetUseBelowRangeColor())
    {
    vtkLookupTable::GetColorAsUnsignedChars(self->GetBelowRangeColor(), color);
    tptr[0] = color[0];
    tptr[1] = color[1];
    tptr[2] = color[2];
    tptr[3] = color[3];
    }
  else
    {
    // Duplicate the first color in the table.
    tptr[0] = table[0];
    tptr[1] = table[1];
    tptr[2] = table[2];
    tptr[3] = table[3];
    }

  // Above range color
  tptr = table + 4*(numberOfColors + vtkLookupTable::ABOVE_RANGE_COLOR_INDEX);
  if (self->GetUseAboveRangeColor())
    {
    vtkLookupTable::GetColorAsUnsignedChars(self->GetAboveRangeColor(), color);
    tptr[0] = color[0];
    tptr[1] = color[1];
    tptr[2] = color[2];
    tptr[3] = color[3];
    }
  else
    {
    // Duplicate the last color in the table.
    tptr[0] = table[4*(numberOfColors-1) + 0];
    tptr[1] = table[4*(numberOfColors-1) + 1];
    tptr[2] = table[4*(numberOfColors-1) + 2];
    tptr[3] = table[4*(numberOfColors-1) + 3];
    }

  // Always use NanColor
  vtkLookupTable::GetColorAsUnsignedChars(self->GetNanColor(), color);
  tptr = table + 4*(numberOfColors + vtkLookupTable::NAN_COLOR_INDEX);
  tptr[0] = color[0];
  tptr[1] = color[1];
  tptr[2] = color[2];
  tptr[3] = color[3];

  if ( (alpha=self->GetAlpha()) >= 1.0 ) //no blending required
    {
    if (self->GetScale() == VTK_SCALE_LOG10)
      {
      double val;
      double logRange[2];
      NMVtkLookupTableLogRange(range, logRange);
      NMVtkLookupShiftAndScale(logRange, p.MaxIndex, p.Shift, p.Scale);
      p.Range[0] = logRange[0];
      p.Range[1] = logRange[1];

      if (outFormat == VTK_RGBA)
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(val, table, p);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          output[3] = cptr[3];
          input += inIncr;
          output += 4;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(val, table, p);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          input += inIncr;
          output += 3;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(val, table, p);
          output[0] = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          output[1] = cptr[3];
          input += inIncr;
          output += 2;
          }
        }
      else // outFormat == VTK_LUMINANCE
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(val, table, p);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          input += inIncr;
          }
        }
      }//if log scale

    else //not log scale
      {
      NMVtkLookupShiftAndScale(range, p.MaxIndex, p.Shift, p.Scale);
      p.Range[0] = range[0];
      p.Range[1] = range[1];
      unsigned char d1,d2,d3,d4;
      if (outFormat == VTK_RGBA)
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(*input, table, p);
          d1 = output[0] = cptr[0];
          d2 = output[1] = cptr[1];
          d3 = output[2] = cptr[2];
          d4 = output[3] = cptr[3];
          input += inIncr;
          output += 4;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(*input, table, p);
          d1 = output[0] = cptr[0];
          d2 = output[1] = cptr[1];
          d3 = output[2] = cptr[2];
          input += inIncr;
          output += 3;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(*input, table, p);
          output[0] = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          output[1] = cptr[3];
          input += inIncr;
          output += 2;
          }
        }
      else // outFormat == VTK_LUMINANCE
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(*input, table, p);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          input += inIncr;
          }
        }
      }//if not log lookup
    }//if blending not needed

  else //blend with the specified alpha
    {
    if (self->GetScale() == VTK_SCALE_LOG10)
      {
      double val;
      double logRange[2];
      NMVtkLookupTableLogRange(range, logRange);
      NMVtkLookupShiftAndScale(logRange, p.MaxIndex, p.Shift, p.Scale);
      p.Range[0] = logRange[0];
      p.Range[1] = logRange[1];

      if (outFormat == VTK_RGBA)
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(val, table, p);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          output[3] = static_cast<unsigned char>(cptr[3]*alpha + 0.5);
          input += inIncr;
          output += 4;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(val, table, p);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          input += inIncr;
          output += 3;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(val, table, p);
          output[0] = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          output[1] = static_cast<unsigned char>(alpha*cptr[3] + 0.5);
          input += inIncr;
          output += 2;
          }
        }
      else // outFormat == VTK_LUMINANCE
        {
        while (--i >= 0)
          {
          val = NMVtkApplyLogScale(*input, range, logRange);
          cptr = NMVtkLinearLookup(val, table, p);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          input += inIncr;
          }
        }
      }//log scale with blending

    else //no log scale with blending
      {
      NMVtkLookupShiftAndScale(range, p.MaxIndex, p.Shift, p.Scale);
      p.Range[0] = range[0];
      p.Range[1] = range[1];

      if (outFormat == VTK_RGBA)
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(*input, table, p);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          output[3] = static_cast<unsigned char>(cptr[3]*alpha + 0.5);
          input += inIncr;
          output += 4;
          }
        }
      else if (outFormat == VTK_RGB)
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(*input, table, p);
          output[0] = cptr[0];
          output[1] = cptr[1];
          output[2] = cptr[2];
          input += inIncr;
          output += 3;
          }
        }
      else if (outFormat == VTK_LUMINANCE_ALPHA)
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(*input, table, p);
          output[0] = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          output[1] = static_cast<unsigned char>(cptr[3]*alpha + 0.5);
          input += inIncr;
          output += 2;
          }
        }
      else // outFormat == VTK_LUMINANCE
        {
        while (--i >= 0)
          {
          cptr = NMVtkLinearLookup(*input, table, p);
          *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                                 cptr[2]*0.11 + 0.5);
          input += inIncr;
          }
        }
      }//no log scale
    }//alpha blending
}

//----------------------------------------------------------------------------
// Apply log to value, with appropriate constraints.
inline double NMVtkApplyLogScale(double v, const double range[2],
                               const double logRange[2])
{
  // is the range set for negative numbers?
  if (range[0] < 0)
    {
    if (v < 0)
      {
      v = -log10(-v);
      }
    else if (range[0] > range[1])
      {
      v = logRange[0];
      }
    else
      {
      v = logRange[1];
      }
    }
  else
    {
    if (v > 0)
      {
      v = log10(v);
      }
    else if (range[0] <= range[1])
      {
      v = logRange[0];
      }
    else
      {
      v = logRange[1];
      }
    }
  return v;
}



//----------------------------------------------------------------------------
template<class T>
void NMVtkLookupTableIndexedMapData(
  NMVtkLookupTable* self, T* input, unsigned char* output, int length,
  int inIncr, int outFormat )
{
  int i = length;
  unsigned char* cptr;
  double alpha;

  unsigned char nanColor[4];
  vtkLookupTable::GetColorAsUnsignedChars(self->GetNanColor(), nanColor);

  vtkVariant vin;
  if ( (alpha=self->GetAlpha()) >= 1.0 ) //no blending required
    {
    if (outFormat == VTK_RGBA)
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );

        output[0] = cptr[0];
        output[1] = cptr[1];
        output[2] = cptr[2];
        output[3] = cptr[3];
        input += inIncr;
        output += 4;
        }
      }
    else if (outFormat == VTK_RGB)
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );

        output[0] = cptr[0];
        output[1] = cptr[1];
        output[2] = cptr[2];
        input += inIncr;
        output += 3;
        }
      }
    else if (outFormat == VTK_LUMINANCE_ALPHA)
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );
        output[0] = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                               cptr[2]*0.11 + 0.5);
        output[1] = cptr[3];
        input += inIncr;
        output += 2;
        }
      }
    else // outFormat == VTK_LUMINANCE
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );
        *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                               cptr[2]*0.11 + 0.5);
        input += inIncr;
        }
      }
    } // if blending not needed

  else // blend with the specified alpha
    {
    if (outFormat == VTK_RGBA)
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );
        output[0] = cptr[0];
        output[1] = cptr[1];
        output[2] = cptr[2];
        output[3] = static_cast<unsigned char>(cptr[3]*alpha + 0.5);
        input += inIncr;
        output += 4;
        }
      }
    else if (outFormat == VTK_RGB)
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );
        output[0] = cptr[0];
        output[1] = cptr[1];
        output[2] = cptr[2];
        input += inIncr;
        output += 3;
        }
      }
    else if (outFormat == VTK_LUMINANCE_ALPHA)
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );
        output[0] = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                               cptr[2]*0.11 + 0.5);
        output[1] = static_cast<unsigned char>(cptr[3]*alpha + 0.5);
        input += inIncr;
        output += 2;
        }
      }
    else // outFormat == VTK_LUMINANCE
      {
      while (--i >= 0)
        {
        vin = *input;
        vtkIdType idx = self->GetAnnotatedValueIndexInternal( vin );
        cptr = idx < 0 ? nanColor : self->GetPointer( idx );
        *output++ = static_cast<unsigned char>(cptr[0]*0.30 + cptr[1]*0.59 +
                                               cptr[2]*0.11 + 0.5);
        input += inIncr;
        }
      }
    } // alpha blending
}
