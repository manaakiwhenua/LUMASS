/*
 * NMFastTrackSelectionModel.h
 *
 *  Created on: 19/11/2013
 *      Author: alex
 */

#ifndef NMFASTTRACKSELECTIONMODEL_H_
#define NMFASTTRACKSELECTIONMODEL_H_

#include <qitemselectionmodel.h>

class NMFastTrackSelectionModel: public QItemSelectionModel
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(QItemSelectionModel)

public:
	NMFastTrackSelectionModel(QAbstractItemModel* model);
	NMFastTrackSelectionModel(QAbstractItemModel* model, QObject* parent=0);
	virtual ~NMFastTrackSelectionModel();

	void setSelection(const QItemSelection& newSel,
			QItemSelectionModel::SelectionFlags command);
};

#endif /* NMFASTTRACKSELECTIONMODEL_H_ */
