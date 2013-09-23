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


class MOSORunnable: public QRunnable
{
public:
	MOSORunnable();
	virtual ~MOSORunnable();

	void setData(QString dsfileName,
			QString losSettingsFileName,
			QString perturbItem,
			float level,
			int startIdx, int numruns);
	void run();

private:
	QString mDsFileName;
	QString mLosFileName;
	QString mPerturbItem;
	float mLevel;
	int mStartIdx;
	int mNumRuns;
};

#endif /* MOSORUNNABLE_H_ */
