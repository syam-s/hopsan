#ifndef MECHANICTHETASOURCE_HPP_INCLUDED
#define MECHANICTHETASOURCE_HPP_INCLUDED

#include <iostream>
#include "../../ComponentEssentials.h"
#include "../../ComponentUtilities.h"
#include <math.h>
#include "matrix.h"

//!
//! @file MechanicThetaSource.hpp
//! @author Petter Krus <petter.krus@liu.se>
//! @date Sat 30 Jul 2011 00:35:23
//! @brief Angular position source
//! @ingroup MechanicComponents
//!
//This component is generated by COMPGEN for HOPSAN-NG simulation 
//from 
/*{, C:, Documents and Settings, petkr14, My Documents, \
CompgenNG}/Mechanic1DNG.nb*/

using namespace hopsan;

class MechanicThetaSource : public ComponentQ
{
private:
     double mthetain;
     double mwin;
     Port *mpPmr1;
     Port *mpPthetain;
     Port *mpPwin;
     int mNstep;
     //Port Pmr1 variable
     double tormr1;
     double thetamr1;
     double wmr1;
     double cmr1;
     double Zcmr1;
     double eqInertiamr1;
     //inputVariables
     double thetain;
     double win;
     //outputVariables
     //Port Pmr1 pointer
     double *mpND_tormr1;
     double *mpND_thetamr1;
     double *mpND_wmr1;
     double *mpND_cmr1;
     double *mpND_Zcmr1;
     double *mpND_eqInertiamr1;
     //Delay declarations
     //inputVariables pointers
     double *mpND_thetain;
     double *mpND_win;
     //outputVariables pointers

    Integrator mInt;

public:
     static Component *Creator()
     {
        std:://cout << "running MechanicThetaSource creator" << std::endl;
        return new MechanicThetaSource("ThetaSource");
     }

     MechanicThetaSource(const std::string name = "ThetaSource"
                             ,const double thetain = 0.
                             ,const double win = 0.
                             )
        : ComponentQ(name)
     {
        mNstep=9;
        mwin = win;

        //Add ports to the component
        mpPmr1=addPowerPort("Pmr1","NodeMechanicRotational");

        //Add inputVariables ports to the component
        mpPthetain=addReadPort("thetain","NodeSignal", Port::NOTREQUIRED);
        mpPwin=addReadPort("win","NodeSignal", Port::NOTREQUIRED);

        //Add outputVariables ports to the component

        //Register changable parameters to the HOPSAN++ core
        registerParameter("omega", "Angular Velocity", "rad/s", mwin);
     }

     void initialize()
     {
        //Read port variable pointers from nodes
        //Port Pmr1
        mpND_tormr1=getSafeNodeDataPtr(mpPmr1, NodeMechanicRotational::TORQUE);
        mpND_thetamr1=getSafeNodeDataPtr(mpPmr1, NodeMechanicRotational::ANGLE);
        mpND_wmr1=getSafeNodeDataPtr(mpPmr1, NodeMechanicRotational::ANGULARVELOCITY);
        mpND_cmr1=getSafeNodeDataPtr(mpPmr1, NodeMechanicRotational::WAVEVARIABLE);
        mpND_Zcmr1=getSafeNodeDataPtr(mpPmr1, NodeMechanicRotational::CHARIMP);
        mpND_eqInertiamr1=getSafeNodeDataPtr(mpPmr1, NodeMechanicRotational::EQINERTIA);
        //Read inputVariables pointers from nodes
        mpND_thetain=getSafeNodeDataPtr(mpPthetain, NodeSignal::VALUE,mthetain);
        mpND_win=getSafeNodeDataPtr(mpPwin, NodeSignal::VALUE,mwin);
        //Read outputVariable pointers from nodes

        //Read variables from nodes
        //Port Pmr1
        tormr1 = (*mpND_tormr1);
        thetamr1 = (*mpND_thetamr1);
        wmr1 = (*mpND_wmr1);
        cmr1 = (*mpND_cmr1);
        Zcmr1 = (*mpND_Zcmr1);
        eqInertiamr1 = (*mpND_eqInertiamr1);

        //Read inputVariables from nodes
        thetain = (*mpND_thetain);
        win = (*mpND_win);

        //Read outputVariables from nodes

        //Initialize delays


        mInt.initialize(mTimestep, wmr1, thetamr1);


        if(mpPthetain->isConnected() && !mpPwin->isConnected())
        {
            stringstream ss;
            ss << "Angle input is connected but angular velocity is constant, kinematic relationsship must be manually enforced.";
            addWarningMessage(ss.str());
        }
        else if(mpPthetain->isConnected() && mpPwin->isConnected())
        {
            stringstream ss;
            ss << "Both angle and velocity inputs are connected, kinematic relationsship must be manually enforced.";
            addWarningMessage(ss.str());
        }
     }

     void simulateOneTimestep()
     {
        //Read variables from nodes
        //Port Pmr1
        cmr1 = (*mpND_cmr1);
        Zcmr1 = (*mpND_Zcmr1);

        //Read inputVariables from nodes
        thetain = (*mpND_thetain);
        win = (*mpND_win);

        //LocalExpressions

        //Expressions
        double wmr1 = win;
        double thetamr1;
        if(mpPthetain->isConnected())
        {
            thetamr1 = thetain;
        }
        else
        {
            thetamr1 = mInt.update(wmr1);
        }
        tormr1 = cmr1 + Zcmr1*wmr1;

        //Write new values to nodes
        //Port Pmr1
        (*mpND_tormr1)=tormr1;
        (*mpND_thetamr1)=thetamr1;
        (*mpND_wmr1)=wmr1;
        (*mpND_eqInertiamr1)=eqInertiamr1;
        //outputVariables

        //Update the delayed variabels

     }
};
#endif // MECHANICTHETASOURCE_HPP_INCLUDED
