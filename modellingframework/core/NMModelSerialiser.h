 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd 
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
 * NMModelSerialiser.h
 *
 *  Created on: 7/06/2012
 *      Author: alex
 */

#ifndef NMMODELSERIALISER_H_
#define NMMODELSERIALISER_H_

#include <string>
#include <iostream>

#include <qobject.h>
#include <QtXml>
#include <QString>
#include <QVariant>

//#include "nmlog.h"
#include "NMModelComponent.h"
#include "NMProcess.h"
#include "NMModelController.h"

#ifdef BUILD_RASSUPPORT
  #include "NMRasdamanConnectorWrapper.h"
#endif

#include "nmmodframecore_export.h"

class NMLogger;
class NMModelController;

class NMMODFRAMECORE_EXPORT NMModelSerialiser: public QObject, public NMModelObject
{
	Q_OBJECT

public:
	NMModelSerialiser(QObject* parent=0);
	virtual ~NMModelSerialiser();

	void serialiseComponent(NMModelComponent* comp,
            const QString& fileName, unsigned int indent,
			bool appendmode);
	void serialiseComponent(NMModelComponent* comp,
			QDomDocument& doc);

    QMap<QString, QString> parseComponent(const QString& fileName,
            NMIterableComponent* importHost, NMModelController* controller
#ifdef BUILD_RASSUPPORT			
			,
			NMRasdamanConnectorWrapper& rasWrapper
#endif			
			);

    void parseModelDocument(QMap<QString, QString>& nameRegister,
                            QDomDocument& doc,
                            NMIterableComponent* importHost);

    //void setLogger(NMLogger* logger);


protected:
	QDomElement createValueElement(QDomDocument& doc,
			QVariant& dataValue);
	QVariant extractPropertyValue(QDomElement& propElem);

    /*!
     * \brief Removes any carriage returns (CR, '\r') in front of
     *          newline character (LF, '\n'), which are put in by
     *          the QDomDocument.createTextNode(), which
     *          replaces any '\n' by "\r\n", which blows up the
     *          text in the NMHoverEdit dialog.
     * \param inStr
     * \return QString
     */
    QString removeSurplusCR(const QString& inStr);

	/*! Harmonises the process component input settings (i.e.
	 *  model component names) with imported model components
	 *  (i.e. accounting for any name adjustments during import)*/
	void harmoniseInputComponentNames(QMap<QString, QString>& nameRegister,
			NMModelController* controller);

    //NMLogger* mLogger;

private:
	string ctx;

#ifdef BUILD_RASSUPPORT	
	NMRasdamanConnectorWrapper* mRasconn;
#endif
	
};

#endif /* NMMODELSERIALISER_H_ */
