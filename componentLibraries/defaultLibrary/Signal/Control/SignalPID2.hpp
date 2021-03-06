/*-----------------------------------------------------------------------------

 Copyright 2017 Hopsan Group

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.


 The full license is available in the file LICENSE.
 For details about the 'Hopsan Group' or information about Authors and
 Contributors see the HOPSANGROUP and AUTHORS files that are located in
 the Hopsan source code root directory.

-----------------------------------------------------------------------------*/

#ifndef SIGNALPID2_HPP_INCLUDED
#define SIGNALPID2_HPP_INCLUDED

#include "ComponentEssentials.h"
#include "ComponentUtilities.h"


//!
//! @file SignalPID2.hpp
//! @author Peter Nordin <peter.nordin@liu.se>
//! @brief PID controller with anti-windup
//! @ingroup SignalComponents
//!

namespace hopsan{

class SignalPID2 : public ComponentSignal
{
private:
    // States
    double mI;
    double mLastErr;
    bool mUseDeInput;

    // Constants
    double mK,mTi,mTd,mTt,mUmax,mUmin;

    // NodeData pointers
    double *mpErr, *mpDerr, *mpOut;


public:
    static Component *Creator()
    {
        return new SignalPID2();
    }

    void configure()
    {
        addInputVariable("e", "Control error", "", 0, &mpErr);
        addInputVariable("de", "Derivative signal input", "", 0, &mpDerr);
        addOutputVariable("u", "Control signal", "", &mpOut);

        addConstant("K", "Gain", "", 1, mK);
        addConstant("Ti", "Integral time", "s", 1, mTi);
        addConstant("Tt", "Anti-windup tracking constant", "s", 1, mTt);
        addConstant("Td", "Derivative time", "s", 0, mTd);
        addConstant("Umin", "", "", -1e100, mUmin);
        addConstant("Umax", "", "", 1e100, mUmax);
    }

    void initialize()
    {
        mI = 0.0;
        mLastErr = 0.0;

        // Decide if we should use de input for derivative signal
        mUseDeInput = (getPort("de")->isConnected());

        if (mTt < mTi)
        {
            addWarningMessage("Tt is lower then Ti this is not correct!");
        }
    }
    void simulateOneTimestep()
    {
        double dErr;

        // Read input control error
        const double err = (*mpErr);

        // Decide what derivative signal to use, de input or derivative in e input
        if (mUseDeInput)
        {
            dErr = (*mpDerr);
        }
        else
        {
            dErr = err-mLastErr;
            mLastErr = err;
        }

        //! @todo many of the constant based calculations can be pre calculated in initialize
        // Integrate
        mI = mI + mK * mTimestep/mTi * err;

        // Calculate control signal
        const double v = mK*err + mI + mK*mTd/mTimestep * dErr;

        // Limit to Umin Umax for anti-windup correction
        double u;
        if (v > mUmax)
        {
            u = mUmax;
        }
        else if (v < mUmin)
        {
            u = mUmin;
        }
        else
        {
            u = v;
        }

        // Anti-windup I correction
        mI = mI + mTimestep/mTt * (u-v);

        // Write output and remember error
        (*mpOut) = v;
    }
};
}
#endif // SIGNALPID2_HPP_INCLUDED
