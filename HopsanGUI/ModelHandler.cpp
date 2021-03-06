/*-----------------------------------------------------------------------------

 Copyright 2017 Hopsan Group

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

 The full license is available in the file GPLv3.
 For details about the 'Hopsan Group' or information about Authors and
 Contributors see the HOPSANGROUP and AUTHORS files that are located in
 the Hopsan source code root directory.

-----------------------------------------------------------------------------*/

//!
//! @file   ModelHandler.cpp
//! @author Robert Braun <robert.braun@liu.se>
//! @date   2013-05-16
//!
//! @brief Contains a model handler object
//!
//$Id$

#include <QFileDialog>
#include <QMessageBox>


//Hopsan includes
#include "common.h"
#include "global.h"
#include "Configuration.h"
#include "DesktopHandler.h"
#include "GraphicsView.h"
#include "GUIObjects/GUISystem.h"
#include "MessageHandler.h"
#include "MainWindow.h"
#include "ModelHandler.h"
#include "SimulationThreadHandler.h"
#include "version_gui.h"
#include "Widgets/DebuggerWidget.h"
#include "MessageHandler.h"
#include "Widgets/LibraryWidget.h"
#include "Widgets/ModelWidget.h"
#include "Widgets/ProjectTabWidget.h"
#include "Widgets/DataExplorer.h"
#include "Widgets/PlotWidget2.h"
#include "Utilities/GUIUtilities.h"

#ifdef USEZMQ
#include "RemoteSimulationUtils.h"
#endif

ModelHandler::ModelHandler(QObject *parent)
    : QObject(parent)
{
    mpDebugger = 0;

    mNumberOfUntitledModels=0;

    mpSimulationThreadHandler = new SimulationThreadHandler(); //!< @todo is this ever deleted

    mCurrentIdx = -1;

    connect(this, SIGNAL(checkMessages()),      gpMessageHandler,    SLOT(collectHopsanCoreMessages()), Qt::UniqueConnection);
}

void ModelHandler::addModelWidget(ModelWidget *pModelWidget, const QString &name, bool detatched)
{
    pModelWidget->setParent(gpCentralTabWidget);    //! @todo Should probably use ModelHandler as parent

    connect(pModelWidget->getTopLevelSystemContainer()->getLogDataHandler(), SIGNAL(dataAddedFromModel(bool)), gpMainWindow->mpShowLossesAction, SLOT(setEnabled(bool)));

    //connect(gpMainWindow->mpDebug2Action, SIGNAL(triggered()), pModelWidget, SLOT(generateModelicaCode()));

    // If the Modelwidget should not be hidden then add it as a tab and switch to that tab
    if(!detatched)
    {
        mModelPtrs.append(pModelWidget);
        mCurrentIdx = mModelPtrs.size()-1;
        gpCentralTabWidget->setCurrentIndex(gpCentralTabWidget->addTab(pModelWidget, name));
        emit newModelWidgetAdded();
        emit modelChanged(pModelWidget);
    }
}


//! @brief Adds a ModelWidget object (a new tab) to itself.
//! @see closeModel(int index)
ModelWidget *ModelHandler::addNewModel(QString modelName, bool hidden)
{
    modelName.append(QString::number(mNumberOfUntitledModels));

    ModelWidget *pNewModelWidget = new ModelWidget(this,gpCentralTabWidget);    //! @todo Should probably use ModelHandler as parent
    pNewModelWidget->getTopLevelSystemContainer()->setName(modelName);

    connect(pNewModelWidget->getTopLevelSystemContainer()->getLogDataHandler(), SIGNAL(dataAddedFromModel(bool)), gpMainWindow->mpShowLossesAction, SLOT(setEnabled(bool)));

    addModelWidget(pNewModelWidget, modelName, hidden);

    pNewModelWidget->setSaved(true);
    mNumberOfUntitledModels += 1;

    return pNewModelWidget;
}


void ModelHandler::setCurrentModel(int idx)
{
    if ( (idx>=0) && (idx<mModelPtrs.size()) )
    {
        mCurrentIdx = idx;
        // Also switch tab if it is visible among the tabs
        if(gpCentralTabWidget->indexOf(mModelPtrs[idx]) != -1)
            gpCentralTabWidget->setCurrentWidget(mModelPtrs[idx]);
        refreshMainWindowConnections();
    }
}


void ModelHandler::setCurrentModel(ModelWidget *pWidget)
{
    setCurrentModel(mModelPtrs.indexOf(pWidget));
}

ModelWidget *ModelHandler::getModel(const QString &rModelFilePath)
{
    for (int i=0; i<mModelPtrs.size(); ++i)
    {
        if (mModelPtrs[i]->getTopLevelSystemContainer()->getModelFilePath() == rModelFilePath)
        {
            return mModelPtrs[i];
        }
    }
    return 0;
}


ModelWidget *ModelHandler::getModel(int idx)
{
    if( (idx >= 0) && (idx < mModelPtrs.size()) )
    {
        return mModelPtrs[idx];
    }
    return 0;
}


ModelWidget *ModelHandler::getCurrentModel()
{
    return getModel(mCurrentIdx);
}

SystemContainer *ModelHandler::getTopLevelSystem(const QString &rModelFilePath)
{
    ModelWidget *pMW = getModel(rModelFilePath);
    if (pMW)
    {
        return pMW->getTopLevelSystemContainer();
    }
    return 0;
}

SystemContainer *ModelHandler::getTopLevelSystem(int idx)
{
    ModelWidget *pMW = getModel(idx);
    if (pMW)
    {
        return pMW->getTopLevelSystemContainer();
    }
    return 0;
}

SystemContainer *ModelHandler::getCurrentTopLevelSystem()
{
    ModelWidget *pMW = getCurrentModel();
    if (pMW)
    {
        return pMW->getTopLevelSystemContainer();
    }
    return 0;
}

ContainerObject *ModelHandler::getViewContainerObject(int idx)
{
    ModelWidget *pMW = getModel(idx);
    if (pMW)
    {
        return pMW->getViewContainerObject();
    }
    return 0;
}

ContainerObject *ModelHandler::getCurrentViewContainerObject()
{
    ModelWidget *pMW = getCurrentModel();
    if (pMW)
    {
        return pMW->getViewContainerObject();
    }
    return 0;
}

LogDataHandler2 *ModelHandler::getCurrentLogDataHandler()
{
    ModelWidget *pMW = getCurrentModel();
    if (pMW)
    {
        return pMW->getLogDataHandler();
    }
    return 0;
}

int ModelHandler::count() const
{
    return mModelPtrs.size();
}


//! @brief Loads a model from a file and opens it in a new project tab.
//! @see loadModel(QString modelFileName)
//! @see Model(saveTarget saveAsFlag)
void ModelHandler::loadModel()
{
    QDir fileDialogOpenDir;
    QStringList modelFileNames = QFileDialog::getOpenFileNames(gpMainWindowWidget, tr("Choose Model File"),
                                                         gpConfig->getStringSetting(CFG_LOADMODELDIR),
                                                         tr("Hopsan Model Files (*.hmf *.xml)"));
    foreach(const QString &modelFileName, modelFileNames)
    {
        loadModel(modelFileName);
        QFileInfo fileInfo = QFileInfo(modelFileName);
        gpConfig->setStringSetting(CFG_LOADMODELDIR, fileInfo.absolutePath());
    }
}


//! @brief Help function that loads a model from the text in a QAction object.
//! Used to facilitate recent models function.
void ModelHandler::loadModel(QAction *action)
{
    loadModel(action->text());
}


void ModelHandler::loadModelParameters()
{
    qobject_cast<SystemContainer*>(getCurrentViewContainerObject())->loadParameterFile();
}


//! @brief Loads a model from a file and opens it in a new project tab.
//! @param modelFileName is the path to the loaded file
//! @see loadModel()
//! @see saveModel(saveTarget saveAsFlag)
ModelWidget *ModelHandler::loadModel(QString modelFileName, bool ignoreAlreadyOpen, bool detatched)
{
    //! @todo maybe  write utility function that opens file, checks existence and sets fileinfo
    QFile modelFile(modelFileName);   //Create a QFile object
    if(!modelFile.exists())
    {
        gpMessageHandler->addErrorMessage("File not found: " + modelFile.fileName());
        return 0;
    }
    QFileInfo modelFileInfo(modelFile);

    // Make sure file not already open
    if(!ignoreAlreadyOpen)
    {
        for(int t=0; t!=mModelPtrs.size(); ++t)
        {
            if(this->getTopLevelSystem(t)->getModelFileInfo().filePath() == modelFileInfo.filePath() && gpCentralTabWidget->indexOf(mModelPtrs[t]) > -1)
            {
                QMessageBox::information(gpMainWindowWidget, tr("Error"), tr("Unable to load model. File is already open."));
                return nullptr;
            }
        }
    }

    ModelWidget *pNewModel = new ModelWidget(this, gpCentralTabWidget);
    connect(pNewModel->getSimulationThreadHandler(), SIGNAL(startSimulation()), gpMainWindow, SLOT(hideSimulateButton()));
    connect(pNewModel->getSimulationThreadHandler(), SIGNAL(done(bool)), gpMainWindow, SLOT(showSimulateButton()));

    if(!detatched)
    {
        gpMessageHandler->addInfoMessage("Loading model: "+modelFileInfo.absoluteFilePath());
    }

   addModelWidget(pNewModel, modelFileInfo.baseName(), detatched);
   bool loadOK = pNewModel->loadModel(modelFile);
   if (loadOK)
   {
       emit newModelWidgetAdded();
       if(!detatched)
       {
           emit modelChanged(pNewModel);
           gpConfig->addRecentModel(modelFileInfo.filePath());
       }
       return pNewModel;
   }
   else
   {
        closeModel(pNewModel);
        return nullptr;
   }
}


bool ModelHandler::closeModelByTabIndex(int tabIdx, bool force)
{
    for(int i=0; i<mModelPtrs.size(); ++i)
    {
        // When found close and return, else return false when loop ends
        if(mModelPtrs[i] == gpCentralTabWidget->widget(tabIdx))
        {
            return closeModel(i,force);
        }
    }

    gpCentralTabWidget->removeTab(tabIdx);
    return false;
}


bool ModelHandler::closeModel(ModelWidget *pModel, bool force)
{
    return closeModel(mModelPtrs.indexOf(pModel), force);
}


//! @brief Closes current project.
//! @param idx defines which project to close.
//! @param force Do not ask for saving model before closing
//! @return true if closing went ok. false if the user canceled the operation.
//! @see closeAllModels()
bool ModelHandler::closeModel(int idx, bool force)
{
    // Only remove if we found the model by index
    ModelWidget *pModelToClose = getModel(idx);
    if(pModelToClose)
    {
        if (!pModelToClose->isSaved() && !force)
        {
            QString modelName = pModelToClose->getTopLevelSystemContainer()->getName();
            QMessageBox msgBox;
            msgBox.setWindowIcon(gpMainWindowWidget->windowIcon());
            msgBox.setText(QString("The model '").append(modelName).append("'  is not saved."));
            msgBox.setInformativeText("Do you want to save your changes before closing?");
            msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Save);

            int answer = msgBox.exec();
            switch (answer)
            {
            case QMessageBox::Save:
                // Save was clicked
                pModelToClose->save();
                break;
            case QMessageBox::Discard:
                // Don't Save was clicked
                break;
            case QMessageBox::Cancel:
                // Cancel was clicked
                return false;
            default:
                // should never be reached
                return false;
            }
        }


//        if (pModelToClose->getTopLevelSystemContainer()->getLogDataHandler()->hasOpenPlotCurves())
//        {
//            QMessageBox msgBox;
//            msgBox.setWindowIcon(gpMainWindow->windowIcon());
//            msgBox.setText(QString("All open plot curves from this model will be lost."));
//            msgBox.setInformativeText("Are you sure you want to close?");
//            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
//            msgBox.setDefaultButton(QMessageBox::Cancel);

//            int answer = msgBox.exec();
//            switch (answer)
//            {
//            case QMessageBox::Ok:
//                // Ok was clicked
//                getTopLevelSystem(idx)->getLogDataHandler()->closePlotsWithCurvesBasedOnOwnedData();
//                break;
//            case QMessageBox::Cancel:
//                // Cancel was clicked
//                return false;
//            default:
//                // should never be reached
//                return false;
//            }
//        }

        // Disconnect signals
        disconnectMainWindowConnections(pModelToClose);
//        pModelToClose->getViewContainerObject()->unmakeMainWindowConnectionsAndRefresh();

//        // Deactivate Undo to prevent each component from registering it being deleted in the undo stack (waste of time)
//        pModelToClose->getViewContainerObject()->setUndoEnabled(false, true);

        // Disconnect and delete model tab if any
        int tabIdx = gpCentralTabWidget->indexOf(pModelToClose);
        if (tabIdx >= 0)
        {
            // Disconnect all signals from this, to prevent a model change will be triggered when the tab is removed
            disconnect(gpCentralTabWidget->widget(tabIdx));
            // Remove the tab
            gpCentralTabWidget->removeTab(tabIdx);
        }

        // Remove and delete the model
        delete pModelToClose; //!< @todo it is very important (right now) that we delete before remove and --mCurrentIdx, since the delete will cause (undowidget trying to refresh from current widget) we can not remove the widgets before it has been deleted because of this. This behavior is really BAD and should be fixed. The destructor indirectly requires the use of one self by triggering a function in the undo widget
        mModelPtrs.removeAt(idx);

        // When a model widget is removed all previous indexes for later models will become incorrect,
        // lets set the new current to the latest in that case
        if (mCurrentIdx >= idx)
        {
            --mCurrentIdx;
        }

        // Refresh mainwindow sig/slot connections and button status to the new current model
        refreshMainWindowConnections();

        // Refresh toolbar connections if tab has been changed
        gpMainWindow->updateToolBarsToNewTab();

        // We are done removing the model widget
        return true;
    }
    return false;
}


//! @brief Closes all opened projects.
//! @return true if closing went ok. false if the user canceled the operation.
//! @param force Do not ask for saving model before closing
//! @see closeModel(int index)
//! @see saveModel()
bool ModelHandler::closeAllModels(bool force)
{
    gpConfig->clearLastSessionModels();

    while(mModelPtrs.size() > 0)
    {
        gpConfig->addLastSessionModel(mModelPtrs.last()->getViewContainerObject()->getModelFileInfo().filePath());
        if (!closeModel(mModelPtrs.size()-1,force))
        {
            return false;
        }
    }
    return true;
}

void ModelHandler::selectModelByTabIndex(int tabIdx)
{
    // Default is to have no current model, in case we click a tab that is not actually a model
    mCurrentIdx = -1;
    // Find which model is selected (if any)
    for(int i=0; i<mModelPtrs.size(); ++i)
    {
        if(mModelPtrs[i] == gpCentralTabWidget->widget(tabIdx))
        {
            mCurrentIdx=i;
        }
    }
    refreshMainWindowConnections();
}


void ModelHandler::refreshMainWindowConnections()
{
    // First disconnect connections from all models
    for(int i=0; i<mModelPtrs.size(); ++i)
    {
        disconnectMainWindowConnections(getModel(i));
        getViewContainerObject(i)->unmakeMainWindowConnectionsAndRefresh();
    }
    // Now refresh actions (we need to do this before reconnecting to avoid sending signals)
    ModelWidget *pCurrentModel = getCurrentModel();
    if (pCurrentModel)
    {
        gpMainWindow->mpToggleRemoteCoreSimAction->setChecked(pCurrentModel->getUseRemoteSimulationCore());
        gpMainWindow->mpDataExplorer->setLogdataHandler(pCurrentModel->getLogDataHandler());
        gpPlotWidget->setLogDataHandler(pCurrentModel->getLogDataHandler());
    }
    else
    {
        gpMainWindow->mpToggleRemoteCoreSimAction->setChecked(false);
        gpMainWindow->mpDataExplorer->setLogdataHandler(nullptr);
        if (gpPlotWidget)
        {
            gpPlotWidget->setLogDataHandler(nullptr);
        }
    }
    // Now reestablish connections to new model
    if(pCurrentModel)
    {
        connectMainWindowConnections(pCurrentModel);
        getCurrentViewContainerObject()->makeMainWindowConnectionsAndRefresh();
        getCurrentViewContainerObject()->updateMainWindowButtons();

        setToolBarSimulationTimeFromTab(pCurrentModel);

        if(gpLibraryWidget->mGfxType != pCurrentModel->getTopLevelSystemContainer()->getGfxType())
        {
            gpLibraryWidget->setGfxType(pCurrentModel->getTopLevelSystemContainer()->getGfxType());
        }
    }

    emit modelChanged(pCurrentModel);
}

void ModelHandler::disconnectMainWindowConnections(ModelWidget *pModel)
{
    //! @todo  Are these connections such connection that are supposed to be permanent connections? otherwise they should be in the disconnectMainWindowActions function
    disconnect(gpMainWindow->mpResetZoomAction,     SIGNAL(triggered()),    pModel->getGraphicsView(),   SLOT(resetZoom()));
    disconnect(gpMainWindow->mpZoomInAction,        SIGNAL(triggered()),    pModel->getGraphicsView(),   SLOT(zoomIn()));
    disconnect(gpMainWindow->mpZoomOutAction,       SIGNAL(triggered()),    pModel->getGraphicsView(),   SLOT(zoomOut()));
    disconnect(gpMainWindow->mpPrintAction,         SIGNAL(triggered()),    pModel->getGraphicsView(),   SLOT(print()));
    disconnect(gpMainWindow->mpExportPDFAction,     SIGNAL(triggered()),    pModel->getGraphicsView(),   SLOT(exportToPDF()));
    disconnect(gpMainWindow->mpExportPNGAction,     SIGNAL(triggered()),    pModel->getGraphicsView(),   SLOT(exportToPNG()));
    disconnect(gpMainWindow->mpCenterViewAction,    SIGNAL(triggered()),    pModel->getGraphicsView(),   SLOT(centerView()));

    disconnect(gpMainWindow->mpSimulationTimeEdit,          SIGNAL(simulationTimeChanged(QString,QString,QString)), pModel, SLOT(setTopLevelSimulationTime(QString,QString,QString)));
    disconnect(pModel, SIGNAL(simulationTimeChanged(QString,QString,QString)), gpMainWindow->mpSimulationTimeEdit, SLOT(displaySimulationTime(QString,QString,QString)));

    disconnect(gpMainWindow,                                SIGNAL(simulateKeyPressed()),   pModel,  SLOT(simulate_nonblocking()));
    disconnect(gpMainWindow->mpCoSimulationAction,          SIGNAL(triggered()),            pModel,  SLOT(startCoSimulation()));
    disconnect(gpMainWindow->mpToggleRemoteCoreSimAction,   SIGNAL(triggered(bool)),        pModel,  SLOT(setUseRemoteSimulation(bool)));
    disconnect(gpMainWindow->mpSaveAction,                  SIGNAL(triggered()),            pModel,  SLOT(save()));
    disconnect(gpMainWindow->mpSaveAsAction,                SIGNAL(triggered()),            pModel,  SLOT(saveAs()));
    disconnect(gpMainWindow->mpExportModelParametersAction, SIGNAL(triggered()),            pModel,  SLOT(exportModelParameters()));
    disconnect(gpMainWindow->mpExportSimulationStateAction, SIGNAL(triggered()),            pModel,  SLOT(exportSimulationStates()));

    connect(pModel,                                         SIGNAL(modelSaved(ModelWidget*)),           SIGNAL(modelChanged(ModelWidget*)));
}

void ModelHandler::connectMainWindowConnections(ModelWidget *pModel)
{
    //! @todo  Are these connections such connection that are supposed to be permanent connections? otherwise they should be in the disconnectMainWindowActions function
    connect(gpMainWindow->mpResetZoomAction,    SIGNAL(triggered()),    pModel->getGraphicsView(),    SLOT(resetZoom()), Qt::UniqueConnection);
    connect(gpMainWindow->mpZoomInAction,       SIGNAL(triggered()),    pModel->getGraphicsView(),    SLOT(zoomIn()), Qt::UniqueConnection);
    connect(gpMainWindow->mpZoomOutAction,      SIGNAL(triggered()),    pModel->getGraphicsView(),    SLOT(zoomOut()), Qt::UniqueConnection);
    connect(gpMainWindow->mpPrintAction,        SIGNAL(triggered()),    pModel->getGraphicsView(),    SLOT(print()), Qt::UniqueConnection);
    connect(gpMainWindow->mpExportPDFAction,    SIGNAL(triggered()),    pModel->getGraphicsView(),    SLOT(exportToPDF()), Qt::UniqueConnection);
    connect(gpMainWindow->mpExportPNGAction,    SIGNAL(triggered()),    pModel->getGraphicsView(),    SLOT(exportToPNG()), Qt::UniqueConnection);
    connect(gpMainWindow->mpCenterViewAction,   SIGNAL(triggered()),    pModel->getGraphicsView(),    SLOT(centerView()), Qt::UniqueConnection);

    connect(gpMainWindow->mpSimulationTimeEdit, SIGNAL(simulationTimeChanged(QString,QString,QString)), pModel, SLOT(setTopLevelSimulationTime(QString,QString,QString)), Qt::UniqueConnection);
    connect(pModel, SIGNAL(simulationTimeChanged(QString,QString,QString)), gpMainWindow->mpSimulationTimeEdit, SLOT(displaySimulationTime(QString,QString,QString)), Qt::UniqueConnection);

    connect(gpMainWindow,                                   SIGNAL(simulateKeyPressed()),   pModel,    SLOT(simulate_nonblocking()), Qt::UniqueConnection);
    connect(gpMainWindow->mpCoSimulationAction,             SIGNAL(triggered()),            pModel,    SLOT(startCoSimulation()), Qt::UniqueConnection);
    connect(gpMainWindow->mpToggleRemoteCoreSimAction,      SIGNAL(triggered(bool)),        pModel,    SLOT(setUseRemoteSimulation(bool)), Qt::UniqueConnection);
    connect(gpMainWindow->mpSaveAction,                     SIGNAL(triggered()),            pModel,    SLOT(save()), Qt::UniqueConnection);
    connect(gpMainWindow->mpSaveAsAction,                   SIGNAL(triggered()),            pModel,    SLOT(saveAs()), Qt::UniqueConnection);
    connect(gpMainWindow->mpExportModelParametersAction,    SIGNAL(triggered()),            pModel,    SLOT(exportModelParameters()), Qt::UniqueConnection);
    connect(gpMainWindow->mpExportSimulationStateAction,    SIGNAL(triggered()),            pModel,    SLOT(exportSimulationStates()), Qt::UniqueConnection);

    connect(pModel,                                         SIGNAL(modelSaved(ModelWidget*)),           SIGNAL(modelChanged(ModelWidget*)));
}

//! @brief Help function to update the toolbar simulation time parameters from a tab
void ModelHandler::setToolBarSimulationTimeFromTab(ModelWidget *pModel)
{
    gpMainWindow->mpSimulationTimeEdit->displaySimulationTime(pModel->getStartTime(), pModel->getTimeStep(), pModel->getStopTime());
}

void ModelHandler::saveState()
{
    mStateInfoBackupList.clear();
    mStateInfoHasChanged.clear();
    mStateInfoHmfList.clear();
    mStateInfoModels.clear();
    mStateInfoTabNames.clear();
    mStateInfoLogDataHandlersList.clear();

    while(!mModelPtrs.isEmpty())
    {
        ModelWidget *pModel = getModel(0);
        mStateInfoHmfList << pModel->getTopLevelSystemContainer()->getModelFileInfo().filePath();
        mStateInfoHasChanged << !pModel->isSaved();
        mStateInfoTabNames << gpCentralTabWidget->tabText(gpCentralTabWidget->indexOf(pModel));
        pModel->getTopLevelSystemContainer()->getLogDataHandler()->setParent(0);       //Make sure it is not removed when deleting the container object
        mStateInfoLogDataHandlersList << pModel->getTopLevelSystemContainer()->getLogDataHandler();
        if(!pModel->isSaved())
        {
            //! @todo This code is duplicated from ModelWidget::saveModel(), make it a common function somehow
                //Save xml document
            QDomDocument domDocument;
            QDomElement hmfRoot = appendHMFRootElement(domDocument, HMF_VERSIONNUM, HOPSANGUIVERSION, getHopsanCoreVersion());
            pModel->getTopLevelSystemContainer()->saveToDomElement(hmfRoot);
            QString fileNameWithoutHmf = getCurrentTopLevelSystem()->getModelFileInfo().fileName();
            fileNameWithoutHmf.chop(4);
            mStateInfoBackupList << gpDesktopHandler->getBackupPath()+fileNameWithoutHmf+"_savedstate.hmf";
            QFile xmlhmf(gpDesktopHandler->getBackupPath()+fileNameWithoutHmf+"_savedstate.hmf");
            if (!xmlhmf.open(QIODevice::WriteOnly | QIODevice::Text))  //open file
            {
                return;
            }
            QTextStream out(&xmlhmf);
            appendRootXMLProcessingInstruction(domDocument); //The xml "comment" on the first line
            domDocument.save(out, XMLINDENTATION);
            xmlhmf.close();
            pModel->setSaved(true);
            closeModel(0);
            //pTab->close();
        }
        else
        {
            mStateInfoBackupList << "";
            closeModel(0);
            //pTab->close();
        }
    }
}

void ModelHandler::restoreState()
{
    for(int i=0; i<mStateInfoHmfList.size(); ++i)
    {
        if(mStateInfoHasChanged[i])
        {
            loadModel(mStateInfoBackupList[i]);
            getCurrentModel()->hasChanged();
            getCurrentTopLevelSystem()->setModelFile(mStateInfoHmfList[i]);
            QString basePath = QFileInfo(mStateInfoHmfList[i]).absolutePath();
            QStringListIterator objIt(getCurrentTopLevelSystem()->getModelObjectNames());
//            while (objIt.hasNext())
//            {
//                //getCurrentTopLevelSystem()->getModelObject(objIt.next())->getAppearanceData()->setBasePath(basePath);
//            }
        }
        else
        {
            loadModel(mStateInfoHmfList[i]);
        }
        if(mModelPtrs.size() < i+1)
        {
            addNewModel();
        }
        gpCentralTabWidget->setCurrentIndex(i+1);
        this->mCurrentIdx = i;
        gpCentralTabWidget->setTabText(i+1, mStateInfoTabNames[i]);

        //! @todo FIXA /Peter
//        getCurrentTopLevelSystem()->setLogDataHandler(mStateInfoLogDataHandlersList[i]);
//        mStateInfoLogDataHandlersList[i]->setParentContainerObject(getCurrentTopLevelSystem());
    }
}

void ModelHandler::revertCurrentModel()
{
    ModelWidget *pModel = gpModelHandler->getCurrentModel();
    if (pModel)
    {
        pModel->revertModel();
        refreshMainWindowConnections();
    }
}

void ModelHandler::createLabviewWrapperFromCurrentModel()
{
    qobject_cast<SystemContainer*>(getCurrentViewContainerObject())->exportToLabView();
}


void ModelHandler::exportCurrentModelToFMU1_32()
{
    qobject_cast<SystemContainer*>(getCurrentViewContainerObject())->exportToFMU1_32();
}

void ModelHandler::exportCurrentModelToFMU1_64()
{
    qobject_cast<SystemContainer*>(getCurrentViewContainerObject())->exportToFMU1_64();
}

void ModelHandler::exportCurrentModelToFMU2_32()
{
    qobject_cast<SystemContainer*>(getCurrentViewContainerObject())->exportToFMU2_32();
}

void ModelHandler::exportCurrentModelToFMU2_64()
{
    qobject_cast<SystemContainer*>(getCurrentViewContainerObject())->exportToFMU2_64();
}

void ModelHandler::exportCurrentModelToSimulink()
{
    qobject_cast<SystemContainer*>(getCurrentViewContainerObject())->exportToSimulink();
}


void ModelHandler::exportCurrentModelToSimulinkCoSim()
{
    qobject_cast<SystemContainer*>(getCurrentViewContainerObject())->exportToSimulinkCoSim();
}


void ModelHandler::showLosses(bool show)
{
    qobject_cast<SystemContainer*>(getCurrentViewContainerObject())->showLosses(show);
}


void ModelHandler::measureSimulationTime()
{
    qobject_cast<SystemContainer*>(getCurrentViewContainerObject())->measureSimulationTime();
}


void ModelHandler::launchDebugger()
{
    if(getCurrentTopLevelSystem() == 0) return;

    if(!mpDebugger || !mpDebugger->isVisible())
    {
        mpDebugger = new DebuggerWidget(getCurrentModel(), gpMainWindow);
        mpDebugger->show();
        mpDebugger->exec();
    }
}


void ModelHandler::openAnimation()
{
    if(!mModelPtrs.isEmpty())
    {
        getCurrentModel()->openAnimation();
    }
}


bool ModelHandler::simulateAllOpenModels_nonblocking(bool modelsHaveNotChanged)
{
    if(!mModelPtrs.isEmpty())
    {
        QVector<ModelWidget*> models = mModelPtrs.toVector();
        // Make sure that current model is first, as that will be used for simulation time settings
        int i = models.indexOf(getCurrentModel());
        if (i >= 0)
        {
            models.remove(i);
        }
        models.push_front(getCurrentModel());
        return simulateMultipleModels_nonblocking(models, modelsHaveNotChanged);
    }
    return false;
}


bool ModelHandler::simulateAllOpenModels_blocking(bool modelsHaveNotChanged)
{
    if(!mModelPtrs.isEmpty())
    {
        QVector<ModelWidget*> models = mModelPtrs.toVector();
        // Make sure that current model is first, as that will be used for simulation time settings
        int i = models.indexOf(getCurrentModel());
        if (i >= 0)
        {
            models.remove(i);
        }
        models.push_front(getCurrentModel());
        return simulateMultipleModels_blocking(models, modelsHaveNotChanged);
    }
    return false;
}



bool ModelHandler::simulateMultipleModels_nonblocking(QVector<ModelWidget*> models, bool noChange)
{
    if (!models.isEmpty())
    {
        // All systems will use start time, stop time and time step from the first model
        double startTime = models.first()->getStartTime().toDouble();
        double stopTime = models.first()->getStopTime().toDouble();
        size_t nSamples = models.first()->getTopLevelSystemContainer()->getNumberOfLogSamples();
        double logStartT = models.first()->getTopLevelSystemContainer()->getLogStartTime();

        QVector<SystemContainer*> systemsVector;
        bool lockOK=false;
        int i;
        for(i=0; i<models.size(); ++i)
        {
            // Append system vector
            systemsVector.append(models[i]->getTopLevelSystemContainer());

            // Lock simulation mutex
            lockOK = models[i]->mSimulateMutex.tryLock();
            if (lockOK)
            {
                // Create relay connection, since we are using the external simulation thread handler
                connect(mpSimulationThreadHandler, SIGNAL(done(bool)), models[i], SIGNAL(simulationFinished()));
            }
            else
            {
                break;
            }
        }
        if (!lockOK)
        {
            // Unlock again
            for (int j=0; j<i; ++j)
            {
                // Remove and previously made relay connections
                disconnect(mpSimulationThreadHandler, SIGNAL(done(bool)), models[j], SIGNAL(simulationFinished()));
                models[j]->mSimulateMutex.unlock();
            }
            // Return with failure
            return false;
        }

        // Set simulation time and start simulation thread
        mpSimulationThreadHandler->setSimulationTimeVariables(startTime, stopTime, logStartT, nSamples);
        mpSimulationThreadHandler->initSimulateFinalize(systemsVector, noChange);
        return mpSimulationThreadHandler->wasSuccessful();
    }
    return false;
}



bool ModelHandler::simulateMultipleModels_blocking(QVector<ModelWidget*> models, bool noChanges)
{
    TicToc tictoc;
    if (!models.isEmpty())
    {
        // All systems will use start time, stop time and time step from the first model
        double startTime = models.first()->getStartTime().toDouble();
        double stopTime = models.first()->getStopTime().toDouble();
        size_t nSamples = models.first()->getTopLevelSystemContainer()->getNumberOfLogSamples();
        double logStartT = models.first()->getTopLevelSystemContainer()->getLogStartTime();

        QVector<SystemContainer*> systemsVector;
        bool lockOK=false;
        int i;
        for(i=0; i<models.size(); ++i)
        {
            // Append system vector
            systemsVector.append(models[i]->getTopLevelSystemContainer());

            // Lock simulation mutex
            lockOK = models[i]->mSimulateMutex.tryLock();
            if (lockOK)
            {
                // Create relay connection, since we are using the external simulation thread handler
                connect(mpSimulationThreadHandler, SIGNAL(done(bool)), models[i], SIGNAL(simulationFinished()));
            }
            else
            {
                break;
            }
        }
        if (!lockOK)
        {
            // Unlock again
            for (int j=0; j<i; ++j)
            {
                // Remove and previously made relay connections
                disconnect(mpSimulationThreadHandler, SIGNAL(done(bool)), models[j], SIGNAL(simulationFinished()));
                models[j]->mSimulateMutex.unlock();
            }
            // Return with failure
            return false;
        }

        mpSimulationThreadHandler->setSimulationTimeVariables(startTime, stopTime, logStartT, nSamples);
        mpSimulationThreadHandler->setProgressDilaogBehaviour(true, false);
        mpSimulationThreadHandler->initSimulateFinalize_blocking(systemsVector, noChanges);
        tictoc.toc("simulateMultipleModels_blocking()");
        return mpSimulationThreadHandler->wasSuccessful();
    }

    tictoc.toc("simulateMultipleModels_blocking()");
    return false;
}
