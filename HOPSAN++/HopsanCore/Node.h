//! @file   Node.h
//! @author FluMeS
//! @date   2009-12-20
//! @brief Contains Node base classes
//!
//$Id$

#ifndef NODE_H_INCLUDED
#define NODE_H_INCLUDED

#include <vector>
#include <string>
#include "CoreUtilities/ClassFactory.hpp"
#include "win32dll.h"

namespace hopsan {

    typedef std::string NodeTypeT;

    //Forward Declarations
    class Port;
    class Component;
    class ComponentSystem;
    class ConnectionAssistant;
    class HopsanEssentials;

    class DLLIMPORTEXPORT Node
    {
        friend class Port;
        friend class PowerPort;
        friend class WritePort;
        friend class Component;
        friend class ComponentSystem;
        friend class ConnectionAssistant;
        friend class HopsanEssentials;

    public:
        //The user should never bother about Nodes
        void logData(const double time);  //Public because simlation threads must be able to log data
        void setData(const size_t &data_type, const double &data);
        Component *getWritePortComponentPtr();

    protected:
        //Protected member functions
        Node(size_t datalength);
        NodeTypeT &getNodeType();

        enum PLOTORNOT {PLOT, NOPLOT};
        enum INTENSITYORFLOW {INTENSITY, FLOW};

        void copyNodeVariables(Node *pNode);
        virtual void setSpecialStartValues(Node *pNode);

        void setLogSettingsNSamples(size_t nSamples, double start, double stop, double sampletime);
        void setLogSettingsSkipFactor(double factor, double start, double stop, double sampletime);
        void setLogSettingsSampleTime(double log_dt, double start, double stop, double sampletime);
        //void preAllocateLogSpace(const size_t nSlots);
        void preAllocateLogSpace();
        void saveLogData(std::string filename);

        //void setData is now public!
        double getData(const size_t data_type);
        double &getDataRef(const size_t data_type);
        double *getDataPtr(const size_t data_type);

        void setDataCharacteristics(size_t id, std::string name, std::string unit, Node::INTENSITYORFLOW intensityOrFlow, Node::PLOTORNOT plotBehaviour = Node::PLOT);
        std::string getDataName(size_t id);
        std::string getDataUnit(size_t id);
        std::vector<size_t> getIntensityVariableIndexes();
        std::vector<size_t> getFlowVariableIndexes();
        int getDataIdFromName(const std::string name);
        void getDataNamesAndUnits(std::vector<std::string> &rNames, std::vector<std::string> &rUnits);
        void getDataNamesValuesAndUnits(std::vector<std::string> &rNames, std::vector<double> &rValues, std::vector<std::string> &rUnits);
        bool setDataValuesByNames(std::vector<std::string> names, std::vector<double> values);
        int getNumberOfPortsByType(int type);

        ComponentSystem *getOwnerSystem();

        //Protected member variables
        std::vector<double> mDataVector;
        std::vector<Port*> mPortPtrs;

        //WAS: Private member variables, be made them protected to get access in inherented classes
        std::string mName;
        std::vector<std::string> mDataNames;
        std::vector<std::string> mDataUnits;
        std::vector<Node::PLOTORNOT> mPlotBehaviour;
        std::vector<Node::INTENSITYORFLOW> mIntensityOrFlow;
        ComponentSystem *mpOwnerSystem;

    private:
        //Private member fuctions
        void setPort(Port *pPort);
        void removePort(Port *pPort);
        bool isConnectedToPort(Port *pPort);
        void enableLog();
        void disableLog();

        //Private member variables
        NodeTypeT mNodeType;

        //Log specific variables
        std::vector<double> mTimeStorage;
        std::vector<std::vector<double> > mDataStorage;
        bool mLogSpaceAllocated;
        bool mDoLog;
        double mLogTimeDt;
        double mLastLogTime;
        size_t mLogSlots;
        size_t mLogCtr;
    };

    typedef ClassFactory<NodeTypeT, Node> NodeFactory;
//    extern NodeFactory gCoreNodeFactory;
//    DLLIMPORTEXPORT NodeFactory* getCoreNodeFactoryPtr();
}

#endif // NODE_H_INCLUDED
