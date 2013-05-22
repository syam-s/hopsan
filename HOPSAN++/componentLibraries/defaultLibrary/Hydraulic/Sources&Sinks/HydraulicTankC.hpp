/*-----------------------------------------------------------------------------
 This source file is part of Hopsan NG

 Copyright (c) 2011 
    Mikael Axin, Robert Braun, Alessandro Dell'Amico, Björn Eriksson,
    Peter Nordin, Karl Pettersson, Petter Krus, Ingo Staack

 This file is provided "as is", with no guarantee or warranty for the
 functionality or reliability of the contents. All contents in this file is
 the original work of the copyright holders at the Division of Fluid and
 Mechatronic Systems (Flumes) at Linköping University. Modifying, using or
 redistributing any part of this file is prohibited without explicit
 permission from the copyright holders.
-----------------------------------------------------------------------------*/

//!
//! @file   HydraulicPressureSource.hpp
//! @author Robert Braun <robert.braun@liu.se>
//! @date   2009-12-21
//!
//! @brief Contains a Hydraulic Tank Component of C-type
//!
//$Id$

#ifndef HYDRAULICTANKC_HPP_INCLUDED
#define HYDRAULICTANKC_HPP_INCLUDED

#include <iostream>
#include "ComponentEssentials.h"

namespace hopsan {

    //!
    //! @brief
    //! @ingroup HydraulicComponents
    //!
    class HydraulicTankC : public ComponentC
    {
    private:
        double mZc;
        double mPressure;

        double *mpND_p, *mpND_c, *mpND_Zc;

        Port *mpP1;

    public:
        static Component *Creator()
        {
            return new HydraulicTankC();
        }

        void configure()
        {
            mpP1 = addPowerPort("P1", "NodeHydraulic");
            addConstant("p", "Default Pressure", "Pa", 1.0e5, mPressure);
            disableStartValue(mpP1, NodeHydraulic::Pressure);
            setDefaultStartValue(mpP1, NodeHydraulic::Flow, 0.0);
        }


        void initialize()
        {
            mZc = 0.0;

            mpND_p = getSafeNodeDataPtr(mpP1, NodeHydraulic::Pressure);
            mpND_c = getSafeNodeDataPtr(mpP1, NodeHydraulic::WaveVariable);
            mpND_Zc = getSafeNodeDataPtr(mpP1, NodeHydraulic::CharImpedance);

            //Override the start value
            (*mpND_p) = mPressure;
            (*mpND_c) = mPressure;
            (*mpND_Zc) = mZc;
        }


        void simulateOneTimestep()
        {
            //Nothing will change
        }

        void finalize()
        {

        }
    };
}

#endif // HYDRAULICTANKC_HPP_INCLUDED
