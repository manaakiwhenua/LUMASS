#ifndef NMVTKMAPSCALE_H
#define NMVTKMAPSCALE_H

#include "nmlog.h"
#include "vtkLegendScaleActor.h"

class NMVtkMapScale : public vtkLegendScaleActor
{
public:
    static NMVtkMapScale* New();

    vtkTypeMacro(NMVtkMapScale, vtkLegendScaleActor);

    virtual void BuildRepresentation(vtkViewport *viewport);

protected:
    NMVtkMapScale();
    ~NMVtkMapScale();

private:
    NMVtkMapScale(const NMVtkMapScale&);
    void operator=(const NMVtkMapScale&);

    const static std::string ctx;
};

#endif // NMVTKMAPSCALE_H
