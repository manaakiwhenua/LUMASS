/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2013 Landcare Research New Zealand Ltd
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
 * NMDataComponent.h
 *
 *  Created on: 12/02/2013
 *      Author: alex
 */

#ifndef NMDATACOMPONENT_H_
#define NMDATACOMPONENT_H_

#include <string>
#include <iostream>
#include <QMap>
#include <QDateTime>
#include <QStringList>

#include "NMMacros.h"
#include "NMModelComponent.h"
#include "NMItkDataObjectWrapper.h"


class NMDataComponent: public NMModelComponent
{
	Q_OBJECT
	//Q_PROPERTY(QStringList InputSpec READ getInputSpec WRITE setInputSpec NOTIFY NMDataComponentChanged)

public:

	//NMPropertyGetSet( InputSpec, QStringList );

	signals:
	void NMDataComponentChanged();

public:
	NMDataComponent(QObject* parent=0);
	virtual ~NMDataComponent(void);

    virtual void setNthInput(unsigned int idx, QSharedPointer<NMItkDataObjectWrapper> inputImg);
    virtual void linkComponents(unsigned int step, const QMap<QString, NMModelComponent*>& repo);
    virtual QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    virtual void update(const QMap<QString, NMModelComponent*>& repo);
    virtual void reset(void);

protected:

    QSharedPointer<NMItkDataObjectWrapper> mDataWrapper;
	//QStringList mInputSpec;

	unsigned int mParamPos;
	bool mbLinked;

    QDateTime mSourceMTime;
    QString mLastInputCompName;
	QString mInputCompName;
	unsigned int mInputOutputIdx;
	unsigned int mLastInputOutputIdx;

	virtual void initAttributes(void);
	void fetchData(NMModelComponent* comp);

private:
	static const std::string ctx;

};

//Q_DECLARE_METATYPE(QSharedPointer<NMItkDataObjectWrapper>)
Q_DECLARE_METATYPE(NMDataComponent)
Q_DECLARE_METATYPE(NMDataComponent*)


#endif /* NMDATACOMPONENT_H_ */
