
/*
 *  FFrameWidget.h
 *
 *  Written by:
 *   James Purciful
 *   Department of Computer Science
 *   University of Utah
 *   Jan. 1995
 *
 *  Copyright (C) 1995 SCI Group
 */


#ifndef SCI_project_Fixed_Frame_Widget_h
#define SCI_project_Fixed_Frame_Widget_h 1

#include <Widgets/BaseWidget.h>


// Variable indexs
enum { FFrameW_PointUL, FFrameW_PointUR, FFrameW_PointDR, FFrameW_PointDL,
       FFrameW_Dist1, FFrameW_Dist2, FFrameW_Hypo, FFrameW_Ratio };
// Material indexs
enum { FFrameW_PointMatl, FFrameW_EdgeMatl, FFrameW_HighMatl };


class FixedFrameWidget : public BaseWidget {
public:
   FixedFrameWidget( Module* module, CrowdMonitor* lock, double widget_scale );
   FixedFrameWidget( const FixedFrameWidget& );
   ~FixedFrameWidget();

   virtual void widget_execute();
   virtual void geom_moved(int, double, const Vector&, void*);

   inline const Vector& GetAxis1() const;
   inline const Vector& GetAxis2() const;
};


inline const Vector&
FixedFrameWidget::GetAxis1() const
{
   static Vector oldaxis;
   Vector axis(variables[FFrameW_PointUR]->Get() - variables[FFrameW_PointUL]->Get());
   if (axis.length2() <= 1e-6)
      return oldaxis;
   else
      return (oldaxis = axis.normal());
}


inline const Vector&
FixedFrameWidget::GetAxis2() const
{
   static Vector oldaxis;
   Vector axis(variables[FFrameW_PointDL]->Get() - variables[FFrameW_PointUL]->Get());
   if (axis.length2() <= 1e-6)
      return oldaxis;
   else
      return (oldaxis = axis.normal());
}


#endif
