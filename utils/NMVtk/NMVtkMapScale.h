#ifndef NMVTKMAPSCALE_H
#define NMVTKMAPSCALE_H

#include "nmlog.h"
#include "vtkLegendScaleActor.h"

/*!
 * \brief The NMVtkMapScale class
 *
 * This class utilises he vtkLegendScaleActor and
 * refines its representation of the legend (scale bar).
 *
 * \sa vtkLegendScaleActor
 */

class NMVtkMapScale : public vtkLegendScaleActor
{
public:
    static NMVtkMapScale* New();

    vtkTypeMacro(NMVtkMapScale, vtkLegendScaleActor);

    // tell the map scale the dpi of the display device
    void SetDPI(int dpi){mDPI = dpi;}
    virtual void BuildRepresentation(vtkViewport *viewport);

protected:
    NMVtkMapScale();
    ~NMVtkMapScale();

    // scale bar height in pixels
    double mBarHeight;
    double mBottomPos;
    int mNumSegments;
    std::string mUnits;
    int mDPI;

private:
    NMVtkMapScale(const NMVtkMapScale&);
    void operator=(const NMVtkMapScale&);

    const static std::string ctx;
};

#endif // NMVTKMAPSCALE_H
