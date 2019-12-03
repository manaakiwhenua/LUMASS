/*
 * MOSORunnable.h
 *
 *  Created on: 22/06/2013
 *      Author: alex
 */

#ifndef MOSORUNNABLE_H_
#define MOSORUNNABLE_H_

#include <qrunnable.h>
#include <QString>

#include "NMLogger.h"

class NMMosra;

class MOSORunnable: public QRunnable
{
public:
	MOSORunnable();
	virtual ~MOSORunnable();

	void setData(QString dsfileName,
			QString losSettingsFileName,
			QString perturbItem,
            const QList<float>& levels,
			int startIdx, int numruns);
	void run();
	void setLogger(NMLogger* logger) { mLogger = logger; }

protected:
    bool loadDataSet(NMMosra* mosra);

private:
	NMLogger* mLogger;
	QString mDsFileName;
	QString mLosFileName;
	QString mPerturbItem;
    QList<float> mflLevels;
	int mStartIdx;
	int mNumRuns;
};

#endif /* MOSORUNNABLE_H_ */
