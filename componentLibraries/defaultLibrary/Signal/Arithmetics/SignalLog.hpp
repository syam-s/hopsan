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
//! @file   SignalLog.hpp
//! @author Petter Krus <petter.krus@liu.se>
//! @date   2015-03-07
//!
//! @brief Contains a signal log function component
//!

#ifndef SIGNALLOG_HPP_INCLUDED
#define SIGNALLOG_HPP_INCLUDED

#include "ComponentEssentials.h"
#include <math.h>

namespace hopsan {

    //!
    //! @brief
    //! @ingroup SignalComponents
    //!
    class SignalLog : public ComponentSignal
    {

    private:
        double *mpND_in, *mpND_out;

    public:
        static Component *Creator()
        {
            return new SignalLog();
        }

        void configure()
        {
            addInputVariable("in", "", "", 0.0, &mpND_in);
            addOutputVariable("out", "log(in)","",&mpND_out);
        }


        void initialize()
        {
            simulateOneTimestep();
        }


        void simulateOneTimestep()
        {
            (*mpND_out) = log(*mpND_in);
        }
    };
}

#endif // SIGNALLOG_HPP_INCLUDED