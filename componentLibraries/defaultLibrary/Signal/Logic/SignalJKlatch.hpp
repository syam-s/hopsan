/*-----------------------------------------------------------------------------
 This source file is a part of Hopsan

 Copyright (c) 2009 to present year, Hopsan Group

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 For license details and information about the Hopsan Group see the files
 GPLv3 and HOPSANGROUP in the Hopsan source code root directory

 For author and contributor information see the AUTHORS file
-----------------------------------------------------------------------------*/

#ifndef SIGNALJKLATCH_HPP_INCLUDED
#define SIGNALJKLATCH_HPP_INCLUDED

#include <iostream>
#include "ComponentEssentials.h"
#include "ComponentUtilities.h"
#include "math.h"

//!
//! @file SignalJKlatch.hpp
//! @author Petter Krus <petter.krus@liu.se>
//! @date Sun 29 Mar 2015 12:41:28
//! @brief J-K latch
//! @ingroup SignalComponents
//!
//==This code has been autogenerated using Compgen==
//from 
/*{, C:, HopsanTrunk, componentLibraries, defaultLibrary, Signal, \
Logic}/SignalJKlatch.nb*/

using namespace hopsan;

class SignalJKlatch : public ComponentSignal
{
private:
     int mNstep;
//==This code has been autogenerated using Compgen==
     //inputVariables
     double setCond;
     double resetCond;
     //outputVariables
     double Qstate;
     double notQstate;
     //InitialExpressions variables
     double oldQstate;
     double oldSetCond;
     double oldResetCond;
     //Expressions variables
     //Delay declarations
//==This code has been autogenerated using Compgen==
     //inputVariables pointers
     double *mpsetCond;
     double *mpresetCond;
     //inputParameters pointers
     //outputVariables pointers
     double *mpQstate;
     double *mpnotQstate;
     EquationSystemSolver *mpSolver;

public:
     static Component *Creator()
     {
        return new SignalJKlatch();
     }

     void configure()
     {
//==This code has been autogenerated using Compgen==

        mNstep=9;

        //Add ports to the component
        //Add inputVariables to the component
            addInputVariable("setCond","On condition","",0.,&mpsetCond);
            addInputVariable("resetCond","off condition","",0.,&mpresetCond);

        //Add inputParammeters to the component
        //Add outputVariables to the component
            addOutputVariable("Qstate","Logical state","",0.,&mpQstate);
            addOutputVariable("notQstate","Logical inverse of \
state","",1.,&mpnotQstate);

//==This code has been autogenerated using Compgen==
        //Add constantParameters
     }

    void initialize()
     {
        //Read port variable pointers from nodes

        //Read variables from nodes

        //Read inputVariables from nodes
        setCond = (*mpsetCond);
        resetCond = (*mpresetCond);

        //Read inputParameters from nodes

        //Read outputVariables from nodes
        Qstate = (*mpQstate);
        notQstate = (*mpnotQstate);

//==This code has been autogenerated using Compgen==
        //InitialExpressions
        oldQstate = Qstate;
        oldSetCond = setCond;
        oldResetCond = resetCond;


        //Initialize delays

     }
    void simulateOneTimestep()
     {
        //Read variables from nodes

        //Read inputVariables from nodes
        setCond = (*mpsetCond);
        resetCond = (*mpresetCond);

        //Read inputParameters from nodes

        //LocalExpressions

          //Expressions
          Qstate = limit(limit(setCond*(1 - limit(resetCond,0,1)),0,1) - \
limit(resetCond*(1 - limit(setCond,0,1)),0,1) + limit((1 - \
limit(oldQstate,0,1))*limit(resetCond*setCond,0,1),0,1) + \
limit(oldQstate*limit((1 - limit(resetCond,0,1))*(1 - \
limit(setCond,0,1)),0,1),0,1),0,1);
          oldQstate = Qstate;
          notQstate = 1 - Qstate;

        //Calculate the delayed parts


        //Write new values to nodes
        //outputVariables
        (*mpQstate)=Qstate;
        (*mpnotQstate)=notQstate;

        //Update the delayed variabels

     }
    void deconfigure()
    {
        delete mpSolver;
    }
};
#endif // SIGNALJKLATCH_HPP_INCLUDED
