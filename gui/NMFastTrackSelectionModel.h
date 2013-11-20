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

	void setSelection(const QItemSelection& newSel);//,
			//QItemSelectionModel::SelectionFlags command);
};

#endif /* NMFASTTRACKSELECTIONMODEL_H_ */



#ifndef NMFASTTRACKSELECTIONMODEL_P_H_
#define NMFASTTRACKSELECTIONMODEL_P_H_

#include <private/qitemselectionmodel_p.h>
class NMFastTrackSelectionModelPrivate : public QItemSelectionModelPrivate
{
	Q_DECLARE_PUBLIC(NMFastTrackSelectionModel)
public:
	NMFastTrackSelectionModelPrivate() {}

};


#endif /* NMFASTTRACKSELECTIONMODEL_P_H_ */
