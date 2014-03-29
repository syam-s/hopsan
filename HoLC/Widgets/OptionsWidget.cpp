#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QFileDialog>

#include "OptionsWidget.h"
#include "Configuration.h"

OptionsWidget::OptionsWidget(Configuration *pConfiguration, QWidget *parent) :
    QWidget(parent)
{
    mpConfiguration = pConfiguration;

    QFont font = this->font();
    font.setBold(true);
    QLabel *pOptionsLabel = new QLabel("Options", this);
    pOptionsLabel->setFont(font);

    QLabel *pHopsanDirLabel = new QLabel("Hopsan Path:");
    mpHopsanDirLineEdit = new QLineEdit(this);
    QToolButton *pHopsanDirButton = new QToolButton(this);
    pHopsanDirButton->setIcon(QIcon("../HopsanGUI/graphics/uiicons/Hopsan-Open.png"));

    QLabel *pLibraryLabel = new QLabel("Library File:");
    mpLibraryLineEdit = new QLineEdit(this);
    mpLibraryLineEdit->setDisabled(true);

    QLabel *pIncludeLabel = new QLabel("Include Directory:");
    mpIncludeLineEdit = new QLineEdit(this);
    mpIncludeLineEdit->setDisabled(true);

    mpWarningLabel = new QLabel(this);
    mpWarningLabel->setText("<font color='red'>Warning! Library file or include files not found in specified directory!</font>");

    QLabel *pCompilerLabel = new QLabel("Compiler Path:");
    mpCompilerLineEdit = new QLineEdit(this);
    QToolButton *pCompilerButton = new QToolButton(this);
    pCompilerButton->setIcon(QIcon("../HopsanGUI/graphics/uiicons/Hopsan-Open.png"));

    mpCompilerWarningLabel = new QLabel(this);
    mpCompilerWarningLabel->setText("<font color='red'>Warning! GCC compiler not found in specified location!</font>");

    //Setup layout
    QGridLayout *pLayout = new QGridLayout(this);
    pLayout->addWidget(pOptionsLabel,           0,0,1,3);
    pLayout->addWidget(pHopsanDirLabel,         1,0,1,1);
    pLayout->addWidget(mpHopsanDirLineEdit,      1,1,1,1);
    pLayout->addWidget(pHopsanDirButton,        1,2,1,1);
    pLayout->addWidget(pLibraryLabel,           2,0,1,1);
    pLayout->addWidget(mpLibraryLineEdit,        2,1,1,2);
    pLayout->addWidget(pIncludeLabel,           3,0,1,1);
    pLayout->addWidget(mpIncludeLineEdit,        3,1,1,2);
    pLayout->addWidget(mpWarningLabel,           4,0,1,3);
    pLayout->addWidget(pCompilerLabel,          5,0,1,1);
    pLayout->addWidget(mpCompilerLineEdit,       5,1,1,1);
    pLayout->addWidget(pCompilerButton,         5,2,1,1);
    pLayout->addWidget(mpCompilerWarningLabel,   6,0,1,3);
    pLayout->addWidget(new QWidget(this),       7,0,1,3);
    pLayout->setRowStretch(9,1);

    //Setup connections
    connect(pHopsanDirButton, SIGNAL(clicked()), this, SLOT(setHopsanPath()));
    connect(pCompilerButton, SIGNAL(clicked()), this, SLOT(setCompilerPath()));

    if(!mpConfiguration->getCompilerPath().isEmpty())
    {
        mpCompilerLineEdit->setText(mpConfiguration->getCompilerPath());
        mpCompilerWarningLabel->hide();
    }

    //Load paths from configuration
    setCompilerPath(mpConfiguration->getCompilerPath());
    setHopsanPath(mpConfiguration->getHopsanPath());
}


void OptionsWidget::setHopsanPath()
{
    QString path = QFileDialog::getExistingDirectory(this, "Set Hopsan Path:");

    if(path.isEmpty()) return;

    setHopsanPath(path);
}

void OptionsWidget::setHopsanPath(const QString &path)
{
    bool success = true;
    QString lib, includeDir;
#ifdef WIN32
    lib = path+"/bin/HopsanCore.dll";
#else
    lib = path+"/bin/libHopsanCore.so";
#endif
    if(!QFile::exists(lib))
    {
        success = false;
    }
    includeDir = path+"/HopsanCore/include";
    if(!QFile::exists(includeDir+"/HopsanCore.h"))
    {
        success = false;
    }

    mpHopsanDirLineEdit->setText(path);
    mpConfiguration->setHopsanPath(path);
    if(success)
    {
        //mpOptionsHandler->setLibPath(lib);
        //mpOptionsHandler->setIncludePath(includeDir);
        mpLibraryLineEdit->setText(lib);
        mpIncludeLineEdit->setText(includeDir);
        mpWarningLabel->setVisible(false);
    }
    else
    {
        //mpOptionsHandler->setLibPath(QString());
        //mpOptionsHandler->setIncludePath(QString());
        mpLibraryLineEdit->clear();
        mpIncludeLineEdit->clear();
        mpWarningLabel->setVisible(true);
    }
}

void OptionsWidget::setCompilerPath()
{
    QString path = QFileDialog::getExistingDirectory(this, "Set Compiler Path:");

    if(path.isEmpty()) return;

    setCompilerPath(path);
}

void OptionsWidget::setCompilerPath(const QString &path)
{
    QString filePath;
#ifdef linux
    filePath = path+"/gcc";

#else
    filePath = path+"/gcc.exe";
#endif
    //! @todo We should also check that it is the correct version of gcc!

    mpConfiguration->setCompilerPath(path);
    if(QFile::exists(filePath))
    {
        //mpOptionsHandler->setCompilerPath(filePath);
        mpCompilerLineEdit->setText(filePath);
        mpCompilerWarningLabel->hide();
    }
    else
    {
        //mpOptionsHandler->setCompilerPath("");
        mpCompilerLineEdit->clear();
        mpCompilerWarningLabel->show();
    }
}