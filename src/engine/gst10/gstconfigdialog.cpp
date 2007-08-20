// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#include "gstconfig.h"
#include "gstconfigdialog.h"
#include "gstengine.h"

#include <QCheckBox>
#include <QLabel>

#include <kcombobox.h>
#include <kdebug.h>
#include <klineedit.h>
#include <klocale.h>
#include <knuminput.h>


GstConfigDialog::GstConfigDialog( GstEngine const * const engine )
    : PluginConfig()
    , m_engine( (GstEngine*)engine )
{
    kDebug() ;

    m_view = new GstConfigDialogBase();

    // Initialise widgets with current settings
    const QStringList outputs = m_engine->getOutputsList();
    m_view->kComboBox_output->insertStringList( outputs );

    if ( outputs.contains( GstConfig::soundOutput() ) )
        m_view->kComboBox_output->setCurrentText( GstConfig::soundOutput() );

    m_view->checkBox_outputDevice->setChecked( GstConfig::useCustomSoundDevice() );
    m_view->kLineEdit_outputDevice->setText( GstConfig::soundDevice() );

    m_view->checkBox_outputParams->setChecked( GstConfig::useCustomOutputParams() );
    m_view->kLineEdit_outputParams->setText( GstConfig::outputParams() );

    // Connections for modification check
    connect( m_view->kComboBox_output, SIGNAL( activated( int ) ), SIGNAL( viewChanged() ) );
    connect( m_view->checkBox_outputDevice, SIGNAL( toggled( bool ) ), SIGNAL( viewChanged() ) );
    connect( m_view->kLineEdit_outputDevice, SIGNAL( textChanged( const QString& ) ), SIGNAL( viewChanged() ) );
    connect( m_view->checkBox_outputParams, SIGNAL( toggled( bool ) ), SIGNAL( viewChanged() ) );
    connect( m_view->kLineEdit_outputParams, SIGNAL( textChanged( const QString& ) ), SIGNAL( viewChanged() ) );
}


GstConfigDialog::~GstConfigDialog()
{
    kDebug() ;

    delete m_view;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
/////////////////////////////////////////////////////////////////////////////////////

bool
GstConfigDialog::hasChanged() const
{
    return GstConfig::soundOutput()           != m_view->kComboBox_output->currentText() ||
           GstConfig::useCustomSoundDevice()  != m_view->checkBox_outputDevice->isChecked() ||
           GstConfig::soundDevice()           != m_view->kLineEdit_outputDevice->text() ||
           GstConfig::useCustomOutputParams() != m_view->checkBox_outputParams->isChecked() ||
           GstConfig::outputParams()          != m_view->kLineEdit_outputParams->text();
}


bool
GstConfigDialog::isDefault() const
{
    return false;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
/////////////////////////////////////////////////////////////////////////////////////

void
GstConfigDialog::save() //SLOT
{
    kDebug() ;

    bool changed =  hasChanged();

    GstConfig::setSoundOutput( m_view->kComboBox_output->currentText() );
    GstConfig::setUseCustomSoundDevice( m_view->checkBox_outputDevice->isChecked() );
    GstConfig::setSoundDevice( m_view->kLineEdit_outputDevice->text() );
    GstConfig::setUseCustomOutputParams( m_view->checkBox_outputParams->isChecked() );
    GstConfig::setOutputParams( m_view->kLineEdit_outputParams->text() );

    if ( changed )
        emit settingsSaved();
}


#include "gstconfigdialog.moc"

