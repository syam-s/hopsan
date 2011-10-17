#ifndef MECHANICVEHICLE1D_HPP_INCLUDED
#define MECHANICVEHICLE1D_HPP_INCLUDED

#include <iostream>
#include "../../ComponentEssentials.h"
#include "../../ComponentUtilities.h"
#include <math.h>
#include "matrix.h"

//!
//! @file MechanicVehicle1D.hpp
//! @author Petter Krus <petter.krus@liu.se>
//! @date Tue 5 Jul 2011 10:47:50
//! @brief This is a one dimmensional model of a vehicle
//! @ingroup MechanicComponents
//!
//This component is generated by COMPGEN for HOPSAN-NG simulation 
//from 
/*{, C:, Documents and Settings, petkr14, My Documents, \
CompgenNG}/ElectricNG5.nb*/

using namespace hopsan;

class MechanicVehicle1D : public ComponentQ
{
private:
     double mMc;
     double mcfr;
     double mCdA;
     double mrwheel;
     double mrho;
     Port *mpPmr1;
     Port *mpPmr2;
     Port *mpPvc;
     Port *mpPxc;
     Port *mpPfd;
     Port *mpPfr;
     double delayParts1[9];
     double delayParts2[9];
     double delayParts3[9];
     double delayParts4[9];
     double delayParts5[9];
     Matrix jacobianMatrix;
     Vec systemEquations;
     Matrix delayedPart;
     int i;
     int iter;
     int mNoiter;
     int jsyseqnweight[4];
     int order[5];
     int mNstep;
     //Port Pmr1 variable
     double tormr1;
     double thetamr1;
     double wmr1;
     double cmr1;
     double Zcmr1;
     //Port Pmr2 variable
     double tormr2;
     double thetamr2;
     double wmr2;
     double cmr2;
     double Zcmr2;
     //inputVariables
     //outputVariables
     double vc;
     double xc;
     double fd;
     double fr;
     //Port Pmr1 pointer
     double *mpND_tormr1;
     double *mpND_thetamr1;
     double *mpND_wmr1;
     double *mpND_cmr1;
     double *mpND_Zcmr1;
     //Port Pmr2 pointer
     double *mpND_tormr2;
     double *mpND_thetamr2;
     double *mpND_wmr2;
     double *mpND_cmr2;
     double *mpND_Zcmr2;
     //Delay declarations
     //inputVariables pointers
     //outputVariables pointers
     double *mpND_vc;
     double *mpND_xc;
     double *mpND_fd;
     double *mpND_fr;
     Delay mDelayedPart10;
     Delay mDelayedPart11;
     Delay mDelayedPart20;
     Delay mDelayedPart21;
     Delay mDelayedPart30;
     Delay mDelayedPart31;

public:
     static Component *Creator()
     {
        std:://cout << "running MechanicVehicle1D creator" << std::endl;
        return new MechanicVehicle1D("Vehicle1D");
     }

     MechanicVehicle1D(const std::string name = "Vehicle1D"
                             ,const double Mc = 1000.
                             ,const double cfr = 0.04
                             ,const double CdA = 0.5
                             ,const double rwheel = 1.
                             ,const double rho = 1.25
                             )
        : ComponentQ(name)
     {
        mNstep=9;
        jacobianMatrix.create(5,5);
        systemEquations.create(5);
        delayedPart.create(6,6);
        mNoiter=2;
        jsyseqnweight[0]=1;
        jsyseqnweight[1]=0.67;
        jsyseqnweight[2]=0.5;
        jsyseqnweight[3]=0.5;

        mMc = Mc;
        mcfr = cfr;
        mCdA = CdA;
        mrwheel = rwheel;
        mrho = rho;

        //Add ports to the component
        mpPmr1=addPowerPort("Pmr1","NodeMechanicRotational");
        mpPmr2=addPowerPort("Pmr2","NodeMechanicRotational");

        //Add inputVariables ports to the component

        //Add outputVariables ports to the component
        mpPvc=addWritePort("Pvc","NodeSignal", Port::NOTREQUIRED);
        mpPxc=addWritePort("Pxc","NodeSignal", Port::NOTREQUIRED);
        mpPfd=addWritePort("Pfd","NodeSignal", Port::NOTREQUIRED);
        mpPfr=addWritePort("Pfr","NodeSignal", Port::NOTREQUIRED);

        //Register changable parameters to the HOPSAN++ core
        registerParameter("M_c", "Vehicle Inertia", "kg", mMc);
        registerParameter("c_fr", "C Roll. Resist.Coeff.", "N/N", mcfr);
        registerParameter("C_dA", "Effective Front Area", "m^2", mCdA);
        registerParameter("r_wheel", "Wheel Radius", "m", mrwheel);
        registerParameter("rho", "Air Density", "kg/m3", mrho);
     }

     void initialize()
     {
        //Read port variable pointers from nodes
        //Port Pmr1
        mpND_tormr1=getSafeNodeDataPtr(mpPmr1, \
NodeMechanicRotational::TORQUE);
        mpND_thetamr1=getSafeNodeDataPtr(mpPmr1, \
NodeMechanicRotational::ANGLE);
        mpND_wmr1=getSafeNodeDataPtr(mpPmr1, \
NodeMechanicRotational::ANGULARVELOCITY);
        mpND_cmr1=getSafeNodeDataPtr(mpPmr1, \
NodeMechanicRotational::WAVEVARIABLE);
        mpND_Zcmr1=getSafeNodeDataPtr(mpPmr1, \
NodeMechanicRotational::CHARIMP);
        //Port Pmr2
        mpND_tormr2=getSafeNodeDataPtr(mpPmr2, \
NodeMechanicRotational::TORQUE);
        mpND_thetamr2=getSafeNodeDataPtr(mpPmr2, \
NodeMechanicRotational::ANGLE);
        mpND_wmr2=getSafeNodeDataPtr(mpPmr2, \
NodeMechanicRotational::ANGULARVELOCITY);
        mpND_cmr2=getSafeNodeDataPtr(mpPmr2, \
NodeMechanicRotational::WAVEVARIABLE);
        mpND_Zcmr2=getSafeNodeDataPtr(mpPmr2, \
NodeMechanicRotational::CHARIMP);
        //Read inputVariables pointers from nodes
        //Read outputVariable pointers from nodes
        mpND_vc=getSafeNodeDataPtr(mpPvc, NodeSignal::VALUE);
        mpND_xc=getSafeNodeDataPtr(mpPxc, NodeSignal::VALUE);
        mpND_fd=getSafeNodeDataPtr(mpPfd, NodeSignal::VALUE);
        mpND_fr=getSafeNodeDataPtr(mpPfr, NodeSignal::VALUE);

        //Read variables from nodes
        //Port Pmr1
        tormr1 = (*mpND_tormr1);
        thetamr1 = (*mpND_thetamr1);
        wmr1 = (*mpND_wmr1);
        cmr1 = (*mpND_cmr1);
        Zcmr1 = (*mpND_Zcmr1);
        //Port Pmr2
        tormr2 = (*mpND_tormr2);
        thetamr2 = (*mpND_thetamr2);
        wmr2 = (*mpND_wmr2);
        cmr2 = (*mpND_cmr2);
        Zcmr2 = (*mpND_Zcmr2);

        //Read inputVariables from nodes

        //Read outputVariables from nodes
        vc = mpPvc->getStartValue(NodeSignal::VALUE);
        xc = mpPxc->getStartValue(NodeSignal::VALUE);
        fd = mpPfd->getStartValue(NodeSignal::VALUE);
        fr = mpPfr->getStartValue(NodeSignal::VALUE);


        //LocalExpressions
        double fd = (mCdA*mrho*Power(vc,2))/2.;
        double fc = 9.82*mcfr*mMc;
        double Br = mMc/mTimestep;
        double fre = limit(Br*vc,-fc,fc);

        //Initialize delays
        delayParts1[1] = (fd*mrwheel*mTimestep + fre*mrwheel*mTimestep - \
mTimestep*tormr1 - mTimestep*tormr2 - 2*mMc*mrwheel*vc)/(2.*mMc*mrwheel);
        mDelayedPart11.initialize(mNstep,delayParts1[0]);
        delayParts2[1] = (-(mTimestep*vc) - 2*xc)/2.;
        mDelayedPart21.initialize(mNstep,delayParts2[0]);
        delayParts3[1] = (-2*mrwheel*thetamr1 - mTimestep*vc)/(2.*mrwheel);
        mDelayedPart31.initialize(mNstep,delayParts3[0]);
     }
    void simulateOneTimestep()
     {
        Vec stateVar(5);
        Vec stateVark(5);
        Vec deltaStateVar(5);

        //Read variables from nodes
        //Port Pmr1
        cmr1 = (*mpND_cmr1);
        Zcmr1 = (*mpND_Zcmr1);
        //Port Pmr2
        cmr2 = (*mpND_cmr2);
        Zcmr2 = (*mpND_Zcmr2);

        //Read inputVariables from nodes

        //LocalExpressions
        double fd = (mCdA*mrho*Power(vc,2))/2.;
        double fc = 9.82*mcfr*mMc;
        double Br = mMc/mTimestep;
        double fre = limit(Br*vc,-fc,fc);

        //Initializing variable vector for Newton-Raphson
        stateVark[0] = vc;
        stateVark[1] = xc;
        stateVark[2] = thetamr1;
        stateVark[3] = tormr1;
        stateVark[4] = tormr2;

        //Iterative solution using Newton-Rapshson
        for(iter=1;iter<=mNoiter;iter++)
        {
         //Vehicle1D
         //Differential-algebraic system of equation parts
          delayParts1[1] = (fd*mrwheel*mTimestep + fre*mrwheel*mTimestep - \
mTimestep*tormr1 - mTimestep*tormr2 - 2*mMc*mrwheel*vc)/(2.*mMc*mrwheel);
          delayParts2[1] = (-(mTimestep*vc) - 2*xc)/2.;
          delayParts3[1] = (-2*mrwheel*thetamr1 - mTimestep*vc)/(2.*mrwheel);

          delayedPart[1][1] = delayParts1[1];
          delayedPart[2][1] = delayParts2[1];
          delayedPart[3][1] = delayParts3[1];
          delayedPart[4][1] = delayParts4[1];
          delayedPart[5][1] = delayParts5[1];

          //Assemble differential-algebraic equations
          systemEquations[0] =(mTimestep*(fd*mrwheel + fre*mrwheel - tormr1 - \
tormr2))/(2.*mMc*mrwheel) + vc + delayedPart[1][1];
          systemEquations[1] =-(mTimestep*vc)/2. + xc + delayedPart[2][1];
          systemEquations[2] =thetamr1 - (mTimestep*vc)/(2.*mrwheel) + \
delayedPart[3][1];
          systemEquations[3] =-cmr1 + tormr1 + (vc*Zcmr1)/mrwheel;
          systemEquations[4] =-cmr2 + tormr2 + (vc*Zcmr2)/mrwheel;

          //Jacobian matrix
          jacobianMatrix[0][0] = 1;
          jacobianMatrix[0][1] = 0;
          jacobianMatrix[0][2] = 0;
          jacobianMatrix[0][3] = -mTimestep/(2.*mMc*mrwheel);
          jacobianMatrix[0][4] = -mTimestep/(2.*mMc*mrwheel);
          jacobianMatrix[1][0] = -mTimestep/2.;
          jacobianMatrix[1][1] = 1;
          jacobianMatrix[1][2] = 0;
          jacobianMatrix[1][3] = 0;
          jacobianMatrix[1][4] = 0;
          jacobianMatrix[2][0] = -mTimestep/(2.*mrwheel);
          jacobianMatrix[2][1] = 0;
          jacobianMatrix[2][2] = 1;
          jacobianMatrix[2][3] = 0;
          jacobianMatrix[2][4] = 0;
          jacobianMatrix[3][0] = Zcmr1/mrwheel;
          jacobianMatrix[3][1] = 0;
          jacobianMatrix[3][2] = 0;
          jacobianMatrix[3][3] = 1;
          jacobianMatrix[3][4] = 0;
          jacobianMatrix[4][0] = Zcmr2/mrwheel;
          jacobianMatrix[4][1] = 0;
          jacobianMatrix[4][2] = 0;
          jacobianMatrix[4][3] = 0;
          jacobianMatrix[4][4] = 1;

          //Solving equation using LU-faktorisation
          ludcmp(jacobianMatrix, order);
          solvlu(jacobianMatrix,systemEquations,deltaStateVar,order);

        for(i=0;i<5;i++)
          {
          stateVar[i] = stateVark[i] - 
          jsyseqnweight[iter - 1] * deltaStateVar[i];
          }
        for(i=0;i<5;i++)
          {
          stateVark[i] = stateVar[i];
          }
        }
        vc=stateVark[0];
        xc=stateVark[1];
        thetamr1=stateVark[2];
        tormr1=stateVark[3];
        tormr2=stateVark[4];
        //Expressions
        double wmr1 = -(vc/mrwheel);
        double wmr2 = -(vc/mrwheel);
        double thetamr2 = thetamr1;
        double fr = fre;

        //Write new values to nodes
        //Port Pmr1
        (*mpND_tormr1)=tormr1;
        (*mpND_thetamr1)=thetamr1;
        (*mpND_wmr1)=wmr1;
        //Port Pmr2
        (*mpND_tormr2)=tormr2;
        (*mpND_thetamr2)=thetamr2;
        (*mpND_wmr2)=wmr2;
        //outputVariables
        (*mpND_vc)=vc;
        (*mpND_xc)=xc;
        (*mpND_fd)=fd;
        (*mpND_fr)=fr;

        //Update the delayed variabels
        mDelayedPart11.update(delayParts1[1]);
        mDelayedPart21.update(delayParts2[1]);
        mDelayedPart31.update(delayParts3[1]);

     }
};
#endif // MECHANICVEHICLE1D_HPP_INCLUDED
