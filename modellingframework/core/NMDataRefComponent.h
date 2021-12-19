/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2017 Landcare Research New Zealand Ltd
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
 * NMDataRefComponent.h
 *
 *  Created on: 30/03/2017
 *      Author: alex
 */

#ifndef NMDATAREFCOMPONENT_H_
#define NMDATAREFCOMPONENT_H_

#include <QObject>
#include <QString>

#include "NMMacros.h"
#include "NMDataComponent.h"

//#include "NMModelComponent.h"
//#include "NMItkDataObjectWrapper.h"
#include "nmmodframecore_export.h"



class NMMODFRAMECORE_EXPORT NMDataRefComponent: public NMDataComponent
{
    Q_OBJECT
    Q_PROPERTY(QString DataComponentName READ getDataComponentName WRITE setDataComponentName)

public:

    NMPropertyGetSet( DataComponentName, QString )

    NMDataRefComponent(QObject* parent=0);
    virtual ~NMDataRefComponent(void);

    virtual void setNthInput(unsigned int idx, QSharedPointer<NMItkDataObjectWrapper> inputImg, const QString& name="");
    virtual QSharedPointer<NMItkDataObjectWrapper> getOutput(unsigned int idx);
    virtual QSharedPointer<NMItkDataObjectWrapper> getOutput(const QString& name);
    virtual void linkComponents(unsigned int step, const QMap<QString, NMModelComponent*>& repo);
    virtual void update(const QMap<QString, NMModelComponent*>& repo);
    virtual void reset(void);

protected:

//    QSharedPointer<NMItkDataObjectWrapper> mDataWrapper;
    QString mDataComponentName;

    NMDataComponent* mDataComponent;

    long long mTabMinPK;
    unsigned int mParamPos;
    bool mbLinked;

    QDateTime mSourceMTime;
    QString mLastInputCompName;
    QString mInputCompName;
    unsigned int mInputOutputIdx;
    unsigned int mLastInputOutputIdx;

    virtual QVariant getModelParameter(const QString &paramSpec);

    virtual void initAttributes(void);
    void fetchData(NMModelComponent* comp);
    void updateDataSource(void);

private:
    static const std::string ctx;

};

//Q_DECLARE_METATYPE(QSharedPointer<NMItkDataObjectWrapper>)
Q_DECLARE_METATYPE(NMDataRefComponent)
Q_DECLARE_METATYPE(NMDataRefComponent*)


#endif /* NMDATAREFCOMPONENT_H_ */
