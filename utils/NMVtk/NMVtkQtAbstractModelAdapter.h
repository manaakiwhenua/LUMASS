/*=========================================================================

  Program:   Visualization Toolkit
  Module:    NMVtkQtAbstractModelAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME NMVtkQtAbstractModelAdapter - Superclass for Qt model adapters.
//
// .SECTION Description
// NMVtkQtAbstractModelAdapter is the superclass for classes that adapt
// VTK objects to QAbstractItemModel. This class contains API for converting
// between QModelIndex and VTK ids, as well as some additional specialized
// functionality such as setting a column of data to use as the Qt header
// information.
//
// .SECTION See also
// vtkQtTableModelAdapter vtkQtTreeModelAdapter

/******************************************************************************
* Adapted by Alexander Herzig
* Copyright 2014 Landcare Research New Zealand Ltd
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
 * Due to API changes from Qt4 to Qt5 QAbstractItemModel::reset() became
 * unavailable. In this adaptation it is replaced by
 *
 * emit beginResetModel();
 * emit endResetModel();
 *
 */

#ifndef __NMVtkQtAbstractModelAdapter_h
#define __NMVtkQtAbstractModelAdapter_h

#include "QVTKWin32Header.h"
#include <QAbstractItemModel>
#include <QItemSelection>

class vtkDataObject;
class vtkSelection;
class QItemSelection;
class QVTK_EXPORT NMVtkQtAbstractModelAdapter : public QAbstractItemModel
{
  Q_OBJECT

public:

  // The view types.
  enum {
    FULL_VIEW,
    DATA_VIEW
  };

  NMVtkQtAbstractModelAdapter(QObject* p) :
    QAbstractItemModel(p), 
    ViewType(FULL_VIEW),
    KeyColumn(-1),
    ColorColumn(-1),
    DataStartColumn(-1),
    DataEndColumn(-1)
    { }

  // Description:
  // Set/Get the VTK data object as input to this adapter
  virtual void SetVTKDataObject(vtkDataObject *data) = 0;
  virtual vtkDataObject* GetVTKDataObject() const = 0;
  
  // Description:
  // Selection conversion from VTK land to Qt land
  virtual vtkSelection* QModelIndexListToVTKIndexSelection(
    const QModelIndexList qmil) const = 0;
  virtual QItemSelection VTKIndexSelectionToQItemSelection(
    vtkSelection *vtksel) const = 0;

  // Description:
  // Set/Get the view type.
  // FULL_VIEW gives access to all the data.
  // DATA_VIEW gives access only to the data columns specified with SetDataColumnRange()
  // The default is FULL_VIEW.
  virtual void SetViewType(int type) { this->ViewType = type; }
  virtual int GetViewType() { return this->ViewType; }

  // Description:
  // Set/Get the key column.
  // The key column is used as the row headers in a table view,
  // and as the first column in a tree view.
  // Set to -1 for no key column.
  // The default is no key column.
  virtual void SetKeyColumn(int col) { this->KeyColumn = col; }
  virtual int GetKeyColumn() { return this->KeyColumn; }
  virtual void SetKeyColumnName(const char* name) = 0;
  
  // Description:
  // Set/Get the column storing the rgba color values for each row.
  // The color column is used as the row headers in a table view,
  // and as the first column in a tree view.
  // Set to -1 for no key column.
  // The default is no key column.
  virtual void SetColorColumn(int col) { this->ColorColumn = col; }
  virtual int GetColorColumn() { return this->ColorColumn; }
  virtual void SetColorColumnName(const char* name) = 0;

  // Description:
  // Set the range of columns that specify the main data matrix.
  // The data column range should not include the key column.
  // The default is no data columns.
  virtual void SetDataColumnRange(int c1, int c2)
    { this->DataStartColumn = c1; this->DataEndColumn = c2; }

  // We make the reset() method public because it isn't always possible for
  // an adapter to know when its input has changed, so it must be callable
  // by an outside entity.
  void reset() { beginResetModel(); endResetModel(); }


signals:
  void beginModelReset();
  void endModelReset();
  void modelChanged();
  
protected:

  // Description:
  // Map a column index in the QAbstractItemModel to a vtkTable column.
  // If the argument is out of range or cannot be mapped then
  // this method may return -1.
  virtual int ModelColumnToFieldDataColumn(int col) const;

  int ViewType;
  int KeyColumn;
  int ColorColumn;
  int DataStartColumn;
  int DataEndColumn;
};

#endif
