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
//! @file   GUIContainerObject.cpp
//! @author Flumes <flumes@lists.iei.liu.se>
//! @date   2010-01-01
//!
//! @brief Contains the GUI Container class (base class for Systems and Groups)
//!
//$Id$

#include "GUIContainerObject.h"

#include "../MainWindow.h"
#include "../PlotWindow.h"
#include "../GUIPort.h"
#include "../GUIConnector.h"
#include "../UndoStack.h"
#include "../GraphicsView.h"
#include "../loadObjects.h"
#include "../CoreAccess.h"
#include "../CopyStack.h"
#include "../Widgets/ProjectTabWidget.h"
#include "../Widgets/MessageWidget.h"
#include "../Widgets/LibraryWidget.h"
#include "../Widgets/PlotWidget.h"
#include "../Widgets/SystemParametersWidget.h"
#include "../Utilities/GUIUtilities.h"
#include "GUIComponent.h"
#include "GUIGroup.h"
#include "GUIContainerPort.h"
#include "GUIWidgets.h"
#include "GUISystem.h"

#include <limits>
#include <QDomElement>


//! @brief Construtor for container objects.
//! @param position Initial position where container object is to be placed in its parent container
//! @param rotation Initial rotation of the object
//! @param pAppearanceData Pointer to the appearance data object
//! @param startSelected Tells whether or not the object is initially selected
//! @param gfxType Tells whether the initial graphics shall be user or ISO
//! @param pParentContainer Pointer to the parent container object (leave empty if not a sub container)
//! @param pParent Pointer to parent object
GUIContainerObject::GUIContainerObject(QPointF position, qreal rotation, const GUIModelObjectAppearance* pAppearanceData, selectionStatus startSelected, graphicsType gfxType, GUIContainerObject *pParentContainer, QGraphicsItem *pParent)
        : GUIModelObject(position, rotation, pAppearanceData, startSelected, gfxType, pParentContainer, pParent)
{
        //Initialize
    setIsCreatingConnector(false);
    mPortsHidden = !gpMainWindow->mpTogglePortsAction->isChecked();
    mNamesHidden = !gpMainWindow->mpToggleNamesAction->isChecked();
    mLossesVisible = false;
    mUndoDisabled = false;
    mGfxType = USERGRAPHICS;

    mHighestWidgetIndex = 0;

    mPasteOffset = -30;

    nPlotCurves = 0;

    //Create the scene
    mpScene = new QGraphicsScene(this);

    //Create the undastack
    mpUndoStack = new UndoStack(this);
    mpUndoStack->clear();

    gpMainWindow->mpToggleNamesAction->setChecked(true);
    gpMainWindow->mpTogglePortsAction->setChecked(true);

    mpDragCopyStack = new CopyStack();

    //Establish connections that should always remain
    connect(this, SIGNAL(checkMessages()), gpMainWindow->mpMessageWidget, SLOT(checkMessages()), Qt::UniqueConnection);

}


//! @brief Destructor for container object
GUIContainerObject::~GUIContainerObject()
{
    //qDebug() << ",,,,,,,,,,,,GUIContainer destructor";
}

//! @brief Connects all SignalAndSlot connections to the mainwindow buttons from this container
//! This is useful when we are swithching what continer we want to the buttons to trigger actions in
void GUIContainerObject::connectMainWindowActions()
{
    connect(gpMainWindow->mpUndoAction, SIGNAL(triggered()), this, SLOT(undo()), Qt::UniqueConnection);
    connect(gpMainWindow->mpRedoAction, SIGNAL(triggered()), this, SLOT(redo()), Qt::UniqueConnection);
    connect(gpMainWindow->mpUndoWidget->getUndoButton(), SIGNAL(clicked()), this, SLOT(undo()), Qt::UniqueConnection);
    connect(gpMainWindow->mpUndoWidget->getRedoButton(), SIGNAL(clicked()), this, SLOT(redo()), Qt::UniqueConnection);
    connect(gpMainWindow->mpUndoWidget->getClearButton(), SIGNAL(clicked()), this, SLOT(clearUndo()), Qt::UniqueConnection);

    connect(gpMainWindow->mpTogglePortsAction,    SIGNAL(triggered(bool)),    this,     SLOT(hidePorts(bool)), Qt::UniqueConnection);
    connect(gpMainWindow->mpToggleNamesAction,    SIGNAL(triggered(bool)),    this,     SLOT(toggleNames(bool)), Qt::UniqueConnection);
    connect(gpMainWindow->mpDisableUndoAction,    SIGNAL(triggered(bool)),    this,     SLOT(setUndoDisabled(bool)), Qt::UniqueConnection);
    connect(gpMainWindow->mpCutAction,            SIGNAL(triggered()),        this,     SLOT(cutSelected()), Qt::UniqueConnection);
    connect(gpMainWindow->mpCopyAction,           SIGNAL(triggered()),        this,     SLOT(copySelected()), Qt::UniqueConnection);
    connect(gpMainWindow->mpPasteAction,          SIGNAL(triggered()),        this,     SLOT(paste()), Qt::UniqueConnection);
    connect(gpMainWindow->mpAlignXAction,         SIGNAL(triggered()),        this,     SLOT(alignX()), Qt::UniqueConnection);
    connect(gpMainWindow->mpAlignYAction,         SIGNAL(triggered()),        this,     SLOT(alignY()), Qt::UniqueConnection);
    connect(gpMainWindow->mpRotateRightAction,    SIGNAL(triggered()),        this,     SLOT(rotateSubObjects90cw()), Qt::UniqueConnection);
    connect(gpMainWindow->mpRotateLeftAction,     SIGNAL(triggered()),        this,     SLOT(rotateSubObjects90ccw()), Qt::UniqueConnection);
    connect(gpMainWindow->mpFlipHorizontalAction, SIGNAL(triggered()),        this,     SLOT(flipSubObjectsHorizontal()), Qt::UniqueConnection);
    connect(gpMainWindow->mpFlipVerticalAction,   SIGNAL(triggered()),        this,     SLOT(flipSubObjectsVertical()), Qt::UniqueConnection);
    connect(gpMainWindow->mpPropertiesAction,     SIGNAL(triggered()),        this,     SLOT(openPropertiesDialogSlot()), Qt::UniqueConnection);

    connect(gpMainWindow->getStartTimeLineEdit(), SIGNAL(editingFinished()),  this,     SLOT(updateStartTime()), Qt::UniqueConnection);//! @todo should these be here (start stop ts)?  and duplicates?
    connect(gpMainWindow->getTimeStepLineEdit(),  SIGNAL(editingFinished()),  this,     SLOT(updateTimeStep()), Qt::UniqueConnection);
    connect(gpMainWindow->getFinishTimeLineEdit(),SIGNAL(editingFinished()),  this,     SLOT(updateStopTime()), Qt::UniqueConnection);
}

//! @brief Disconnects all SignalAndSlot connections to the mainwindow buttons from this container
//! This is useful when we are swithching what continer we want to the buttons to trigger actions in
void GUIContainerObject::disconnectMainWindowActions()
{
    disconnect(gpMainWindow->mpUndoAction, SIGNAL(triggered()), this, SLOT(undo()));
    disconnect(gpMainWindow->mpRedoAction, SIGNAL(triggered()), this, SLOT(redo()));
    disconnect(gpMainWindow->mpUndoWidget->getUndoButton(), SIGNAL(clicked()), this, SLOT(undo()));
    disconnect(gpMainWindow->mpUndoWidget->getRedoButton(), SIGNAL(clicked()), this, SLOT(redo()));
    disconnect(gpMainWindow->mpUndoWidget->getClearButton(), SIGNAL(clicked()), this, SLOT(clearUndo()));

    disconnect(gpMainWindow->mpToggleNamesAction,     SIGNAL(triggered(bool)),    this,      SLOT(toggleNames(bool)));
    disconnect(gpMainWindow->mpTogglePortsAction,     SIGNAL(triggered(bool)),    this,     SLOT(hidePorts(bool)));
    disconnect(gpMainWindow->mpDisableUndoAction,     SIGNAL(triggered(bool)),    this,    SLOT(setUndoDisabled(bool)));
    disconnect(gpMainWindow->mpCutAction,             SIGNAL(triggered()),        this,    SLOT(cutSelected()));
    disconnect(gpMainWindow->mpCopyAction,            SIGNAL(triggered()),        this,    SLOT(copySelected()));
    disconnect(gpMainWindow->mpPasteAction,           SIGNAL(triggered()),        this,    SLOT(paste()));
    disconnect(gpMainWindow->mpAlignXAction,          SIGNAL(triggered()),        this,    SLOT(alignX()));
    disconnect(gpMainWindow->mpAlignYAction,          SIGNAL(triggered()),        this,    SLOT(alignY()));
    disconnect(gpMainWindow->mpRotateRightAction,     SIGNAL(triggered()),        this,    SLOT(rotateSubObjects90cw()));
    disconnect(gpMainWindow->mpRotateLeftAction,      SIGNAL(triggered()),        this,    SLOT(rotateSubObjects90ccw()));
    disconnect(gpMainWindow->mpFlipHorizontalAction,  SIGNAL(triggered()),        this,    SLOT(flipSubObjectsHorizontal()));
    disconnect(gpMainWindow->mpFlipVerticalAction,    SIGNAL(triggered()),        this,    SLOT(flipSubObjectsVertical()));
    disconnect(gpMainWindow->mpPropertiesAction,      SIGNAL(triggered()),        this,    SLOT(openPropertiesDialogSlot()));

    disconnect(gpMainWindow->getStartTimeLineEdit(),   SIGNAL(editingFinished()),  this,    SLOT(updateStartTime()));//! @todo should these be here (start stop ts)? and duplicates?
    disconnect(gpMainWindow->getTimeStepLineEdit(),    SIGNAL(editingFinished()),  this,    SLOT(updateTimeStep()));
    disconnect(gpMainWindow->getFinishTimeLineEdit(),  SIGNAL(editingFinished()),  this,    SLOT(updateStopTime()));
}

//! @brief A helpfunction that determines on which edge an external port should be placed based on its internal position
//! @param[in] center The center point of all objects to be compared with
//! @param[in] pt The position of this object, used to determine the center relative posistion
//! @returns An enum that indicates on which side the port should be placed
GUIContainerObject::ContainerEdgeT GUIContainerObject::findPortEdge(QPointF center, QPointF pt)
{
    //By swapping place of pt1 and pt2 we get the angle in the same coordinate system as the view
    QPointF diff = pt-center;
    ////qDebug() << "=============The Diff: " << diff;

    //If only one sysport default to left side
    //! @todo Do this smarter later and take into account port orientation, or position relative all other components, need to extend this function a bit for that though
    if (diff.manhattanLength() < 1.0)
    {
        return LEFTEDGE;
    }

    //Determine on what edge the port should be placed based on the angle from the center point
    qreal angle = normRad(qAtan2(diff.x(), diff.y()));
    ////qDebug() << "angle: " << rad2deg(angle);
    if (fabs(angle) <= M_PI_4)
    {
        return RIGHTEDGE;
    }
    else if (fabs(angle) >= 3.0*M_PI_4)
    {
        return LEFTEDGE;
    }
    else if (angle > M_PI_4)
    {
        return BOTTOMEDGE;
    }
    else
    {
        return TOPEDGE;
    }
}

//! @brief Refreshes the appearance and postion of all external ports
void GUIContainerObject::refreshExternalPortsAppearanceAndPosition()
{
    //refresh the external port poses
    GUIModelObjectMapT::iterator moit;
    double val;

    //Set the initial values to be overwriten by the if bellow
    double xMin=std::numeric_limits<double>::max(), xMax=-xMin, yMin=xMin, yMax=xMax;
    for(moit = mGUIModelObjectMap.begin(); moit != mGUIModelObjectMap.end(); ++moit)
    {
        //if(moit.value()->type() == GUICONTAINERPORT)
        //{
            //check x max and min
            val = moit.value()->getCenterPos().x();
            xMin = std::min(xMin,val);
            xMax = std::max(xMax,val);
            //check y max and min
            val = moit.value()->getCenterPos().y();
            yMin = std::min(yMin,val);
            yMax = std::max(yMax,val);
        //}
    }
    //! @todo Find out if it is possible to ask the scene or view for this information instead of calulating it ourselves
    QPointF center = QPointF((xMax+xMin)/2.0, (yMax+yMin)/2.0);
    ////qDebug() << "center max min: " << center << " " << xMin << " " << xMax << " " << yMin << " " << yMax;

    QMap<qreal, GUIPort*> leftEdge, rightEdge, topEdge, bottomEdge;
    for(moit = mGUIModelObjectMap.begin(); moit != mGUIModelObjectMap.end(); ++moit)
    {
        if(moit.value()->type() == GUICONTAINERPORT)
        {
            //            QLineF line = QLineF(center, moit.value()->getCenterPos());
            //            this->getContainedScenePtr()->addLine(line); //debug-grej

            ContainerEdgeT edge = findPortEdge(center, moit.value()->getCenterPos());
            ////qDebug() << " sysp: " << moit.value()->getName() << " edge: " << edge;

            //Make sure we dont screw up in the code and forget to rename or create external ports on internal rename or create
            assert(this->getPort(moit.value()->getName()) != 0);

            //We insert into maps for automatic sorting based on x or y position as key value
            switch (edge) {
            case RIGHTEDGE:
                rightEdge.insertMulti(moit.value()->getCenterPos().y(), this->getPort(moit.value()->getName()));
                break;
            case BOTTOMEDGE:
                bottomEdge.insertMulti(moit.value()->getCenterPos().x(),this->getPort(moit.value()->getName()));
                break;
            case LEFTEDGE:
                leftEdge.insertMulti(moit.value()->getCenterPos().y(), this->getPort(moit.value()->getName()));
                break;
            case TOPEDGE:
                topEdge.insertMulti(moit.value()->getCenterPos().x(), this->getPort(moit.value()->getName()));
                break;
            }
        }
    }

    //Now disperse the port icons evenly along each edge
    QMap<qreal, GUIPort*>::iterator it;
    qreal disp;  //Dispersion factor
    qreal sdisp; //sumofdispersionfactors

    //! @todo maybe we should be able to update rotation in all of these also
    //! @todo weird to use createfunction to refresh graphics, but ok for now
    disp = 1.0/((qreal)(rightEdge.size()+1));
    sdisp=disp;
    for (it=rightEdge.begin(); it!=rightEdge.end(); ++it)
    {
        it.value()->setCenterPosByFraction(1.0, sdisp);
        it.value()->setRotation(0);
        this->createExternalPort(it.value()->getPortName());    //refresh the external port graphics
        sdisp += disp;
    }

    disp = 1.0/((qreal)(bottomEdge.size()+1));
    sdisp=disp;
    for (it=bottomEdge.begin(); it!=bottomEdge.end(); ++it)
    {
        it.value()->setCenterPosByFraction(sdisp, 1.0);
        it.value()->setRotation(90);
        this->createExternalPort(it.value()->getPortName());    //refresh the external port graphics
        sdisp += disp;
    }

    disp = 1.0/((qreal)(leftEdge.size()+1));
    sdisp=disp;
    for (it=leftEdge.begin(); it!=leftEdge.end(); ++it)
    {
        it.value()->setCenterPosByFraction(0.0, sdisp);
        it.value()->setRotation(180);
        this->createExternalPort(it.value()->getPortName());    //refresh the external port graphics
        sdisp += disp;
    }

    disp = 1.0/((qreal)(topEdge.size()+1));
    sdisp=disp;
    for (it=topEdge.begin(); it!=topEdge.end(); ++it)
    {
        it.value()->setCenterPosByFraction(sdisp, 0.0);
        it.value()->setRotation(270);
        this->createExternalPort(it.value()->getPortName());    //refresh the external port graphics
        sdisp += disp;
    }
}

//! @brief Overloaded refreshAppearance for containers, to make sure that port positions are updeted if graphics size is changed
void GUIContainerObject::refreshAppearance()
{
    GUIModelObject::refreshAppearance();
    this->refreshExternalPortsAppearanceAndPosition();
}

//! @brief Use this function to calculate the placement of the ports on a subsystem icon.
//! @param[in] w width of the subsystem icon
//! @param[in] h heigth of the subsystem icon
//! @param[in] angle the angle in radians of the line between center and the actual port
//! @param[out] x the new calculated horizontal placement for the port
//! @param[out] y the new calculated vertical placement for the port
//! @todo rename this one and maybe change it a bit as it is now included in this class, it should be common for subsystems and groups
void GUIContainerObject::calcSubsystemPortPosition(const double w, const double h, const double angle, double &x, double &y)
{
    //! @todo make common PI declaration, maybe also PIhalf or include math.h and use M_PI
    double tanAngle = tan(angle);//Otherwise division by zero
    if(fabs(tanAngle) < 0.0001)
    {
        tanAngle=.0001;
    }
    if(angle>3.1415*3.0/2.0)
    {
        x=-std::max(std::min(h/tanAngle, w), -w);
        y=std::max(std::min(w*tanAngle, h), -h);
    }
    else if(angle>3.1415)
    {
        x=-std::max(std::min(h/tanAngle, w), -w);
        y=-std::max(std::min(w*tanAngle, h), -h);
    }
    else if(angle>3.1415/2.0)
    {
        x=std::max(std::min(h/tanAngle, w), -w);
        y=-std::max(std::min(w*tanAngle, h), -h);
    }
    else
    {
        x=std::max(std::min(h/tanAngle, w), -w);
        y=std::max(std::min(w*tanAngle, h), -h);
    }
}


//! @brief Returns a pointer to the CoreSystemAccess that this container represents
//! @returns Pointer the the CoreSystemAccess that this container represents
CoreSystemAccess *GUIContainerObject::getCoreSystemAccessPtr()
{
    //Should be overloaded
    return 0;
}


//! @brief Retunrs a pointer to the contained scene
QGraphicsScene *GUIContainerObject::getContainedScenePtr()
{
    return this->mpScene;
}


void GUIContainerObject::createPorts()
{
    //! @todo maybe try to make this function the same as refreshExternal.... and have one common function in modelobject, component and containerports class,
    //This one should not be used in this class only for component and containerport
    assert(false);
}


//! @brief This method creates ONE external port. Or refreshes existing ports. It assumes that port appearance information for this port exists
//! @param[portName] The name of the port to create
//! @todo maybe defualt create that info if it is missing
void GUIContainerObject::createExternalPort(QString portName)
{
    //If port appearance is not already existing then we create it
    if ( mGUIModelObjectAppearance.getPortAppearanceMap().count(portName) == 0 )
    {
        mGUIModelObjectAppearance.addPortAppearance(portName);
    }

    //Fetch appearance data
    PortAppearanceMapT::iterator it = mGUIModelObjectAppearance.getPortAppearanceMap().find(portName);
    if (it != mGUIModelObjectAppearance.getPortAppearanceMap().end())
    {
        //Create new external port if if does not already exist (this is the usual case for individual components)
        GUIPort *pPort = this->getPort(it.key());
        if ( pPort == 0 )
        {
            //qDebug() << "##This is OK though as this means that we should create the stupid port for the first time";

            //! @todo to minimaze search time make a get porttype  and nodetype function, we need to search twice now
            QString nodeType = this->getCoreSystemAccessPtr()->getNodeType(it.key(), it.key());
            QString portType = this->getCoreSystemAccessPtr()->getPortType(it.key(), it.key());
            it.value().selectPortIcon(getTypeCQS(), portType, nodeType);

            qreal x = it.value().x;
            qreal y = it.value().y;
            //qDebug() << "x,y: " << x << " " << y;

            if (this->type() == GUIGROUP)
            {
                pPort = new GroupPort(it.key(), x*boundingRect().width(), y*boundingRect().height(), &(it.value()), this);
            }
            else
            {
                pPort = new GUIPort(it.key(), x*boundingRect().width(), y*boundingRect().height(), &(it.value()), this);
            }


            mPortListPtrs.append(pPort);
        }
        else
        {

            //The external port already seems to exist, lets update it incase something has changed
            //! @todo Maybe need to have a refresh portappearance function, dont really know if this will ever be used though, will fix when it becomes necessary
            pPort->refreshPortGraphics();
            //qDebug() << "--------------------------ExternalPort already exist refreshing its graphics: " << it.key() << " in: " << this->getName();
        }
    }
    else
    {
        //This should never happen
        //qDebug() << "Could not find portappearance info for port: " << portName << " in: " << this->getName();
        assert(false);
    }
}


//! @breif Removes an external Port from a container object
//! @param[in] portName The name of the port to be removed
//! @todo maybe we should use a map instead to make delete more efficient, (may not amtter usually not htat many external ports)
void GUIContainerObject::removeExternalPort(QString portName)
{
    ////qDebug() << "mPortListPtrs.size(): " << mPortListPtrs.size();
    QList<GUIPort*>::iterator plit;
    for (plit=mPortListPtrs.begin(); plit!=mPortListPtrs.end(); ++plit)
    {
        if ((*plit)->getPortName() == portName )
        {
            //Delete the GUIPort its post in the portlist and its appearance data
            mGUIModelObjectAppearance.erasePortAppearance(portName);
            delete *plit;
            mPortListPtrs.erase(plit);
            break;
        }
    }
    ////qDebug() << "mPortListPtrs.size(): " << mPortListPtrs.size();
}


//! @brief Reanmes an external GUIPort
//! @param[in] oldName The name to be replaced
//! @param[in] newName The new name
//! This function assumes that oldName exist and that newName is correct, no error checking is done
void GUIContainerObject::renameExternalPort(const QString oldName, const QString newName)
{
    QList<GUIPort*>::iterator plit;
    for (plit=mPortListPtrs.begin(); plit!=mPortListPtrs.end(); ++plit)
    {
        if ((*plit)->getPortName() == oldName )
        {
            //Rename the port appearance data by remove and re-add
            GUIPortAppearance tmp = mGUIModelObjectAppearance.getPortAppearanceMap().value(oldName);
            mGUIModelObjectAppearance.erasePortAppearance(oldName);
            mGUIModelObjectAppearance.addPortAppearance(newName, &tmp);

            //Rename port
            (*plit)->setDisplayName(newName);
            break;
        }
    }
}


//! @brief Helper function that allows calling addGUIModelObject with typeName instead of appearance data
//! @todo Remove the other function and use only the typename version if possible
GUIModelObject* GUIContainerObject::addGUIModelObject(QString typeName, QPointF position, qreal rotation, selectionStatus startSelected, nameVisibility nameStatus, undoStatus undoSettings)
{
    GUIModelObjectAppearance *pAppearanceData = gpMainWindow->mpLibrary->getAppearanceData(typeName);
    if(!pAppearanceData)    //Not an existing component
        return 0;       //No error message here, it depends on from where this function is called
    else
        return addGUIModelObject(pAppearanceData, position, rotation, startSelected, nameStatus, undoSettings);
}


//! @brief Creates and adds a GuiModel Object to the current container
//! @param componentType is a string defining the type of component.
//! @param position is the position where the component will be created.
//! @param name will be the name of the component.
//! @returns a pointer to the created and added object
//! @todo only modelobjects for now
GUIModelObject* GUIContainerObject::addGUIModelObject(GUIModelObjectAppearance *pAppearanceData, QPointF position, qreal rotation, selectionStatus startSelected, nameVisibility nameStatus, undoStatus undoSettings)
{
        //Deselect all other components and connectors
    emit deselectAllGUIObjects();
    emit deselectAllGUIConnectors();

    QString componentTypeName = pAppearanceData->getTypeName();
    if (componentTypeName == HOPSANGUISYSTEMTYPENAME)
    {
        mpTempGUIModelObject= new GUISystem(position, rotation, pAppearanceData, this, startSelected, mGfxType);
    }
    else if (componentTypeName == HOPSANGUICONTAINERPORTTYPENAME)
    {
        mpTempGUIModelObject = new GUIContainerPort(position, rotation, pAppearanceData, this, startSelected, mGfxType);
        this->createExternalPort(mpTempGUIModelObject->getName());
        this->refreshExternalPortsAppearanceAndPosition();
    }
    else if (componentTypeName == HOPSANGUIGROUPTYPENAME)
    {
        mpTempGUIModelObject = new GUIGroup(position, rotation, pAppearanceData, this);
    }
    else //Assume some standard component type
    {
        mpTempGUIModelObject = new GUIComponent(position, rotation, pAppearanceData, this, startSelected, mGfxType);
    }

    emit checkMessages();

    if ( mGUIModelObjectMap.contains(mpTempGUIModelObject->getName()) )
    {
        gpMainWindow->mpMessageWidget->printGUIErrorMessage("Trying to add component with name: " + mpTempGUIModelObject->getName() + " that already exist in GUIObjectMap, (Not adding)");
        //! @todo Is this check really necessary? Two objects cannot have the same name anyway...
    }
    else
    {
        mGUIModelObjectMap.insert(mpTempGUIModelObject->getName(), mpTempGUIModelObject);
    }

    if(undoSettings == UNDO)
    {
        mpUndoStack->registerAddedObject(mpTempGUIModelObject);
    }

    mpTempGUIModelObject->setSelected(false);
    mpTempGUIModelObject->setSelected(true);

    return mpTempGUIModelObject;
}


//! @brief Returns a list with the favorite plot parameters.
QList<QStringList> GUIContainerObject::getFavoriteVariables()
{
    return mFavoriteVariables;
}


//! @brief Defines a new favorite plot variable
//! @param componentName Name of the component where the parameter is located
//! @param portName Name of the port where the parameter is located
//! @param dataName Name of the parameter
//! @param dataUnit Unit of the parameter
void GUIContainerObject::setFavoriteVariable(QString componentName, QString portName, QString dataName, QString dataUnit)
{
    QStringList tempParameter;
    tempParameter.append(componentName);
    tempParameter.append(portName);
    tempParameter.append(dataName);
    tempParameter.append(dataUnit);
    if(!mFavoriteVariables.contains(tempParameter))
    {
        mFavoriteVariables.append(tempParameter);
    }
    gpMainWindow->mpPlotWidget->mpPlotParameterTree->updateList();

    mpParentProjectTab->hasChanged();
}


//! @brief Removes all favorite variables which belongs to the specified component.
//! @param componentName Name of the component
void GUIContainerObject::removeFavoriteVariableByComponentName(QString componentName)
{
    QList<QStringList>::iterator it;
    for(it=mFavoriteVariables.begin(); it!=mFavoriteVariables.end(); ++it)
    {
        if((*it).at(0) == componentName)
        {
            mFavoriteVariables.removeAll((*it));
            gpMainWindow->mpPlotWidget->mpPlotParameterTree->updateList();
            return;
        }
    }
}


bool GUIContainerObject::areLossesVisible()
{
    return mLossesVisible;
}


GUITextBoxWidget *GUIContainerObject::addTextBoxWidget(QPointF position, undoStatus undoSettings)
{
    GUITextBoxWidget *pTempTextBoxWidget;
    pTempTextBoxWidget = new GUITextBoxWidget("Text", position, 0, DESELECTED, this, mHighestWidgetIndex);
    //qDebug() << "Creating widget, index = " << pTempTextBoxWidget->getWidgetIndex();
    mWidgetMap.insert(mHighestWidgetIndex, pTempTextBoxWidget);
    //qDebug() << "Inserting widget in map, index = " << mHighestWidgetIndex;
    ++mHighestWidgetIndex;
    if(undoSettings == UNDO)
    {
        mpUndoStack->registerAddedWidget(pTempTextBoxWidget);
    }
    mpParentProjectTab->hasChanged();

    return pTempTextBoxWidget;
}


//! @brief Removes specified widget
//! Works for both text and box widgets
//! @param pWidget Pointer to widget to remove
//! @param undoSettings Tells whether or not this shall be registered in undo stack
void GUIContainerObject::removeWidget(GUIWidget *pWidget, undoStatus undoSettings)
{
    if(undoSettings == UNDO)
    {
        mpUndoStack->newPost();
        mpUndoStack->registerDeletedWidget(pWidget);
    }

    mSelectedGUIWidgetsList.removeAll(pWidget);
    mWidgetMap.remove(pWidget->getWidgetIndex());
    delete(pWidget);
}


//! @brief Delete GUIObject with specified name
//! @param objectName is the name of the componenet to delete
void GUIContainerObject::deleteGUIModelObject(QString objectName, undoStatus undoSettings)
{
    ////qDebug() << "deleteGUIModelObject(): " << objectName << " in: " << this->getName() << " coresysname: " << this->getCoreSystemAccessPtr()->getRootSystemName() ;
    this->removeFavoriteVariableByComponentName(objectName);   //Does nothing unless this is a system

    GUIModelObjectMapT::iterator it = mGUIModelObjectMap.find(objectName);
    GUIModelObject* obj_ptr = it.value();

        //Remove connectors that are connected to the model object
    QList<GUIConnector *> pConnectorList = obj_ptr->getGUIConnectorPtrs();
    for(int i=0; i<pConnectorList.size(); ++i)
    {
        this->removeSubConnector(pConnectorList[i], undoSettings);
    }

    if (undoSettings == UNDO && !mUndoDisabled)
    {
        //Register removal of model object in undo stack
        this->mpUndoStack->registerDeletedObject(it.value());
        //emit componentChanged(); //!< @todo Why do we need to emit this signal after regestering in undostack
        ////qDebug() << "Emitting!";
    }


    if (it != mGUIModelObjectMap.end())
    {
        //! @todo maybe this should be handled somwhere else (not sure maybe this is the best place)
        if ((*it)->type() == GUICONTAINERPORT )
        {
            this->removeExternalPort((*it)->getName());
        }

        mGUIModelObjectMap.erase(it);
        mSelectedGUIModelObjectsList.removeOne(obj_ptr);
        mpScene->removeItem(obj_ptr);
        delete(obj_ptr);
    }
    else
    {
        ////qDebug() << "In delete GUIObject: could not find object with name " << objectName;
        gpMainWindow->mpMessageWidget->printGUIErrorMessage("Error: Could not delete object with name " + objectName + ", object not found");
    }
    emit checkMessages();
    mpParentProjectTab->getGraphicsView()->updateViewPort();
}


//! @brief This function is used to rename a SubGUIObject
void GUIContainerObject::renameGUIModelObject(QString oldName, QString newName, undoStatus undoSettings)
{
    //Avoid work if no change is requested
    if (oldName != newName)
    {
        QString modNewName;
            //First find record with old name
        GUIModelObjectMapT::iterator it = mGUIModelObjectMap.find(oldName);
        if (it != mGUIModelObjectMap.end())
        {
                //Make a backup copy
            GUIModelObject* obj_ptr = it.value();
                //Erase old record
            mGUIModelObjectMap.erase(it);
                //Set new name, first in core then in gui object
            ////qDebug() << "Renaming: " << oldName << " " << newName << " type: " << obj_ptr->type();
            switch (obj_ptr->type())
            {
            case GUICOMPONENT:
                ////qDebug() << "GUICOMPONENT";
            case GUISYSTEM :
                ////qDebug() << "GUISYSTEM";
                modNewName = this->getCoreSystemAccessPtr()->renameSubComponent(oldName, newName);
                break;
            case GUICONTAINERPORT : //!< @todo What will happen when we try to rename a groupport
                ////qDebug() << "GUISYSTEMPORT";
                modNewName = this->getCoreSystemAccessPtr()->renameSystemPort(oldName, newName);
                renameExternalPort(oldName, modNewName);
                break;
            //default :
                ////qDebug() << "default";
                    //No Core rename action
            }
            ////qDebug() << "modNewName: " << modNewName;
            obj_ptr->setDisplayName(modNewName);
                //Re insert
            mGUIModelObjectMap.insert(obj_ptr->getName(), obj_ptr);
        }
        else
        {
            ////qDebug() << "Old name: " << oldName << " not found";
            //! @todo Maybe we should give the user a message?
        }

        for(int i=0; i<mPlotData.size(); ++i)
        {
            if(mPlotData.at(i).contains(oldName))
            {
                QMap< QString, QMap<QString, QMap<QString, QPair<QVector<double>, QVector<double> > > > > generation;
                generation = mPlotData.at(i);
                QMap< QString, QMap<QString, QPair<QVector<double>, QVector<double> > > > oldPlotData;
                oldPlotData = mPlotData.at(i).find(oldName).value();
                generation.insert(newName, oldPlotData);
                generation.remove(oldName);
                mPlotData.removeAt(i);
                mPlotData.insert(i, generation);
            }
        }

        if (undoSettings == UNDO)
        {
            mpUndoStack->newPost();
            mpUndoStack->registerRenameObject(oldName, modNewName);
            emit componentChanged();
        }
    }
    emit checkMessages();
}


//! @brief Tells whether or not a component with specified name exist in the GraphicsView
bool GUIContainerObject::hasGUIModelObject(QString name)
{
    return (mGUIModelObjectMap.count(name) > 0);
}

//! @brief Takes ownership of supplied objects, widgets and connectors
//!
//! This method assumes that the previous owner have forgotten all about these objects, it however sets iself as new Qtparent, parentContainer and scene, overwriting the old values
void GUIContainerObject::takeOwnershipOf(QList<GUIModelObject*> &rModelObjectList, QList<GUIWidget*> &rWidgetList)
{
    for (int i=0; i<rModelObjectList.size(); ++i)
    {
        //! @todo if a containerport is received we must update the external port list also, we cant handle such objects right now
        if (rModelObjectList[i]->type() != GUICONTAINERPORT)
        {
            this->getContainedScenePtr()->addItem(rModelObjectList[i]);
            rModelObjectList[i]->setParentContainerObject(this);
            mGUIModelObjectMap.insert(rModelObjectList[i]->getName(), rModelObjectList[i]);
            //! @todo what if name already taken, dont care for now as we shal only move into groups when they are created

            //rModelObjectList[i]->refreshParentContainerSigSlotConnections();
        }
        else
        {
            //We cant handle this yet
            assert(false);
        }
    }

    for (int i=0; i<rWidgetList.size(); ++i)
    {
        this->getContainedScenePtr()->addItem(rWidgetList[i]);
        rWidgetList[i]->setParentContainerObject(this);
        mWidgetMap.insert(rWidgetList[i]->getWidgetIndex(), rWidgetList[i]);
        //! @todo what if idx already taken, dont care for now as we shal only move into groups when they are created
    }

    QList<GUIConnector*> transitConnectors;
    QList<GUIConnector*> internalConnectors;

    //Determine what connectors are transitconnectors
    for (int i=0; i<rModelObjectList.size(); ++i)
    {
        GUIModelObject *pObj = rModelObjectList[i];

        QList<GUIConnector*> connectorPtrs = pObj->getGUIConnectorPtrs();
        for(int i=0; i<connectorPtrs.size(); ++i)
        {
            if((rModelObjectList.contains(connectorPtrs[i]->getStartPort()->getGuiModelObject())) &&
               (rModelObjectList.contains(connectorPtrs[i]->getEndPort()->getGuiModelObject())))
            {
                //This seems to be an internal connector, add to internal connector list, (if it has not already been added)
                if (!internalConnectors.contains(connectorPtrs[i]))
                {
                    internalConnectors.append(connectorPtrs[i]);
                }

            }
            else
            {
                //This seems to be a connector that connects to an external modelobject, lets create a transit connector
                if (!transitConnectors.contains(connectorPtrs[i]))
                {
                    transitConnectors.append(connectorPtrs[i]);

                    //! @todo for now we dissconnect the transit connection as we are noy yet capable of recreating the external connection
                    this->getCoreSystemAccessPtr()->disconnect(connectorPtrs[i]->getStartComponentName(),
                                                               connectorPtrs[i]->getStartPortName(),
                                                               connectorPtrs[i]->getEndComponentName(),
                                                               connectorPtrs[i]->getEndPortName());
                }
            }
        }
    }

    //! @todo move this code up into loop above
    //Add the internal connectors
    for (int i=0; i<internalConnectors.size(); ++i)
    {
        //qDebug() << "___Adding internalConnection";
        //Make previous parent container forget about the connector
        internalConnectors[i]->getParentContainer()->forgetSubConnector(internalConnectors[i]);

        //Make new container own and know about the connector
        this->getContainedScenePtr()->addItem(internalConnectors[i]);
        internalConnectors[i]->setParentContainer(this);
        mSubConnectorList.append(internalConnectors[i]);
    }

    //Add the transit connectors and create group ports
    for (int i=0; i<transitConnectors.size(); ++i)
    {
        //qDebug() << "___Adding transitConnection";
        QPointF portpos;
        bool endPortIsTransitPort = false;
        if (rModelObjectList.contains(transitConnectors[i]->getStartPort()->getGuiModelObject()))
        {
            portpos = transitConnectors[i]->getEndPort()->getGuiModelObject()->getCenterPos();
            //Make end object forget about this connector as we are actually splitting it into two new connectors
            transitConnectors[i]->getEndPort()->getGuiModelObject()->forgetConnector(transitConnectors[i]);
            endPortIsTransitPort = true;
        }
        else
        {
            portpos = transitConnectors[i]->getStartPort()->getGuiModelObject()->getCenterPos();
            //Make start object forget about this connector as we are actually splitting it into two new connectors
            transitConnectors[i]->getStartPort()->getGuiModelObject()->forgetConnector(transitConnectors[i]);
        }

        //Create the "transit port"
        GUIModelObject *pTransPort = this->addGUIModelObject(HOPSANGUICONTAINERPORTTYPENAME, portpos.toPoint(),0);

        //Make previous parent container forget about the connector
        transitConnectors[i]->getParentContainer()->forgetSubConnector(transitConnectors[i]);

        //Add the port to this container
        this->getContainedScenePtr()->addItem(transitConnectors[i]);
        transitConnectors[i]->setParentContainer(this);
        mSubConnectorList.append(transitConnectors[i]);

        //! @todo instead of having set startport and set end port, (we can keep them also maybe), we should have a function that sets startport if no port is set and end port if start port already set, dont know if good idea but we can keep it in mind, then we would not have to do stuff like bellow. (maybe we could call that function "connect")
        if (endPortIsTransitPort)
        {
            //Make new port and connector know about eachother
            transitConnectors[i]->setEndPort(pTransPort->getPortListPtrs().at(0));
            transitConnectors[i]->getEndPort()->getGuiModelObject()->rememberConnector(transitConnectors[i]);
        }
        else
        {
            //Make new port and connector know about eachother
            transitConnectors[i]->setStartPort(pTransPort->getPortListPtrs().at(0));
            transitConnectors[i]->getStartPort()->getGuiModelObject()->rememberConnector(transitConnectors[i]);
        }
    }

    //! @todo do much more stuff

    //! @todo center the objects in the new view

}


//! @brief Notifies container object that a gui widget has been selected
void GUIContainerObject::rememberSelectedWidget(GUIWidget *widget)
{
    mSelectedGUIWidgetsList.append(widget);
}


//! @brief Notifies container object that a gui widget is no longer selected
void GUIContainerObject::forgetSelectedWidget(GUIWidget *widget)
{
    mSelectedGUIWidgetsList.removeAll(widget);
}


//! @brief Returns a list with pointers to the selected GUI widgets
QList<GUIWidget *> GUIContainerObject::getSelectedGUIWidgetPtrs()
{
    return mSelectedGUIWidgetsList;
}


//! @brief Notifies container object that a gui model object has been selected
void GUIContainerObject::rememverSelectedGUIModelObject(GUIModelObject *object)
{
    mSelectedGUIModelObjectsList.append(object);
}


//! @brief Notifies container object that a gui model object is no longer selected
void GUIContainerObject::forgetSelectedGUIModelObject(GUIModelObject *object)
{
    mSelectedGUIModelObjectsList.removeAll(object);
}


//! @brief Returns a list with pointers to the selected GUI model objects
QList<GUIModelObject *> GUIContainerObject::getSelectedGUIModelObjectPtrs()
{
    return mSelectedGUIModelObjectsList;
}


//! @brief Returns a pointer to the component with specified name, 0 if not found
GUIModelObject *GUIContainerObject::getGUIModelObject(const QString modelObjectName)
{
    GUIModelObjectMapT::Iterator moit = mGUIModelObjectMap.find(modelObjectName);
    if (moit != mGUIModelObjectMap.end())
    {
        return moit.value();
    }
    else
    {
        return 0;
    }
}

//! @brief Get the port of a sub model object, returns 0 if modelobject or port not found
GUIPort *GUIContainerObject::getGUIModelObjectPort(const QString modelObjectName, const QString portName)
{
    GUIModelObject *pModelObject = this->getGUIModelObject(modelObjectName);
    if (pModelObject != 0)
    {
        return pModelObject->getPort(portName);
    }
    else
    {
        return 0;
    }
}


//! @brief Find a connector in the connector vector
GUIConnector* GUIContainerObject::findConnector(QString startComp, QString startPort, QString endComp, QString endPort)
{
    GUIConnector *item;
    item = 0;
    for(int i = 0; i < mSubConnectorList.size(); ++i)
    {
        if((mSubConnectorList[i]->getStartComponentName() == startComp) &&
           (mSubConnectorList[i]->getStartPortName() == startPort) &&
           (mSubConnectorList[i]->getEndComponentName() == endComp) &&
           (mSubConnectorList[i]->getEndPortName() == endPort))
        {
            item = mSubConnectorList[i];
            break;
        }
        //Find even if the caller mixed up start and stop
        else if((mSubConnectorList[i]->getStartComponentName() == endComp) &&
                (mSubConnectorList[i]->getStartPortName() == endPort) &&
                (mSubConnectorList[i]->getEndComponentName() == startComp) &&
                (mSubConnectorList[i]->getEndPortName() == startPort))
        {
            item = mSubConnectorList[i];
            break;
        }
    }
    assert(!item == 0);
    return item;
}


//! @brief Tells whether or not there is a connector between two specified ports
bool GUIContainerObject::hasConnector(QString startComp, QString startPort, QString endComp, QString endPort)
{
    for(int i = 0; i < mSubConnectorList.size(); ++i)
    {
        if((mSubConnectorList[i]->getStartComponentName() == startComp) &&
           (mSubConnectorList[i]->getStartPortName() == startPort) &&
           (mSubConnectorList[i]->getEndComponentName() == endComp) &&
           (mSubConnectorList[i]->getEndPortName() == endPort))
        {
            return true;
        }
        //Find even if the caller mixed up start and stop
        else if((mSubConnectorList[i]->getStartComponentName() == endComp) &&
                (mSubConnectorList[i]->getStartPortName() == endPort) &&
                (mSubConnectorList[i]->getEndComponentName() == startComp) &&
                (mSubConnectorList[i]->getEndPortName() == startPort))
        {
            return true;
        }
    }
    return false;
}


//! @brief Notifies container object that a subconnector has been selected
void GUIContainerObject::rememberSelectedSubConnector(GUIConnector *pConnector)
{
    mSelectedSubConnectorsList.append(pConnector);
}


//! @brief Notifies container object that a subconnector has been deselected
void GUIContainerObject::forgetSelectedSubConnector(GUIConnector *pConnector)
{
    mSelectedSubConnectorsList.removeAll(pConnector);
}


//! @brief Removes a specified connector from the model.
//! @param pConnector is a pointer to the connector to remove.
//! @param undoSettings is true if the removal of the connector shall not be registered in the undo stack, for example if this function is called by a redo-function.
void GUIContainerObject::removeSubConnector(GUIConnector* pConnector, undoStatus undoSettings)
{
    bool doDelete = false;          //! @todo Why would we not want to delete the connector when we call the function that is meant to delete it?
    bool endPortWasConnected = false;       //Tells if connector is finished or being created

    if(undoSettings == UNDO)
    {
        mpUndoStack->registerDeletedConnector(pConnector);
    }

    for(int i = 0; i < mSubConnectorList.size(); ++i)
    {
        if(mSubConnectorList[i] == pConnector)
        {
             //! @todo some error handling both ports must exist and be connected to each other
             if(pConnector->isConnected())
             {
                 GUIPort *pStartP = pConnector->getStartPort();
                 GUIPort *pEndP = pConnector->getEndPort();
                 this->getCoreSystemAccessPtr()->disconnect(pStartP->getGuiModelObjectName(), pStartP->getPortName(), pEndP->getGuiModelObjectName(), pEndP->getPortName());
                 emit checkMessages();
                 endPortWasConnected = true;
             }
             doDelete = true;       //Connector exists, so delete it?!
        }
        if(mSubConnectorList.empty())
        {
            break;
        }
    }

    //! @todo maybe we should let the port decide by itself if it should be visible or not insed add and removeconnection functions
    //Show the end port if it exists and if it is no longer connected
    if(endPortWasConnected)
    {
        pConnector->getEndPort()->removeConnection(pConnector);
        if(!pConnector->getEndPort()->isConnected())
        {
            pConnector->getEndPort()->setVisible(!mPortsHidden);
        }
    }

    //! @todo maybe we should let the port decide by itself if it should be visible or not insed add and removeconnection functions
    //Show the start port if it is no longer connected
    pConnector->getStartPort()->removeConnection(pConnector);
    if(!pConnector->getStartPort()->isConnected())
    {
        pConnector->getStartPort()->setVisible(!mPortsHidden);
    }

    //Delete the connector and remove it from scene and lists
    if(doDelete)
    {
        mSubConnectorList.removeAll(pConnector);
        mSelectedSubConnectorsList.removeAll(pConnector);
        mpScene->removeItem(pConnector);
        delete pConnector;
    }

    //Refresh the graphics view
    mpParentProjectTab->getGraphicsView()->updateViewPort();

    emit connectorRemoved();
}



//! @brief Begins creation of connector or complete creation of connector depending on the mIsCreatingConnector flag.
//! @param pPort is a pointer to the clicked port, either start or end depending on the mIsCreatingConnector flag.
//! @param undoSettings is true if the added connector shall not be registered in the undo stack, for example if this function is called by a redo function.
void GUIContainerObject::createConnector(GUIPort *pPort, undoStatus undoSettings)
{
        //When clicking start port (begin creation of connector)
    if (!isCreatingConnector())
    {
        mpTempConnector = new GUIConnector(pPort, this);
        deselectAll();
        setIsCreatingConnector(true);
        mpTempConnector->drawConnector();
        gpMainWindow->showHelpPopupMessage("Create the connector by clicking in the workspace. Finish connector by clicking on another component port.");
    }
        //When clicking end port (finish creation of connector)
    else
    {

        bool success = false;

            //If we are connecting to group run special gui only check if connection OK
        if (pPort->getGuiModelObject()->type() == GUIGROUP)
        {
            //! @todo do this
        }
        else
        {
            success = this->getCoreSystemAccessPtr()->connect(mpTempConnector->getStartComponentName(), mpTempConnector->getStartPortName(), pPort->getGuiModelObjectName(), pPort->getPortName() );
        }

        if (success)
        {
            gpMainWindow->hideHelpPopupMessage();
            setIsCreatingConnector(false);
            pPort->getGuiModelObject()->rememberConnector(mpTempConnector);
            mpTempConnector->setEndPort(pPort);
            mpTempConnector->finishCreation();
            mSubConnectorList.append(mpTempConnector);

            if(undoSettings == UNDO)
            {
                mpUndoStack->newPost();
                mpUndoStack->registerAddedConnector(mpTempConnector);
            }
            mpParentProjectTab->hasChanged();
        }
        emit checkMessages();
     }
}


//! @brief Copies the selected components, and then deletes them.
//! @see copySelected()
//! @see paste()
void GUIContainerObject::cutSelected(CopyStack *xmlStack)
{
    this->copySelected(xmlStack);
    this->mpUndoStack->newPost("cut");
    emit deleteSelected();
    mpParentProjectTab->getGraphicsView()->updateViewPort();
}


//! @brief Puts the selected components in the copy stack, and their positions in the copy position stack.
//! @see cutSelected()
//! @see paste()
void GUIContainerObject::copySelected(CopyStack *xmlStack)
{
    QDomElement *copyRoot;
    if(xmlStack == 0)
    {
        gCopyStack.clear();
        copyRoot = gCopyStack.getCopyRoot();
    }
    else
    {
        xmlStack->clear();
        copyRoot = xmlStack->getCopyRoot();
    }

        //Store center point
    QPointF center = getCenterPointFromSelection();
    appendCoordinateTag(*copyRoot, center.x(), center.y());

        //Copy components
    QList<GUIModelObject *>::iterator it;
    for(it = mSelectedGUIModelObjectsList.begin(); it!=mSelectedGUIModelObjectsList.end(); ++it)
    {
        //qDebug() << "Copying " << (*it)->getName();
        (*it)->saveToDomElement(*copyRoot);
    }

        //Copy connectors
    for(int i = 0; i != mSubConnectorList.size(); ++i)
    {
        if(mSubConnectorList[i]->getStartPort()->getGuiModelObject()->isSelected() && mSubConnectorList[i]->getEndPort()->getGuiModelObject()->isSelected() && mSubConnectorList[i]->isActive())
        {
            mSubConnectorList[i]->saveToDomElement(*copyRoot);
        }
    }

        //Copy widgets
    QMap<size_t, GUIWidget *>::iterator itw;
    for(itw = mWidgetMap.begin(); itw!=mWidgetMap.end(); ++itw)
    {
        if((*itw)->isSelected())
        {
            (*itw)->saveToDomElement(*copyRoot);
        }
    }
}


//! @brief Pastes the contents in the copy stack at the mouse position
//! @see cutSelected()
//! @see copySelected()
void GUIContainerObject::paste(CopyStack *xmlStack)
{

    //gpMainWindow->mpMessageWidget->printGUIDebugMessage(gCopyStack.getXML());

    mpUndoStack->newPost("paste");
    mpParentProjectTab->hasChanged();

    QDomElement *copyRoot;
    if(xmlStack == 0)
    {
        copyRoot = gCopyStack.getCopyRoot();
    }
    else
    {
        copyRoot = xmlStack->getCopyRoot();
    }

        //Deselect all components & connectors
    emit deselectAllGUIObjects();
    emit deselectAllGUIConnectors();

    QHash<QString, QString> renameMap;       //Used to track name changes, so that connectors will know what components are called

        //Determine paste offset (will paste components at mouse position
    QDomElement coordTag = copyRoot->firstChildElement(HMF_COORDINATETAG);
    double x, y;
    parseCoordinateTag(coordTag, x, y);
    QPointF oldCenter = QPointF(x, y);

    QCursor cursor;
    QPointF newCenter = mpParentProjectTab->getGraphicsView()->mapToScene(mpParentProjectTab->getGraphicsView()->mapFromGlobal(cursor.pos()));

    //qDebug() << "Pasting at " << newCenter;

    double xOffset = newCenter.x() - oldCenter.x();
    double yOffset = newCenter.y() - oldCenter.y();

        //Paste components
    QDomElement objectElement = copyRoot->firstChildElement(HMF_COMPONENTTAG);
    while(!objectElement.isNull())
    {
        GUIModelObject *pObj = loadGUIModelObject(objectElement, gpMainWindow->mpLibrary, this);

            //Apply parameter values
        QDomElement xmlParameters = objectElement.firstChildElement(HMF_PARAMETERS);
        QDomElement xmlParameter = xmlParameters.firstChildElement(HMF_PARAMETERTAG);
        while (!xmlParameter.isNull())
        {
            loadParameterValue(xmlParameter, pObj);
            xmlParameter = xmlParameter.nextSiblingElement(HMF_PARAMETERTAG);
        }

            //Apply offset to pasted object
        QPointF oldPos = pObj->pos();
        pObj->moveBy(xOffset, yOffset);
        mpUndoStack->registerMovedObject(oldPos, pObj->pos(), pObj->getName());

        renameMap.insert(objectElement.attribute(HMF_NAMETAG), pObj->getName());
        //objectElement.setAttribute("name", renameMap.find(objectElement.attribute(HMF_NAMETAG)).value());
        objectElement = objectElement.nextSiblingElement("component");
    }

        // Paste subsystems
    //! @todo maybe this subsystem loop can be merged with components above somehow. Basically the same code is used now after some cleanup, That way we could  have one loop for guimodelobjects, one for connector and after some cleanup one for widgets
    QDomElement systemElement = copyRoot->firstChildElement(HMF_SYSTEMTAG);
    while (!systemElement.isNull())
    {
        GUIModelObject* pObj = loadGUIModelObject(systemElement, gpMainWindow->mpLibrary, this, UNDO);
        renameMap.insert(systemElement.attribute(HMF_NAMETAG), pObj->getName());
        systemElement = systemElement.nextSiblingElement(HMF_SYSTEMTAG);

            //Apply offset to pasted object
        QPointF oldPos = pObj->pos();
        pObj->moveBy(xOffset, yOffset);
        mpUndoStack->registerMovedObject(oldPos, pObj->pos(), pObj->getName());
    }

        // Paste container ports
    QDomElement systemPortElement = copyRoot->firstChildElement(HMF_SYSTEMPORTTAG);
    while (!systemPortElement.isNull())
    {
        GUIModelObject* pObj = loadContainerPortObject(systemPortElement, gpMainWindow->mpLibrary, this, UNDO);
        renameMap.insert(systemPortElement.attribute(HMF_NAMETAG), pObj->getName());
        systemPortElement = systemPortElement.nextSiblingElement(HMF_SYSTEMPORTTAG);

            //Apply offset to pasted object
        QPointF oldPos = pObj->pos();
        pObj->moveBy(xOffset, yOffset);
        mpUndoStack->registerMovedObject(oldPos, pObj->pos(), pObj->getName());
    }

        //Paste connectors
    QDomElement connectorElement = copyRoot->firstChildElement(HMF_CONNECTORTAG);
    while(!connectorElement.isNull())
    {
        QDomElement tempConnectorElement = connectorElement.cloneNode(true).toElement();
        tempConnectorElement.setAttribute("startcomponent", renameMap.find(connectorElement.attribute("startcomponent")).value());
        tempConnectorElement.setAttribute("endcomponent", renameMap.find(connectorElement.attribute("endcomponent")).value());

        bool sucess = loadConnector(tempConnectorElement, this, UNDO);
        if (sucess)
        {
            ////qDebug() << ",,,,,,,,,: " << tempConnectorElement.attribute("startcomponent") << " " << tempConnectorElement.attribute("startport") << " " << tempConnectorElement.attribute("endcomponent") << " " << tempConnectorElement.attribute("endport");
            GUIConnector *tempConnector = this->findConnector(tempConnectorElement.attribute("startcomponent"), tempConnectorElement.attribute("startport"),
                                                              tempConnectorElement.attribute("endcomponent"), tempConnectorElement.attribute("endport"));

                //Apply offset to connector and register it in undo stack
            tempConnector->moveAllPoints(xOffset, yOffset);
            tempConnector->drawConnector(true);
            for(int i=0; i<(tempConnector->getNumberOfLines()-2); ++i)
            {
                mpUndoStack->registerModifiedConnector(QPointF(tempConnector->getLine(i)->pos().x(), tempConnector->getLine(i)->pos().y()),
                                                      tempConnector->getLine(i+1)->pos(), tempConnector, i+1);
            }
        }

        connectorElement = connectorElement.nextSiblingElement("connect");
    }

        //Paste widgets
    QDomElement textBoxElement = copyRoot->firstChildElement(HMF_TEXTBOXWIDGETTAG);
    while(!textBoxElement.isNull())
    {
        GUITextBoxWidget *pWidget = loadTextBoxWidget(textBoxElement, this, NOUNDO);

        pWidget->setSelected(true);
        pWidget->moveBy(xOffset, yOffset);
        mpUndoStack->registerAddedWidget(pWidget);
        textBoxElement = textBoxElement.nextSiblingElement(HMF_TEXTBOXWIDGETTAG);
    }

        //Select all pasted components
    QHash<QString, QString>::iterator itn;
    for(itn = renameMap.begin(); itn != renameMap.end(); ++itn)
    {
        mGUIModelObjectMap.find(itn.value()).value()->setSelected(true);
    }

    mpParentProjectTab->getGraphicsView()->updateViewPort();
}


//! @brief Aligns all selected objects vertically to the last selected object.
void GUIContainerObject::alignX()
{
    if(mSelectedGUIModelObjectsList.size() > 1)
    {
        mpUndoStack->newPost("alignx");
        for(int i=0; i<mSelectedGUIModelObjectsList.size()-1; ++i)
        {
            QPointF oldPos = mSelectedGUIModelObjectsList.at(i)->pos();
            mSelectedGUIModelObjectsList.at(i)->setCenterPos(QPointF(mSelectedGUIModelObjectsList.last()->getCenterPos().x(), mSelectedGUIModelObjectsList.at(i)->getCenterPos().y()));
            QPointF newPos = mSelectedGUIModelObjectsList.at(i)->pos();
            mpUndoStack->registerMovedObject(oldPos, newPos, mSelectedGUIModelObjectsList.at(i)->getName());
            for(int j=0; j<mSelectedGUIModelObjectsList.at(i)->getGUIConnectorPtrs().size(); ++j)
            {
                mSelectedGUIModelObjectsList.at(i)->getGUIConnectorPtrs().at(j)->drawConnector(true);
            }
        }
        mpParentProjectTab->hasChanged();
    }
}


//! @brief Aligns all selected objects horizontally to the last selected object.
void GUIContainerObject::alignY()
{
    if(mSelectedGUIModelObjectsList.size() > 1)
    {
        mpUndoStack->newPost("aligny");
        for(int i=0; i<mSelectedGUIModelObjectsList.size()-1; ++i)
        {
            QPointF oldPos = mSelectedGUIModelObjectsList.at(i)->pos();
            mSelectedGUIModelObjectsList.at(i)->setCenterPos(QPointF(mSelectedGUIModelObjectsList.at(i)->getCenterPos().x(), mSelectedGUIModelObjectsList.last()->getCenterPos().y()));
            QPointF newPos = mSelectedGUIModelObjectsList.at(i)->pos();
            mpUndoStack->registerMovedObject(oldPos, newPos, mSelectedGUIModelObjectsList.at(i)->getName());
            for(int j=0; j<mSelectedGUIModelObjectsList.at(i)->getGUIConnectorPtrs().size(); ++j)
            {
                mSelectedGUIModelObjectsList.at(i)->getGUIConnectorPtrs().at(j)->drawConnector(true);
            }
        }
        mpParentProjectTab->hasChanged();
    }
}


//! @brief Calculates the geometrical center position of the selected objects.
QPointF GUIContainerObject::getCenterPointFromSelection()
{
    double sumX = 0;
    double sumY = 0;
    int nSelected = 0;
    for(int i=0; i<mSelectedGUIModelObjectsList.size(); ++i)
    {
        sumX += mSelectedGUIModelObjectsList.at(i)->getCenterPos().x();
        sumY += mSelectedGUIModelObjectsList.at(i)->getCenterPos().y();
        ++nSelected;
    }
    for(int i=0; i<mSelectedGUIWidgetsList.size(); ++i)
    {
        sumX += mSelectedGUIWidgetsList.at(i)->getCenterPos().x();
        sumY += mSelectedGUIWidgetsList.at(i)->getCenterPos().y();
        ++nSelected;
    }

    return QPointF(sumX/nSelected, sumY/nSelected);
}


//! @brief Groups the selected objects together.
void GUIContainerObject::groupSelected(QPointF pt)
{
    gpMainWindow->mpMessageWidget->printGUIWarningMessage("Groups are not yet fully implemented, DO NOT use them, it will only end in tears!");
    //qDebug() << "pos where we want to create group: " << pt;
    //qDebug() << "In group selected";

    //Copy the selected objects, the lists will be cleared by addGuiobject and we need to keep this information
    QList<GUIModelObject*> modelObjects = mSelectedGUIModelObjectsList;
    QList<GUIWidget*> widgets = mSelectedGUIWidgetsList;

    //"Detach" the selected objects from this container, basically by removing pointers from the subobject storage maps, make this container forget aboout these objects
    for (int i=0; i<modelObjects.size(); ++i)
    {
        //! @todo if a containerport is selcted we need to remove it in core, not only from the storage vector, we must also make sure that the external ports are updated accordingly, for now we just ignore them (maybe we should allways ignore them when grouping)
        if (modelObjects[i]->type() != GUICONTAINERPORT)
        {
            mGUIModelObjectMap.remove(modelObjects[i]->getName());
        }
    }

    for (int i=0; i<widgets.size(); ++i)
    {
        mWidgetMap.remove(widgets[i]->getWidgetIndex());
    }

    //Create a new group at the location of the specified
    GUIModelObject* pObj =  this->addGUIModelObject(HOPSANGUIGROUPTYPENAME, pt.toPoint(),0);
    GUIContainerObject* pContainer =  qobject_cast<GUIContainerObject*>(pObj);

    //If dyncast sucessfull (it should allways be) then let new group take ownership of objects
    if (pContainer != 0)
    {
        pContainer->takeOwnershipOf(modelObjects, widgets);
    }
    else
    {
        assert(false);
    }
}


//! @brief Selects model objects in section with specified number.
//! @param no Number of section
//! @param append True if previously selected objects shall remain selected
void GUIContainerObject::selectSection(int no, bool append)
{
    if(!append)
    {
        deselectAll();
    }
    if(mSection.size() < no+1)
    {
        return;
    }
    for(int i=0; i<mSection.at(no).size(); ++i)
    {
        mSection[no][i]->select();
    }
}


//! @brief Assigns the selected component to the section with specified number.
//! This is used to "group" components into sections with Ctrl+#, so they can be selected quickly again by pressing #.
//! @param no Number of section
void GUIContainerObject::assignSection(int no)
{
    if(!isSubObjectSelected()) return;
    while(mSection.size()<no+1)
    {
        QList<GUIModelObject *> dummyList;
        mSection.append(dummyList);
    }
    mSection[no].clear();
    mSection[no].append(mSelectedGUIModelObjectsList);
}


//! @brief Selects all objects and connectors.
void GUIContainerObject::selectAll()
{
    emit selectAllGUIObjects();
    emit selectAllGUIConnectors();
}


//! @brief Deselects all objects and connectors.
void GUIContainerObject::deselectAll()
{
    emit deselectAllGUIObjects();
    emit deselectAllGUIConnectors();
}


//! @brief Hides all component names.
//! @see showNames()
void GUIContainerObject::hideNames()
{
    mpUndoStack->newPost("hideallnames");
    emit deselectAllNameText();
    emit hideAllNameText();
}


//! @brief Shows all component names.
//! @see hideNames()
void GUIContainerObject::showNames()
{
    mpUndoStack->newPost("showallnames");
    emit showAllNameText();
}


//! @brief Toggles name text on or off
//! @see showNames();
//! @see hideNames();
void GUIContainerObject::toggleNames(bool value)
{
    if(value)
    {
        emit showAllNameText();
    }
    else
    {
        emit hideAllNameText();
    }
    mNamesHidden = !value;
    //mpParentProjectTab->hasChanged();
}


//! @brief Slot that sets hide ports flag to true or false
void GUIContainerObject::hidePorts(bool doIt)
{
    mPortsHidden = !doIt;
    //mpParentProjectTab->hasChanged();
}




//! @brief Slot that tells the mUndoStack to execute one undo step. Necessary because the undo stack is not a QT object and cannot use its own slots.
//! @see redo()
//! @see clearUndo()
void GUIContainerObject::undo()
{
    mpUndoStack->undoOneStep();
}


//! @brief Slot that tells the mUndoStack to execute one redo step. Necessary because the redo stack is not a QT object and cannot use its own slots.
//! @see undo()
//! @see clearUndo()
void GUIContainerObject::redo()
{
    mpUndoStack->redoOneStep();
}

//! @brief Slot that tells the mUndoStack to clear itself. Necessary because the redo stack is not a QT object and cannot use its own slots.
//! @see undo()
//! @see redo()
void GUIContainerObject::clearUndo()
{
    //qDebug() << "before mUndoStack->clear(); in GUIContainerObject: " << this->getName();
    mpUndoStack->clear();
}


//! @brief Returns true if at least one GUIObject is selected
bool GUIContainerObject::isSubObjectSelected()
{
    return (mSelectedGUIModelObjectsList.size() > 0);
}


//! @brief Returns true if at least one GUIConnector is selected
bool GUIContainerObject::isConnectorSelected()
{
    return (mSelectedSubConnectorsList.size() > 0);
}


//! @brief Tells the container object that one more plot curve is opened
void GUIContainerObject::incrementOpenPlotCurves()
{
    ++nPlotCurves;
}


//! @brief Tells the container object that one less plot curve is opened
void GUIContainerObject::decrementOpenPlotCurves()
{
    --nPlotCurves;
}

//! @brief Tells whether or not the container object has at least one plot curve opened in a plot window
bool GUIContainerObject::hasOpenPlotCurves()
{
    return (nPlotCurves > 0);
}


//! @brief Returns a pointer to the undo stack
UndoStack *GUIContainerObject::getUndoStackPtr()
{
    return mpUndoStack;
}


//! @brief Returns a pointer to the drag-copy copy stack
CopyStack *GUIContainerObject::getDragCopyStackPtr()
{
    return mpDragCopyStack;
}


//! @brief Specifies model file for the container object
void GUIContainerObject::setModelFile(QString path)
{
    mModelFileInfo.setFile(path);
}


//! @brief Returns a copy of the model file info of the container object
QFileInfo GUIContainerObject::getModelFileInfo()
{
    return mModelFileInfo;
}


//! @brief Specifies a script file to be executed when model is loaded
//! @todo Shall we have this?
void GUIContainerObject::setScriptFile(QString path)
{
    mScriptFilePath = path;
}


//! @brief Returns path to the script file
QString GUIContainerObject::getScriptFile()
{
    return mScriptFilePath;
}


//! @brief Returns a list with the names of the model objects in the container
QStringList GUIContainerObject::getGUIModelObjectNames()
{
    QStringList retval;
    GUIContainerObject::GUIModelObjectMapT::iterator it;
    for(it = mGUIModelObjectMap.begin(); it!=mGUIModelObjectMap.end(); ++it)
    {
        retval.append(it.value()->getName());
    }
    return retval;
}


//! @brief Returns a list with pointers to GUI widgets
QList<GUIWidget *> GUIContainerObject::getGUIWidgets()
{
    QList<GUIWidget *> list;
    QMap<size_t, GUIWidget *>::iterator it;
    for(it=mWidgetMap.begin(); it!=mWidgetMap.end(); ++it)
    {
        list.append(it.value());
    }
    return list;
}

//! @brief Returns the path to the icon with iso graphics.
//! @todo should we return full path or relative
QString GUIContainerObject::getIconPath(const graphicsType gfxType, const AbsoluteRelativeT absrelType)
{
    return mGUIModelObjectAppearance.getIconPath(gfxType, absrelType);
}


//! @brief Sets the path to the icon of the specified type
void GUIContainerObject::setIconPath(const QString path, const graphicsType gfxType, const AbsoluteRelativeT absrelType)
{
    mGUIModelObjectAppearance.setIconPath(path, gfxType, absrelType);
}


//! @brief Access function for mIsCreatingConnector
//! @param isConnected is the new value
void GUIContainerObject::setIsCreatingConnector(bool isCreatingConnector)
{
    mIsCreatingConnector = isCreatingConnector;
}


//! @brief Access function for mIsCreatingConnector
bool GUIContainerObject::isCreatingConnector()
{
    return mIsCreatingConnector;
}


//! @brief Tells container object to remember a new sub connector
void GUIContainerObject::rememberSubConnector(GUIConnector *pConnector)
{
    mSubConnectorList.append(pConnector);
}


//! @brief This is a helpfunction that can be used to make a container "forget" about a certain connector
//!
//! It does not delete the connector and connected components dos not forget about it
//! use only when transfering ownership of objects to an other container
void GUIContainerObject::forgetSubConnector(GUIConnector *pConnector)
{
    mSubConnectorList.removeAll(pConnector);
}

//! @brief Refresh the graphics of all internal container ports
void GUIContainerObject::refreshInternalContainerPortGraphics()
{
    GUIModelObjectMapT::iterator moit;
    for(moit = mGUIModelObjectMap.begin(); moit != mGUIModelObjectMap.end(); ++moit)
    {
        if(moit.value()->type() == GUICONTAINERPORT)
        {
            //We assume that a container port only have ONE gui port
            moit.value()->getPortListPtrs().first()->refreshPortGraphics();
        }
    }
}


//! @brief Aborts creation of new connector.
void GUIContainerObject::cancelCreatingConnector()
{
    if(mIsCreatingConnector)
    {
        mpTempConnector->getStartPort()->removeConnection(mpTempConnector);
        if(!mpTempConnector->getStartPort()->isConnected() && !mPortsHidden)
        {
            mpTempConnector->getStartPort()->show();
        }
        mpTempConnector->getStartPort()->getGuiModelObject()->forgetConnector(mpTempConnector);
        setIsCreatingConnector(false);
        delete(mpTempConnector);
        gpMainWindow->hideHelpPopupMessage();
    }
}


//! @brief Swiches mode of connector being created to or from diagonal mode.
//! @param diagonal Tells whether or not connector shall be diagonal or not
void GUIContainerObject::makeConnectorDiagonal(bool diagonal)
{
    if (mIsCreatingConnector && (mpTempConnector->isMakingDiagonal() != diagonal))
    {
        mpTempConnector->makeDiagonal(diagonal);
        mpTempConnector->drawConnector();
        mpParentProjectTab->getGraphicsView()->updateViewPort();
    }
}


//! @brief Redraws the connector being created.
//! @param pos Position to draw connector to
void GUIContainerObject::updateTempConnector(QPointF pos)
{
    mpTempConnector->updateEndPoint(pos);
    mpTempConnector->drawConnector();
}


//! @brief Adds one new line to the connector being created.
//! @param pos Position to add new line at
void GUIContainerObject::addOneConnectorLine(QPointF pos)
{
    mpTempConnector->addPoint(pos);
}


//! @brief Removse one line from connector being created.
//! @param pos Position to redraw connector to after removing the line
void GUIContainerObject::removeOneConnectorLine(QPointF pos)
{
    if((mpTempConnector->getNumberOfLines() == 1 && mpTempConnector->isMakingDiagonal()) ||  (mpTempConnector->getNumberOfLines() == 2 && !mpTempConnector->isMakingDiagonal()))
    {
        mpTempConnector->getStartPort()->removeConnection(mpTempConnector);
        if(!mpTempConnector->getStartPort()->isConnected() && !mPortsHidden)
        {
            mpTempConnector->getStartPort()->show();
        }
        mpTempConnector->getStartPort()->getGuiModelObject()->forgetConnector(mpTempConnector);
        mIsCreatingConnector = false;
        mpParentProjectTab->getGraphicsView()->setIgnoreNextContextMenuEvent();
        delete(mpTempConnector);
        gpMainWindow->hideHelpPopupMessage();
    }

    if(mIsCreatingConnector)
    {
        mpTempConnector->removePoint(true);
        mpTempConnector->updateEndPoint(pos);
        mpTempConnector->drawConnector();
        mpParentProjectTab->getGraphicsView()->updateViewPort();
    }
}


void GUIContainerObject::setUndoDisabled(bool disabled, bool dontAskJustDoIt)
{
    setUndoEnabled(!disabled, dontAskJustDoIt);
}


//! @brief Disables the undo function for the current model.
//! @param enabled Tells whether or not to enable the undo stack
//! @param dontAskJustDoIt If true, the warning box will not appear
void GUIContainerObject::setUndoEnabled(bool enabled, bool dontAskJustDoIt)
{
    if(!enabled)
    {
        bool doIt=true;
        if (!dontAskJustDoIt)
        {
            QMessageBox disableUndoWarningBox(QMessageBox::Warning, tr("Warning"),tr("Disabling undo history will clear all undo history for this model. Do you want to continue?"), 0, 0);
            disableUndoWarningBox.addButton(tr("&Yes"), QMessageBox::AcceptRole);
            disableUndoWarningBox.addButton(tr("&No"), QMessageBox::RejectRole);
            disableUndoWarningBox.setWindowIcon(gpMainWindow->windowIcon());

            doIt = (disableUndoWarningBox.exec() == QMessageBox::AcceptRole);
        }

        if (doIt)
        {
            this->clearUndo();
            mUndoDisabled = true;
            if(gpMainWindow->mpProjectTabs->getCurrentContainer() == this)      //Only modify main window actions if this is current container
            {
                gpMainWindow->mpUndoAction->setDisabled(true);
                gpMainWindow->mpRedoAction->setDisabled(true);
            }
        }
    }
    else
    {
        mUndoDisabled = false;
        if(gpMainWindow->mpProjectTabs->getCurrentContainer() == this)      //Only modify main window actions if this is current container
        {
            gpMainWindow->mpUndoAction->setDisabled(false);
            gpMainWindow->mpRedoAction->setDisabled(false);
        }
    }

    if(gpMainWindow->mpDisableUndoAction->isChecked() != mUndoDisabled)
        gpMainWindow->mpDisableUndoAction->setChecked(mUndoDisabled);
}


//! @brief Tells whether or not unconnected ports in container are hidden
bool GUIContainerObject::arePortsHidden()
{
    return mPortsHidden;
}


//! @brief Tells whether or not object names in container are hidden
bool GUIContainerObject::areNamesHidden()
{
    return mNamesHidden;
}


//! @brief Tells whether or not undo/redo is enabled
bool GUIContainerObject::isUndoEnabled()
{
    return !mUndoDisabled;
}


//! @brief Enables or disables the undo buttons depending on whether or not undo is disabled in current tab
void GUIContainerObject::updateMainWindowButtons()
{
    gpMainWindow->mpUndoAction->setDisabled(mUndoDisabled);
    gpMainWindow->mpRedoAction->setDisabled(mUndoDisabled);

    gpMainWindow->mpPlotAction->setDisabled(mPlotData.isEmpty());
    gpMainWindow->mpShowLossesAction->setDisabled(mPlotData.isEmpty());
}


//! @brief Sets the iso graphics option for the model
void GUIContainerObject::setGfxType(graphicsType gfxType)
{
    this->mGfxType = gfxType;
    this->mpParentProjectTab->getGraphicsView()->updateViewPort();
    emit setAllGfxType(mGfxType);
}


//! @brief Returns current graphics type used by container object
graphicsType GUIContainerObject::getGfxType()
{
    return mGfxType;
}


//! @brief A slot that opens the properties dialog
void GUIContainerObject::openPropertiesDialogSlot()
{
    this->openPropertiesDialog();
}


//! @brief Slot that tells all selected name texts to deselect themselves
void GUIContainerObject::deselectSelectedNameText()
{
    emit deselectAllNameText();
}


//! @brief Defines the right click menu for container objects.
//! @todo Maybe should try to reduce multiple copys of same functions with other GUIObjects
void GUIContainerObject::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu menu;
    QAction *loadAction = menu.addAction(tr("Load Subsystem File"));
    if(!mModelFileInfo.filePath().isEmpty())
    {
        loadAction->setDisabled(true);
    }
    QAction *pAction = this->buildBaseContextMenu(menu, event);
    if (pAction == loadAction)
    {
        QDir fileDialog; QFile file;
        QString modelFilePath = QFileDialog::getOpenFileName(mpParentProjectTab->mpParentProjectTabWidget, tr("Choose Subsystem File"),
                                                             fileDialog.currentPath() + QString(MODELPATH),
                                                             tr("Hopsan Model Files (*.hmf)"));
        if (!modelFilePath.isNull())
        {
            bool doIt = true;
            if (mGUIModelObjectMap.size() > 0)
            {
                QMessageBox clearAndLoadQuestionBox(QMessageBox::Warning, tr("Warning"),tr("All current contents of the system will be replaced. Do you want to continue?"), 0, 0);
                clearAndLoadQuestionBox.addButton(tr("&Yes"), QMessageBox::AcceptRole);
                clearAndLoadQuestionBox.addButton(tr("&No"), QMessageBox::RejectRole);
                clearAndLoadQuestionBox.setWindowIcon(gpMainWindow->windowIcon());
                doIt = (clearAndLoadQuestionBox.exec() == QMessageBox::AcceptRole);
            }

            if (doIt)
            {
                this->clearContents();

                file.setFileName(modelFilePath);
                QDomDocument domDocument;
                QDomElement hmfRoot = loadXMLDomDocument(file, domDocument, HMF_ROOTTAG);
                if (!hmfRoot.isNull())
                {
                    //! @todo Check version numbers
                    //! @todo check if we could load else give error message and dont attempt to load
                    QDomElement systemElement = hmfRoot.firstChildElement(HMF_SYSTEMTAG);
                    this->setModelFileInfo(file); //Remember info about the file from which the data was loaded
                    QFileInfo fileInfo(file);
                    this->setAppearanceDataBasePath(fileInfo.absolutePath());
                    this->loadFromDomElement(systemElement);
                }
            }
        }
    }

    //Dont call GUIModelObject::contextMenuEvent as that will open an other menu after this one is closed
    //GUIModelObject::contextMenuEvent(event);
    ////QGraphicsItem::contextMenuEvent(event);
}


//! @brief Defines the double click event for container objects (used to enter containers).
void GUIContainerObject::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    GUIModelObject::mouseDoubleClickEvent(event);
    this->enterContainer();
}


//! @brief Opens the properites dialog for container objects.
void GUIContainerObject::openPropertiesDialog()
{
    //Do Nothing
}


//! @brief Clears all of the contained objects (and delets them).
//! This code cant be run in the desturctor as this wold cause wired behaviour in the derived susyem class.
//! The core system would be deleted before container clear code is run, that is why we have it as a convenient protected function
void GUIContainerObject::clearContents()
{
    GUIModelObjectMapT::iterator mit;
    QMap<size_t, GUIWidget *>::iterator wit;

    //qDebug() << "Clearing model objects in " << getName();
    //We cant use for loop over iterators as the maps are modified on each delete (and iterators invalidated)
    mit=mGUIModelObjectMap.begin();
    while (mit!=mGUIModelObjectMap.end())
    {
        //This may lead to a crash if undo stack is not disabled before calling this
        (*mit)->deleteMe();
        mit=mGUIModelObjectMap.begin();
    }

    //qDebug() << "Clearing widget objects in " << getName();
    wit=mWidgetMap.begin();
    while (wit!=mWidgetMap.end())
    {
        (*wit)->deleteMe();
        wit=mWidgetMap.begin();
    }
}


//! @brief Enters a container object and maks the view represent it contents.
void GUIContainerObject::enterContainer()
{
    // First deselect everything so that buttons pressed in the view are not sent to obejcts in the previous container
    mpParentContainerObject->deselectAll(); //deselect myself and anyone else

    // Show this scene
    mpParentProjectTab->getGraphicsView()->setScene(getContainedScenePtr());
    mpParentProjectTab->getGraphicsView()->setContainerPtr(this);
    mpParentProjectTab->getQuickNavigationWidget()->addOpenContainer(this);

    // Disconnect parent system and connect new system with actions
    mpParentContainerObject->disconnectMainWindowActions();
    this->connectMainWindowActions();

    //Update plot widget and undo widget to new container
    gpMainWindow->mpPlotWidget->mpPlotParameterTree->updateList();
    gpMainWindow->mpSystemParametersWidget->update();
    gpMainWindow->mpUndoWidget->refreshList();
    gpMainWindow->mpUndoAction->setDisabled(this->mUndoDisabled);
    gpMainWindow->mpRedoAction->setDisabled(this->mUndoDisabled);

    refreshInternalContainerPortGraphics();

    this->collectPlotData();

    mpParentProjectTab->setExternalSystem((this->isExternal() &&
                                           this != mpParentProjectTab->getSystem()) ||
                                           this->isAncestorOfExternalSubsystem());
}

//! @brief Exit a container object and maks its the view represent its parents contents.
void GUIContainerObject::exitContainer()
{
    this->deselectAll();

    // Go back to parent system
    mpParentProjectTab->getGraphicsView()->setScene(this->mpParentContainerObject->getContainedScenePtr());
    mpParentProjectTab->getGraphicsView()->setContainerPtr(this->mpParentContainerObject);

    mpParentProjectTab->setExternalSystem((mpParentContainerObject->isExternal() &&
                                           mpParentContainerObject != mpParentProjectTab->getSystem()) ||
                                           mpParentContainerObject->isAncestorOfExternalSubsystem());

    // Disconnect this system and connect parent system with undo and redo actions
    this->disconnectMainWindowActions();
    mpParentContainerObject->connectMainWindowActions();

        //Update plot widget and undo widget to new container
    gpMainWindow->mpPlotWidget->mpPlotParameterTree->updateList();
    gpMainWindow->mpSystemParametersWidget->update();
    gpMainWindow->mpUndoWidget->refreshList();
    gpMainWindow->mpUndoAction->setDisabled(!mpParentContainerObject->isUndoEnabled());
    gpMainWindow->mpRedoAction->setDisabled(!mpParentContainerObject->isUndoEnabled());

    // Refresh external port appearance
    //! @todo We only need to do this if ports have change, right now we always refresh, dont know if this is a big deal
    this->refreshExternalPortsAppearanceAndPosition();

    mpParentContainerObject->collectPlotData();
}


//! @brief Rotates all selected objects right (clockwise)
void GUIContainerObject::rotateSubObjects90cw()
{
    if(this->isSubObjectSelected())
    {
        mpUndoStack->newPost();
        mpParentProjectTab->hasChanged();
        emit rotateSelectedObjectsRight();
    }
}


//! @brief Rotates all selected objects left (counter-clockwise)
void GUIContainerObject::rotateSubObjects90ccw()
{
    if(this->isSubObjectSelected())
    {
        mpUndoStack->newPost();
        mpParentProjectTab->hasChanged();
        emit rotateSelectedObjectsLeft();
    }
}


//! @brief Flips selected contained objects horizontally
void GUIContainerObject::flipSubObjectsHorizontal()
{
    if(this->isSubObjectSelected())
    {
        mpUndoStack->newPost();
        mpParentProjectTab->hasChanged();
        emit flipSelectedObjectsHorizontal();
    }
}


//! @brief Flips selected contained objects vertically
void GUIContainerObject::flipSubObjectsVertical()
{
    if(this->isSubObjectSelected())
    {
        mpUndoStack->newPost();
        mpParentProjectTab->hasChanged();
        emit flipSelectedObjectsVertical();
    }
}


//! @brief Collects the plot data from the last simulation for all plot variables from the core and stores them locally.
void GUIContainerObject::collectPlotData()
{
    //bool timeVectorObtained = false;

    GUIModelObjectMapT::iterator moit;
    QList<GUIPort*>::iterator pit;
    QMap< QString, QMap< QString, QMap<QString, QPair<QVector<double>, QVector<double> > > > > componentMap;
    for(moit=mGUIModelObjectMap.begin(); moit!=mGUIModelObjectMap.end(); ++moit)
    {
        QMap< QString, QMap<QString, QPair<QVector<double>, QVector<double> > > > portMap;
        for(pit=moit.value()->getPortListPtrs().begin(); pit!=moit.value()->getPortListPtrs().end(); ++pit)
        {
            QMap<QString, QPair<QVector<double>, QVector<double> > > variableMap;

            QVector<QString> names;
            QVector<QString> units;
            getCoreSystemAccessPtr()->getPlotDataNamesAndUnits(moit.value()->getName(), (*pit)->getPortName(), names, units);

            QVector<QString>::iterator nit;
            for(nit=names.begin(); nit!=names.end(); ++nit)
            {
                QPair<QVector<double>, QVector<double> > data;
                getCoreSystemAccessPtr()->getPlotData(moit.value()->getName(), (*pit)->getPortName(), (*nit), data);
                variableMap.insert((*nit), data);
//                if(!timeVectorObtained)
//                {
//                    mTimeVectors.append(QVector<double>::fromStdVector(getCoreSystemAccessPtr()->getTimeVector(moit.value()->getName(), (*pit)->getName())));
//                    timeVectorObtained = true;
//                }

                ////qDebug() << "Inserting: " << moit.value()->getName() << ", " << (*pit)->getName() << ", " << (*nit);
            }
            portMap.insert((*pit)->getPortName(), variableMap);
        }
        componentMap.insert(moit.value()->getName(), portMap);
    }
    if(!componentMap.isEmpty())
    {
        mPlotData.append(componentMap);
    }
}


void GUIContainerObject::showLosses(bool show)
{
    if(!show)
    {
        mLossesVisible=false;
        hideLosses();
        return;
    }


    QLabel *pInfoLabel = new QLabel(tr("This will calculate energy losses for each component."));
    pInfoLabel->setWordWrap(true);
    pInfoLabel->setFixedWidth(300);

    mpLossesDialog = new QDialog(gpMainWindow);
    mpLossesDialog->setWindowTitle("Calculate Losses");

    QCheckBox *pIgnoreSmallLossesCheckBox = new QCheckBox("Ignore small losses in bar chart plot");
    pIgnoreSmallLossesCheckBox->setChecked(true);

    QLabel *pMinLossesLabel = new QLabel("Ignore losses smaller than ");
    QLabel *pMinLossesValue = new QLabel();
    QLabel *pMinLossesUnit = new QLabel("%");
    pMinLossesValue->setNum(5);
    mpMinLossesSlider = new QSlider(mpLossesDialog);
    mpMinLossesSlider->setOrientation(Qt::Horizontal);
    mpMinLossesSlider->setMinimum(0);
    mpMinLossesSlider->setMaximum(100);
    mpMinLossesSlider->setValue(5);
    connect(mpMinLossesSlider, SIGNAL(valueChanged(int)), pMinLossesValue, SLOT(setNum(int)));

    QHBoxLayout *pSliderLayout = new QHBoxLayout();
    pSliderLayout->addWidget(pMinLossesLabel);
    pSliderLayout->addWidget(mpMinLossesSlider);
    pSliderLayout->addWidget(pMinLossesValue);
    pSliderLayout->addWidget(pMinLossesUnit);

    connect(pIgnoreSmallLossesCheckBox, SIGNAL(toggled(bool)), pMinLossesLabel, SLOT(setEnabled(bool)));
    connect(pIgnoreSmallLossesCheckBox, SIGNAL(toggled(bool)), mpMinLossesSlider, SLOT(setEnabled(bool)));
    connect(pIgnoreSmallLossesCheckBox, SIGNAL(toggled(bool)), pMinLossesValue, SLOT(setEnabled(bool)));
    connect(pIgnoreSmallLossesCheckBox, SIGNAL(toggled(bool)), pMinLossesUnit, SLOT(setEnabled(bool)));

    QPushButton *pCancelButton = new QPushButton("Cancel");
    QPushButton *pNextButton = new QPushButton("Go!");

    QGridLayout *pLossesDialogLayout = new QGridLayout;
    pLossesDialogLayout->addWidget(pInfoLabel, 0, 0, 1, 2);
    pLossesDialogLayout->addWidget(pIgnoreSmallLossesCheckBox, 1, 0, 1, 2);
    pLossesDialogLayout->addLayout(pSliderLayout, 2, 0, 1, 2);
    pLossesDialogLayout->addWidget(pCancelButton, 3, 0, 1, 1);
    pLossesDialogLayout->addWidget(pNextButton, 3, 1, 1, 1);

    mpLossesDialog->setLayout(pLossesDialogLayout);

    mpLossesDialog->show();

    connect(pCancelButton, SIGNAL(clicked()), mpLossesDialog, SLOT(close()));
    connect(pNextButton, SIGNAL(clicked()), this, SLOT(showLossesFromDialog()));
}


void GUIContainerObject::showLossesFromDialog()
{
    mpLossesDialog->close();
    mLossesVisible=true;

    double limit=0;
    if(mpMinLossesSlider->isEnabled())
    {
        limit=mpMinLossesSlider->value()/100.0;
    }

    //We should not be here if there is no plot data, but let's check to be sure
    if(mPlotData.isEmpty())
    {
        gpMainWindow->mpMessageWidget->printGUIErrorMessage("Attempted to calculate losses for a model that has not been simulated (or is empty).");
        return;
    }

    //Calculate total losses in model
    double totalLosses=0;
    GUIModelObjectMapT::iterator moit;
    for(moit = mGUIModelObjectMap.begin(); moit != mGUIModelObjectMap.end(); ++moit)
    {
        moit.value()->showLosses();
        double componentTotal, componentHydraulic, componentMechanic;
        moit.value()->getLosses(componentTotal, componentHydraulic, componentMechanic);
        if(componentTotal > 0)
            totalLosses += componentTotal;
    }

    //Count number of component that are to be plotted, and store their names
    int nComponents=0;
    QStringList componentNames;
    QList<double> componentLosses;
    for(moit = mGUIModelObjectMap.begin(); moit != mGUIModelObjectMap.end(); ++moit)
    {
        double componentTotal, componentHydraulic, componentMechanic;
        moit.value()->getLosses(componentTotal, componentHydraulic, componentMechanic);
        if(abs(componentTotal) > abs(limit*totalLosses))     //Condition for plotting
        {
            ++nComponents;
            componentNames.append(moit.value()->getName());
            componentLosses.append(componentTotal);
        }
    }

    QStringList lossesNames;
    QList<double> lossesValues;
    QStringList addedNames;
    QList<double> addedValues;
    for(int i=0; i<componentLosses.size(); ++i)
    {
        if(componentLosses.at(i) > 0)
        {
            lossesNames.append(componentNames.at(i));
            lossesValues.append(componentLosses.at(i));
        }
        else
        {
            addedNames.append(componentNames.at(i));
            addedValues.append(componentLosses.at(i));
        }
    }

    //Sort losses for plot (bubblesort)
    int i,j;
    for(i=0; i<lossesValues.size(); i++)
    {
        for(j=0;j<i;j++)
        {
            if(fabs(lossesValues[i])>fabs(lossesValues[j]))
            {
                double temp=lossesValues[i];
                lossesValues[i]=lossesValues[j];
                lossesValues[j]=temp;

                QString temp2 = lossesNames[i];
                lossesNames[i]=lossesNames[j];
                lossesNames[j]=temp2;
            }
        }
    }
    for(i=0; i<addedValues.size(); i++)
    {
        for(j=0;j<i;j++)
        {
            if(fabs(addedValues[i])>fabs(addedValues[j]))
            {
                double temp=addedValues[i];
                addedValues[i]=addedValues[j];
                addedValues[j]=temp;

                QString temp2 = addedNames[i];
                addedNames[i]=addedNames[j];
                addedNames[j]=temp2;
            }
        }
    }

    componentNames.clear();
    componentNames.append(addedNames);
    componentNames.append(lossesNames);

    componentLosses.clear();
    componentLosses.append(addedValues);
    componentLosses.append(lossesValues);

    //Create item model, containing data for bar chart plot
    QStandardItemModel *pItemModel = new QStandardItemModel(2,nComponents,this);
    pItemModel->setHeaderData(0, Qt::Vertical, QColor("crimson"), Qt::BackgroundRole);
    pItemModel->setHeaderData(1, Qt::Vertical, QColor("forestgreen"), Qt::BackgroundRole);

    //Add data to plot bars from each component
    for(int c=0; c<componentLosses.size(); ++c)
    {
        if(abs(componentLosses.at(c)) > abs(limit*totalLosses))
        {
            if(componentLosses.at(c) > 0)
                pItemModel->setData(pItemModel->index(0,c), componentLosses.at(c));
            else
                pItemModel->setData(pItemModel->index(1,c), -componentLosses.at(c));
        }
    }

    pItemModel->setVerticalHeaderLabels(QStringList() << "Added" << "Losses");
    pItemModel->setHorizontalHeaderLabels(componentNames);

    PlotWindow *pPlotWindow = new PlotWindow(gpMainWindow->mpPlotWidget->mpPlotParameterTree, gpMainWindow);
    pPlotWindow->getCurrentPlotTab()->setTabName("Energy Losses");
    pPlotWindow->addBarChart(pItemModel);
    pPlotWindow->show();
}


void GUIContainerObject::hideLosses()
{
    GUIModelObjectMapT::iterator moit;
    for(moit = mGUIModelObjectMap.begin(); moit != mGUIModelObjectMap.end(); ++moit)
    {
        moit.value()->hideLosses();
    }
}


bool GUIContainerObject::isAncestorOfExternalSubsystem()
{
    if(this == mpParentProjectTab->getSystem())
    {
        return false;
    }
    else if(this->isExternal())
    {
        return true;
    }
    else
    {
        return mpParentContainerObject->isAncestorOfExternalSubsystem();
    }
}


bool GUIContainerObject::isExternal()
{
    return !mModelFileInfo.filePath().isEmpty();
}

//! @brief Returns time vector for specified plot generation.
//! @param generation Generation to fetch time vector from
QVector<double> GUIContainerObject::getTimeVector(int generation, QString componentName, QString portName)
{
    return mPlotData.at(generation).find(componentName).value().find(portName).value().begin().value().first;
}


//! @brief Returns plot data for specified variable.
//! @param generation Generation to plot from
//! @param componentName Name of component where variable is located
//! @param portName Name of port where variable is located
//! @param dataName Name of physical quantity of the variable
QVector<double> GUIContainerObject::getPlotData(int generation, QString componentName, QString portName, QString dataName)
{
    ////qDebug() << "Looking for " << generation << ", " << componentName << ", " << portName << ", " << dataName;
    ////qDebug() << "Size of data: " << mPlotData.size();
    return mPlotData.at(generation).find(componentName).value().find(portName).value().find(dataName).value().second;
}


//! @brief Tells whether or not specified component has specified plot generation.
//! It does not if the generation was simulated before this component was added
//! @param generation Generation to look for
//! @param componentName Name of component to look in
bool GUIContainerObject::componentHasPlotGeneration(int generation, QString componentName)
{
    return mPlotData.at(generation).contains(componentName);
}


//! @brief Returns a copy of all existing plot data in container.
//! @note This object is gigantic, and will likey reduce performance if used too often.
QList< QMap< QString, QMap< QString, QMap<QString, QPair<QVector<double>, QVector<double> > > > > > GUIContainerObject::getAllPlotData()
{
    return mPlotData;
}


//! @brief Returns total number of plot generation.
//! I.e. how many times the container has been simulated since opened.
int GUIContainerObject::getNumberOfPlotGenerations()
{
    return mPlotData.size();
}


//! @brief Opens a box and lets user choose new plot alias for specified variable.
//! @param componentName Name of component where variable is located
//! @param portName Name of port where variable is located
//! @param dataName Name of physical quantity of the variable
void GUIContainerObject::definePlotAlias(QString componentName, QString portName, QString dataName)
{
    bool ok;
    QString d = QInputDialog::getText(gpMainWindow, tr("Define Variable Alias"),
                                     tr("Alias:"), QLineEdit::Normal, "", &ok);

    if(ok)
    {
       QString alias = d;
       definePlotAlias(alias, componentName, portName, dataName);
    }
}


//! @brief Defines a new plot alias for specified variable.
//! @param componentName Name of component where variable is located
//! @param portName Name of port where variable is located
//! @param dataName Name of physical quantity of the variable
bool GUIContainerObject::definePlotAlias(QString alias, QString componentName, QString portName, QString dataName)
{
    if(mPlotAliasMap.contains(alias)) return false;
    QStringList variableDescription;
    variableDescription.append(componentName);
    variableDescription.append(portName);
    variableDescription.append(dataName);
    mPlotAliasMap.insert(alias, variableDescription);
    return true;
}


//! @brief Undefines an existing plot alias.
//! @param alias Name of alias to undefine
void GUIContainerObject::undefinePlotAlias(QString alias)
{
    mPlotAliasMap.remove(alias);
}


//! @brief Returns the plot variable for specified alias.
//! @returns A stringlist with componentName, portName and dataName of variable, or null if alias does not exist.
QStringList GUIContainerObject::getPlotVariableFromAlias(QString alias)
{
    if(mPlotAliasMap.contains(alias))
        return mPlotAliasMap.find(alias).value();
    else
        return QStringList();
}


//! @brief Returns plot alias for specified variable.
//! @param componentName Name of component where variable is located
//! @param portName Name of port where variable is located
//! @param dataName Name of physical quantity of the variable
QString GUIContainerObject::getPlotAlias(QString componentName, QString portName, QString dataName)
{
    QStringList variableDescription;
    variableDescription.append(componentName);
    variableDescription.append(portName);
    variableDescription.append(dataName);

    QMap<QString, QStringList>::iterator it;
    for(it=mPlotAliasMap.begin(); it!=mPlotAliasMap.end(); ++it)
    {
        if(it.value() == variableDescription) return it.key();
    }
    return QString();
}
