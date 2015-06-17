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

#ifndef HYDRAULICCYLDAMP_HPP_INCLUDED
#define HYDRAULICCYLDAMP_HPP_INCLUDED

#include <iostream>
#include "ComponentEssentials.h"
#include "ComponentUtilities.h"
#include "math.h"

//!
//! @file HydraulicCyldamp.hpp
//! @author Petter Krus <petter.krus@liu.se>
//  co-author/auditor **Not yet audited by a second person**
//! @date Tue 5 May 2015 10:30:34
//! @brief A load sensing hydraulic directional valve
//! @ingroup HydraulicComponents
//!
//==This code has been autogenerated using Compgen==
//from 
/*{, C:, HopsanTrunk, componentLibraries, defaultLibrary, Hydraulic, \
Special}/HydraulicCyldamp2.nb*/

using namespace hopsan;

class HydraulicCyldamp : public ComponentQ
{
private:
     double rho;
     double visc;
     double Cdt;
     double del;
     double Ab;
     double Sb;
     Port *mpPp;
     Port *mpPc;
     double delayParts1[9];
     double delayParts2[9];
     double delayParts3[9];
     Matrix jacobianMatrix;
     Vec systemEquations;
     Matrix delayedPart;
     int i;
     int iter;
     int mNoiter;
     double jsyseqnweight[4];
     int order[3];
     int mNstep;
     //Port Pp variable
     double pp;
     double qp;
     double Tp;
     double dEp;
     double cp;
     double Zcp;
     //Port Pc variable
     double pc;
     double qc;
     double Tc;
     double dEc;
     double cc;
     double Zcc;
//==This code has been autogenerated using Compgen==
     //inputVariables
     double Ao;
     double So;
     double lo;
     //outputVariables
     //Expressions variables
     //Port Pp pointer
     double *mpND_pp;
     double *mpND_qp;
     double *mpND_Tp;
     double *mpND_dEp;
     double *mpND_cp;
     double *mpND_Zcp;
     //Port Pc pointer
     double *mpND_pc;
     double *mpND_qc;
     double *mpND_Tc;
     double *mpND_dEc;
     double *mpND_cc;
     double *mpND_Zcc;
     //Delay declarations
//==This code has been autogenerated using Compgen==
     //inputVariables pointers
     double *mpAo;
     double *mpSo;
     double *mplo;
     //inputParameters pointers
     double *mprho;
     double *mpvisc;
     double *mpCdt;
     double *mpdel;
     double *mpAb;
     double *mpSb;
     //outputVariables pointers
     Delay mDelayedPart10;
     EquationSystemSolver *mpSolver;

public:
     static Component *Creator()
     {
        return new HydraulicCyldamp();
     }

     void configure()
     {
//==This code has been autogenerated using Compgen==

        mNstep=9;
        jacobianMatrix.create(3,3);
        systemEquations.create(3);
        delayedPart.create(4,6);
        mNoiter=2;
        jsyseqnweight[0]=1;
        jsyseqnweight[1]=0.67;
        jsyseqnweight[2]=0.5;
        jsyseqnweight[3]=0.5;


        //Add ports to the component
        mpPp=addPowerPort("Pp","NodeHydraulic");
        mpPc=addPowerPort("Pc","NodeHydraulic");
        //Add inputVariables to the component
            addInputVariable("Ao","Otifice area","m2",0.0003,&mpAo);
            addInputVariable("So","Orifice perimeter","m",0.3,&mpSo);
            addInputVariable("lo","Length","m",0.001,&mplo);

        //Add inputParammeters to the component
            addInputVariable("rho", "oil density", "kg/m3", 860.,&mprho);
            addInputVariable("visc", "Dynamic viscosity ", "m", \
0.12,&mpvisc);
            addInputVariable("Cdt", "Turbulent flow coefficient.", "", \
0.67,&mpCdt);
            addInputVariable("del", "Flow coefficient.", "", 0.67,&mpdel);
            addInputVariable("Ab", "back valve area", "m2", 0.0001,&mpAb);
            addInputVariable("Sb", "back valve perimeter", "m2", \
0.0001,&mpSb);
        //Add outputVariables to the component

//==This code has been autogenerated using Compgen==
        //Add constantParameters
        mpSolver = new EquationSystemSolver(this,3);
     }

    void initialize()
     {
        //Read port variable pointers from nodes
        //Port Pp
        mpND_pp=getSafeNodeDataPtr(mpPp, NodeHydraulic::Pressure);
        mpND_qp=getSafeNodeDataPtr(mpPp, NodeHydraulic::Flow);
        mpND_Tp=getSafeNodeDataPtr(mpPp, NodeHydraulic::Temperature);
        mpND_dEp=getSafeNodeDataPtr(mpPp, NodeHydraulic::HeatFlow);
        mpND_cp=getSafeNodeDataPtr(mpPp, NodeHydraulic::WaveVariable);
        mpND_Zcp=getSafeNodeDataPtr(mpPp, NodeHydraulic::CharImpedance);
        //Port Pc
        mpND_pc=getSafeNodeDataPtr(mpPc, NodeHydraulic::Pressure);
        mpND_qc=getSafeNodeDataPtr(mpPc, NodeHydraulic::Flow);
        mpND_Tc=getSafeNodeDataPtr(mpPc, NodeHydraulic::Temperature);
        mpND_dEc=getSafeNodeDataPtr(mpPc, NodeHydraulic::HeatFlow);
        mpND_cc=getSafeNodeDataPtr(mpPc, NodeHydraulic::WaveVariable);
        mpND_Zcc=getSafeNodeDataPtr(mpPc, NodeHydraulic::CharImpedance);

        //Read variables from nodes
        //Port Pp
        pp = (*mpND_pp);
        qp = (*mpND_qp);
        Tp = (*mpND_Tp);
        dEp = (*mpND_dEp);
        cp = (*mpND_cp);
        Zcp = (*mpND_Zcp);
        //Port Pc
        pc = (*mpND_pc);
        qc = (*mpND_qc);
        Tc = (*mpND_Tc);
        dEc = (*mpND_dEc);
        cc = (*mpND_cc);
        Zcc = (*mpND_Zcc);

        //Read inputVariables from nodes
        Ao = (*mpAo);
        So = (*mpSo);
        lo = (*mplo);

        //Read inputParameters from nodes
        rho = (*mprho);
        visc = (*mpvisc);
        Cdt = (*mpCdt);
        del = (*mpdel);
        Ab = (*mpAb);
        Sb = (*mpSb);

        //Read outputVariables from nodes

//==This code has been autogenerated using Compgen==


        //Initialize delays

        delayedPart[1][1] = delayParts1[1];
        delayedPart[2][1] = delayParts2[1];
        delayedPart[3][1] = delayParts3[1];

        simulateOneTimestep();

     }
    void simulateOneTimestep()
     {
        Vec stateVar(3);
        Vec stateVark(3);
        Vec deltaStateVar(3);

        //Read variables from nodes
        //Port Pp
        Tp = (*mpND_Tp);
        cp = (*mpND_cp);
        Zcp = (*mpND_Zcp);
        //Port Pc
        Tc = (*mpND_Tc);
        cc = (*mpND_cc);
        Zcc = (*mpND_Zcc);

        //Read inputVariables from nodes
        Ao = (*mpAo);
        So = (*mpSo);
        lo = (*mplo);

        //Read inputParameters from nodes
        rho = (*mprho);
        visc = (*mpvisc);
        Cdt = (*mpCdt);
        del = (*mpdel);
        Ab = (*mpAb);
        Sb = (*mpSb);

        //LocalExpressions

        //Initializing variable vector for Newton-Raphson
        stateVark[0] = qp;
        stateVark[1] = pp;
        stateVark[2] = pc;

        //Iterative solution using Newton-Rapshson
        for(iter=1;iter<=mNoiter;iter++)
        {
         //Cyldamp
         //Differential-algebraic system of equation parts

          //Assemble differential-algebraic equations
          systemEquations[0] =qp + (8*(-pc + pp)*ifPositive(pc - \
pp,Ao,Ab))/Power((331776*Power(Cdt,4)*Power(del,8)*Power(lo,4)*Power(So,8)*Po\
wer(visc,4) + \
1152*Power(Cdt,4)*Power(del,4)*Power(lo,2)*Power(So,6)*Power(visc,4)*Power(if\
Positive(pc - pp,Ao,Ab),2) + \
Power(Cdt,4)*Power(So,4)*Power(visc,4)*Power(ifPositive(pc - pp,Ao,Ab),4) + \
1024*Power(del,8)*Power(pc - pp,2)*Power(rho,2)*Power(ifPositive(pc - \
pp,Ao,Ab),8))/(Power(Cdt,4)*Power(del,8)*Power(ifPositive(pc - \
pp,Ao,Ab),8)),0.25);
          systemEquations[1] =pp - lowLimit(cp + qp*Zcp,0);
          systemEquations[2] =pc - lowLimit(cc - qp*Zcc,0);

          //Jacobian matrix
          jacobianMatrix[0][0] = 1;
          jacobianMatrix[0][1] = (4096*(pc - pp)*(-pc + \
pp)*Power(rho,2)*ifPositive(pc - \
pp,Ao,Ab))/(Power(Cdt,4)*Power((331776*Power(Cdt,4)*Power(del,8)*Power(lo,4)*\
Power(So,8)*Power(visc,4) + \
1152*Power(Cdt,4)*Power(del,4)*Power(lo,2)*Power(So,6)*Power(visc,4)*Power(if\
Positive(pc - pp,Ao,Ab),2) + \
Power(Cdt,4)*Power(So,4)*Power(visc,4)*Power(ifPositive(pc - pp,Ao,Ab),4) + \
1024*Power(del,8)*Power(pc - pp,2)*Power(rho,2)*Power(ifPositive(pc - \
pp,Ao,Ab),8))/(Power(Cdt,4)*Power(del,8)*Power(ifPositive(pc - \
pp,Ao,Ab),8)),1.25)) + (8*ifPositive(pc - \
pp,Ao,Ab))/Power((331776*Power(Cdt,4)*Power(del,8)*Power(lo,4)*Power(So,8)*Po\
wer(visc,4) + \
1152*Power(Cdt,4)*Power(del,4)*Power(lo,2)*Power(So,6)*Power(visc,4)*Power(if\
Positive(pc - pp,Ao,Ab),2) + \
Power(Cdt,4)*Power(So,4)*Power(visc,4)*Power(ifPositive(pc - pp,Ao,Ab),4) + \
1024*Power(del,8)*Power(pc - pp,2)*Power(rho,2)*Power(ifPositive(pc - \
pp,Ao,Ab),8))/(Power(Cdt,4)*Power(del,8)*Power(ifPositive(pc - \
pp,Ao,Ab),8)),0.25);
          jacobianMatrix[0][2] = (-4096*(pc - pp)*(-pc + \
pp)*Power(rho,2)*ifPositive(pc - \
pp,Ao,Ab))/(Power(Cdt,4)*Power((331776*Power(Cdt,4)*Power(del,8)*Power(lo,4)*\
Power(So,8)*Power(visc,4) + \
1152*Power(Cdt,4)*Power(del,4)*Power(lo,2)*Power(So,6)*Power(visc,4)*Power(if\
Positive(pc - pp,Ao,Ab),2) + \
Power(Cdt,4)*Power(So,4)*Power(visc,4)*Power(ifPositive(pc - pp,Ao,Ab),4) + \
1024*Power(del,8)*Power(pc - pp,2)*Power(rho,2)*Power(ifPositive(pc - \
pp,Ao,Ab),8))/(Power(Cdt,4)*Power(del,8)*Power(ifPositive(pc - \
pp,Ao,Ab),8)),1.25)) - (8*ifPositive(pc - \
pp,Ao,Ab))/Power((331776*Power(Cdt,4)*Power(del,8)*Power(lo,4)*Power(So,8)*Po\
wer(visc,4) + \
1152*Power(Cdt,4)*Power(del,4)*Power(lo,2)*Power(So,6)*Power(visc,4)*Power(if\
Positive(pc - pp,Ao,Ab),2) + \
Power(Cdt,4)*Power(So,4)*Power(visc,4)*Power(ifPositive(pc - pp,Ao,Ab),4) + \
1024*Power(del,8)*Power(pc - pp,2)*Power(rho,2)*Power(ifPositive(pc - \
pp,Ao,Ab),8))/(Power(Cdt,4)*Power(del,8)*Power(ifPositive(pc - \
pp,Ao,Ab),8)),0.25);
          jacobianMatrix[1][0] = -(Zcp*dxLowLimit(cp + qp*Zcp,0));
          jacobianMatrix[1][1] = 1;
          jacobianMatrix[1][2] = 0;
          jacobianMatrix[2][0] = Zcc*dxLowLimit(cc - qp*Zcc,0);
          jacobianMatrix[2][1] = 0;
          jacobianMatrix[2][2] = 1;
//==This code has been autogenerated using Compgen==

          //Solving equation using LU-faktorisation
          mpSolver->solve(jacobianMatrix, systemEquations, stateVark, iter);
          qp=stateVark[0];
          pp=stateVark[1];
          pc=stateVark[2];
          //Expressions
          qc = -qp;
        }

        //Calculate the delayed parts

        delayedPart[1][1] = delayParts1[1];
        delayedPart[2][1] = delayParts2[1];
        delayedPart[3][1] = delayParts3[1];

        //Write new values to nodes
        //Port Pp
        (*mpND_pp)=pp;
        (*mpND_qp)=qp;
        (*mpND_dEp)=dEp;
        //Port Pc
        (*mpND_pc)=pc;
        (*mpND_qc)=qc;
        (*mpND_dEc)=dEc;
        //outputVariables

        //Update the delayed variabels

     }
    void deconfigure()
    {
        delete mpSolver;
    }
};
#endif // HYDRAULICCYLDAMP_HPP_INCLUDED
