/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010,2011,2012,2013 Landcare Research New Zealand Ltd
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
 * NMFastTrackSelectionModel.h
 *
 *  Created on: 19/11/2013
 *      Author: alex
 */

#ifndef NMFASTTRACKSELECTIONMODEL_H_
#define NMFASTTRACKSELECTIONMODEL_H_

#include <qitemselectionmodel.h>
class NMFastTrackSelectionModelPrivate;
class NMFastTrackSelectionModel: public QItemSelectionModel
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(NMFastTrackSelectionModel)

public:
	NMFastTrackSelectionModel(QAbstractItemModel* model);
	NMFastTrackSelectionModel(QAbstractItemModel* model, QObject* parent=0);
	virtual ~NMFastTrackSelectionModel();

    void setSelection(const QItemSelection& newSel,
                      bool expToRows=false);
	QItemSelection getSelection(void);
	void toggleRow(int row, int column,
			const QModelIndex& parent=QModelIndex());
};

#endif /* NMFASTTRACKSELECTIONMODEL_H_ */



//#ifndef NMFASTTRACKSELECTIONMODEL_P_H_
//#define NMFASTTRACKSELECTIONMODEL_P_H_
//
//#include <private/qitemselectionmodel_p.h>
//class NMFastTrackSelectionModelPrivate : public QItemSelectionModelPrivate
//{
//	Q_DECLARE_PUBLIC(NMFastTrackSelectionModel)
//public:
//	NMFastTrackSelectionModelPrivate() {}
//
//};
//
//
//#endif /* NMFASTTRACKSELECTIONMODEL_P_H_ */
