#include "NMVtkMapScale.h"

#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"
#include "vtkViewport.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkActor2D.h"
#include "vtkTextMapper.h"
#include "vtkCoordinate.h"

#include <string>

const std::string NMVtkMapScale::ctx = "NMVtkMapScale";
//#define ctxNMVtkMapScale = "NMVtkMapScale";


//vtkSandardNewMacro(NMVtkMapScale);

NMVtkMapScale*
NMVtkMapScale::New()
{
    return new NMVtkMapScale();
}


NMVtkMapScale::NMVtkMapScale()
    : vtkLegendScaleActor(),
      mBarHeight(5),
      mBottomPos(12),
      mNumSegments(4),
      mUnits("")
{
}

NMVtkMapScale::~NMVtkMapScale()
{
}

void
NMVtkMapScale::BuildRepresentation(vtkViewport *viewport)
{
    Superclass::BuildRepresentation(viewport);

    int* size = viewport->GetSize();

    if ( this->LegendVisibility )
    {
        // calc initial position and length of scale bar
        double xr1 = 0.33333*size[0];
        double xr2 = 0.66666*size[0];
        this->Coordinate->SetValue(0.33333*size[0],15,0.0);
        double *xr = this->Coordinate->GetComputedWorldValue(viewport);
        double xrL[3]; xrL[0]=xr[0];xrL[1]=xr[1];xrL[2]=xr[2];
        this->Coordinate->SetValue(0.66667*size[0],15,0.0);
        xr = this->Coordinate->GetComputedWorldValue(viewport);
        double xrR[3]; xrR[0]=xr[0];xrR[1]=xr[1];xrR[2]=xr[2];
        double rlen = sqrt(vtkMath::Distance2BetweenPoints(xrL,xrR));

        // calc final position and nice length & interval for scale bar
        int ilen = vtkMath::Round(rlen);
        int divisor = 1e6;
        while (ilen > 0 && (ilen % divisor) >= ilen)
        {
            divisor /= 10;
        }

        ilen = ilen - (ilen % divisor);
        double lenIncr = (double)ilen / (double)mNumSegments;

        this->Coordinate->SetValue(0.5*size[0],15.0,0.0);
        xr = this->Coordinate->GetComputedWorldValue(viewport);
        double x1W = xr[0] - 2*lenIncr;
        double x2W = xr[0] + 2*lenIncr;

        this->Coordinate->SetCoordinateSystemToWorld();
        this->Coordinate->SetValue(x1W, xrL[1], xrL[2]);
        xr = this->Coordinate->GetComputedDoubleDisplayValue(viewport);
        double x1 = xr[0];
        this->Coordinate->SetValue(x2W, xrL[1], xrL[2]);
        xr = this->Coordinate->GetComputedDoubleDisplayValue(viewport);
        double dlen = std::abs(xr[0] - x1);
        double delX = dlen / (double)mNumSegments;

        this->Coordinate->SetCoordinateSystemToDisplay();

        // update scale bar cooridnates
        this->LegendPoints->SetPoint(0, x1,mBottomPos,0);
        this->LegendPoints->SetPoint(1, x1+delX,mBottomPos,0);
        this->LegendPoints->SetPoint(2, x1+2*delX,mBottomPos,0);
        this->LegendPoints->SetPoint(3, x1+3*delX,mBottomPos,0);
        this->LegendPoints->SetPoint(4, x1+4*delX,mBottomPos,0);
        this->LegendPoints->SetPoint(5, x1,mBottomPos+mBarHeight,0);
        this->LegendPoints->SetPoint(6, x1+delX,mBottomPos+mBarHeight,0);
        this->LegendPoints->SetPoint(7, x1+2*delX,mBottomPos+mBarHeight,0);
        this->LegendPoints->SetPoint(8, x1+3*delX,mBottomPos+mBarHeight,0);
        this->LegendPoints->SetPoint(9, x1+4*delX,mBottomPos+mBarHeight,0);

        // Specify the position of the legend title
        this->LabelActors[5]->SetPosition(0.5*size[0],mBottomPos+mBarHeight+2);
        double *x = 0;
        char buf[256];
        double scale = (double)ilen / ((dlen > 0 ? dlen : 1e-12) / ((double)mDPI/2.54*100));

        //        NMDebugAI(<< "DPI=" << mDPI
        //                  << " dpm=" << (mDPI*100/2.54)
        //                  << " ilen=" << ilen
        //                  << " dlen=" << dlen
        //                  << std::endl);

        if (mUnits == "m")
        {
            sprintf(buf,"1 : %d",vtkMath::Round(scale));
            this->LabelMappers[5]->SetInput(buf);
        }
        else
        {
            this->LabelMappers[5]->SetInput("");
        }

        // Now specify the position of the legend labels
        x = this->LegendPoints->GetPoint(0);
        this->LabelActors[0]->SetPosition(x[0],x[1]-1);
        x = this->LegendPoints->GetPoint(1);
        this->LabelActors[1]->SetPosition(x[0],x[1]-1);
        x = this->LegendPoints->GetPoint(2);
        this->LabelActors[2]->SetPosition(x[0],x[1]-1);
        x = this->LegendPoints->GetPoint(3);
        this->LabelActors[3]->SetPosition(x[0],x[1]-1);
        x = this->LegendPoints->GetPoint(4);
        this->LabelActors[4]->SetPosition(x[0],x[1]-1);


        this->LabelMappers[0]->SetInput("0");
        sprintf(buf, "%g", (lenIncr));
        this->LabelMappers[1]->SetInput(buf);
        sprintf(buf, "%g", (2*lenIncr));
        this->LabelMappers[2]->SetInput(buf);
        sprintf(buf, "%g", (3*lenIncr));
        this->LabelMappers[3]->SetInput(buf);
        sprintf(buf, "%g %s", (4*lenIncr), mUnits.c_str());
        this->LabelMappers[4]->SetInput(buf);

    }

}
