//#include "ExternalOrifice.h"

#include "Component.h"
#include "Nodes.h"

extern "C"
{

    class ComponentExternalOrifice : public ComponentQ
    {
    private:
        double mKc;
        enum {P1, P2};

    public:
        static Component *maker()
        {
            return new ComponentExternalOrifice("somename");
        }

        ComponentExternalOrifice(const string name,
                                 const double kc       = 1.0e-11,
                                 const double timestep = 0.001)
            : ComponentQ(name, timestep)
        {
            mKc = kc;

            addPort("P1", "NodeHydraulic", P1);
            addPort("P2", "NodeHydraulic", P2);
            //addMultiPort("P", "NodeHydraulic", 2);

            registerParameter("Tryck-flödeskoeff.", "m^5/Ns", mKc);
        }


        void initialize()
        {
            //Nothing to initialize
        }


        void simulateOneTimestep()
        {
            //Get the nodes
            Node* p1_ptr = mPorts[P1].getNodePtr();
            Node* p2_ptr = mPorts[P2].getNodePtr();

            //Get variable values from nodes
            double c1  = p1_ptr->getData(NodeHydraulic::WAVEVARIABLE);
            double Zc1 = p1_ptr->getData(NodeHydraulic::CHARIMP);
            double c2  = p2_ptr->getData(NodeHydraulic::WAVEVARIABLE);
            double Zc2 = p2_ptr->getData(NodeHydraulic::CHARIMP);

            //Orifice equations
            double q2 = mKc*(c1-c2)/(1+ mKc*(Zc1+Zc2));
            double q1 = -q2;
            double p1 = c1 + q1*Zc1;
            double p2 = c2 + q2*Zc2;

            //Write new values to nodes
            p1_ptr->setData(NodeHydraulic::PRESSURE, p1);
            p1_ptr->setData(NodeHydraulic::MASSFLOW, q1);
            p2_ptr->setData(NodeHydraulic::PRESSURE, p2);
            p2_ptr->setData(NodeHydraulic::MASSFLOW, q2);
        }
    };

    class proxy
    {
    public:
        proxy()
        {
            //factory["shape name"] = maker;
            ComponentFactory::RegisterCreatorFunction("ExternalOrifice", ComponentExternalOrifice::maker);
        }
    };

    proxy p;

}
