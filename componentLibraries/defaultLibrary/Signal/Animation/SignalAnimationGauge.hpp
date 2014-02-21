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
//! @file   SignalAnimationGauge.hpp
//! @author Robert Braun <robert.braun@liu.se>
//! @date   2012-10-17
//!
//! @brief Contains a Signal Animated Gauge (does nothing in the simulation)
//!
//$Id$

#ifndef SIGNALANIMATIONGAUGE_HPP_INCLUDED
#define SIGNALANIMATIONGAUGE_HPP_INCLUDED

#include "ComponentEssentials.h"

namespace hopsan {

    //!
    //! @brief
    //! @ingroup SignalComponents
    //!
    class SignalAnimationGauge : public ComponentSignal
    {

    private:
        double max;

    public:
        static Component *Creator()
        {
            return new SignalAnimationGauge();
        }

        void configure()
        {
            addInputVariable("in", "", "", 0.0);
            addConstant("max", "Upper limit", "", 1, max);
        }


        void initialize()
        {

        }


        void simulateOneTimestep()
        {

        }
    };
}

#endif //SIGNALANIMATIONGAUGE_HPP_INCLUDED