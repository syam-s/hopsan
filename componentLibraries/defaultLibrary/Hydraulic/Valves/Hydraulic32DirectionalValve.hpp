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
//! @file   Hydraulic32DirectionalValve.hpp
//! @author Robert Braun <robert.braun@liu.se>
//! @date   2010-12-06
//!
//! @brief Contains a hydraulic directional3/2-valve of Q-type
// $Id$

#ifndef HYDRAULIC32DIRECTIONALVALVE_HPP_INCLUDED
#define HYDRAULIC32DIRECTIONALVALVE_HPP_INCLUDED



#include <iostream>
#include "ComponentEssentials.h"
#include "ComponentUtilities.h"

namespace hopsan {

    //!
    //! @brief Hydraulic directional 3/2-valve of Q-type.
    //! @ingroup HydraulicComponents
    //!
    class Hydraulic32DirectionalValve : public ComponentQ
    {
    private:
        double *mpCq, *mpD, *mpF, *mpXvmax, *mpRho;
        double omegah;
        double deltah;

        double *mpND_pa, *mpND_qa, *mpND_ca, *mpND_Zca, *mpND_pp, *mpND_qp, *mpND_cp, *mpND_Zcp, *mpND_pt, *mpND_qt, *mpND_ct, *mpND_Zct;
        double *mpXvIn, *mpXv;

        SecondOrderTransferFunction filter;
        TurbulentFlowFunction qTurb_pa;
        TurbulentFlowFunction qTurb_at;
        Port *mpPP, *mpPT, *mpPA, *mpPB;

    public:
        static Component *Creator()
        {
            return new Hydraulic32DirectionalValve();
        }

        void configure()
        {
            mpPP = addPowerPort("PP", "NodeHydraulic");
            mpPT = addPowerPort("PT", "NodeHydraulic");
            mpPA = addPowerPort("PA", "NodeHydraulic");

            addOutputVariable("xv", "Spool position", "", 0.0, &mpXv);
            addInputVariable("in", "Desired spool position", "", 0.0, &mpXvIn);
            addInputVariable("C_q", "Flow Coefficient", "-", 0.67, &mpCq);
            addInputVariable("rho", "Oil Density", "kg/m^3", 890, &mpRho);
            addInputVariable("d", "Spool Diameter", "m", 0.01, &mpD);
            addInputVariable("f", "Spool Fraction of the Diameter", "-", 1.0, &mpF);
            addInputVariable("x_vmax", "Maximum Spool Displacement", "m", 0.01, &mpXvmax);

            addConstant("omega_h", "Resonance Frequency", "rad/s", 100.0, omegah);
            addConstant("delta_h", "Damping Factor", "-", 1.0, deltah);
        }


        void initialize()
        {
            mpND_pp = getSafeNodeDataPtr(mpPP, NodeHydraulic::Pressure);
            mpND_qp = getSafeNodeDataPtr(mpPP, NodeHydraulic::Flow);
            mpND_cp = getSafeNodeDataPtr(mpPP, NodeHydraulic::WaveVariable);
            mpND_Zcp = getSafeNodeDataPtr(mpPP, NodeHydraulic::CharImpedance);

            mpND_pt = getSafeNodeDataPtr(mpPT, NodeHydraulic::Pressure);
            mpND_qt = getSafeNodeDataPtr(mpPT, NodeHydraulic::Flow);
            mpND_ct = getSafeNodeDataPtr(mpPT, NodeHydraulic::WaveVariable);
            mpND_Zct = getSafeNodeDataPtr(mpPT, NodeHydraulic::CharImpedance);

            mpND_pa = getSafeNodeDataPtr(mpPA, NodeHydraulic::Pressure);
            mpND_qa = getSafeNodeDataPtr(mpPA, NodeHydraulic::Flow);
            mpND_ca = getSafeNodeDataPtr(mpPA, NodeHydraulic::WaveVariable);
            mpND_Zca = getSafeNodeDataPtr(mpPA, NodeHydraulic::CharImpedance);

            double num[3];// = {1.0, 0.0, 0.0};
            double den[3];// = {1.0, 2.0*deltah/omegah, 1.0/(omegah*omegah)};
            num[0] = 1.0;
            num[1] = 0.0;
            num[2] = 0.0;
            den[0] = 1.0;
            den[1] = 2.0*deltah/omegah;
            den[2] = 1.0/(omegah*omegah);

            filter.initialize(mTimestep, num, den, 0, 0, -(*mpXvmax), (*mpXvmax));
        }


        void simulateOneTimestep()
        {
            //Declare local variables
            double xv, xpanom, xatnom, Kcpa, Kcat, qpa, qat;
            double pa, qa, ca, Zca, pp, qp, cp, Zcp, pt, qt, ct, Zct, xvin;
            double rho, xvmax, Cq, d, f;
            bool cav = false;

            //Get variable values from nodes
            cp = (*mpND_cp);
            Zcp = (*mpND_Zcp);
            ct = (*mpND_ct);
            Zct = (*mpND_Zct);
            ca = (*mpND_ca);
            Zca = (*mpND_Zca);
            xvin = (*mpXvIn);

            rho = (*mpRho);
            xvmax = (*mpXvmax);
            Cq = (*mpCq);
            d = (*mpD);
            f = (*mpF);

            if(doubleToBool(xvin))
            {
                filter.update(xvmax);
            }
            else
            {
                filter.update(-xvmax);
            }

            xv = filter.value();

            xpanom = std::max(xv,0.0);
            xatnom = std::max(-xv,0.0);

            Kcpa = Cq*f*pi*d*xpanom*sqrt(2.0/rho);
            Kcat = Cq*f*pi*d*xatnom*sqrt(2.0/rho);

            //With TurbulentFlowFunction:
            qTurb_pa.setFlowCoefficient(Kcpa);
            qTurb_at.setFlowCoefficient(Kcat);

            qpa = qTurb_pa.getFlow(cp, ca, Zcp, Zca);
            qat = qTurb_at.getFlow(ca, ct, Zca, Zct);

            if (xv >= 0.0)
            {
                qp = -qpa;
                qa = qpa;
                qt = 0;
            }
            else
            {
                qp = 0;
                qa = -qat;
                qt = qat;
            }

            pp = cp + qp*Zcp;
            pa = ca + qa*Zca;
            pt = ct + qt*Zct;

            //Cavitation check
            if(pa < 0.0)
            {
                ca = 0.0;
                Zca = 0;
                cav = true;
            }
            if(pp < 0.0)
            {
                cp = 0.0;
                Zcp = 0;
                cav = true;
            }
            if(pt < 0.0)
            {
                ct = 0.0;
                Zct = 0;
                cav = true;
            }

            if(cav)
            {
                qpa = qTurb_pa.getFlow(cp, ca, Zcp, Zca);
                qat = qTurb_at.getFlow(ca, ct, Zca, Zct);

                if (xv >= 0.0)
                {
                    qp = -qpa;
                    qa = qpa;
                    qt = 0;
                }
                else
                {
                    qp = 0;
                    qa = -qat;
                    qt = qat;
                }

                pp = cp + qp*Zcp;
                pa = ca + qa*Zca;
                pt = ct + qt*Zct;
            }


            //Write new values to nodes

            (*mpND_pp) = pp;
            (*mpND_qp) = qp;
            (*mpND_pa) = pa;
            (*mpND_qa) = qa;
            (*mpND_pt) = pt;
            (*mpND_qt) = qt;
            (*mpXv) = xv;
        }
    };
}

#endif // HYDRAULIC32DIRECTIONALVALVE_HPP_INCLUDED
