
#include "basepopup.hpp"

#include <osl/process.h>
#include <osl/mutex.h>

#include <rtl/ustrbuf.hxx>

#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/XModuleManager.hpp>
#include <com/sun/star/task/XJobExecutor.hpp>
#include <com/sun/star/resource/XStringResourceWithLocation.hpp>
#include <com/sun/star/awt/XMessageBoxFactory.hpp>
#include <com/sun/star/awt/XMessageBox.hpp>
#include <com/sun/star/frame/XDispatchHelper.hpp>
#include <com/sun/star/system/SystemShellExecuteFlags.hpp>
#ifdef AOO4
#include <com/sun/star/awt/MessageBoxType.hpp>
#include <com/sun/star/system/SystemShellExecute.hpp>
#else
#include <com/sun/star/awt/XMenuExtended2.hpp>
#include <com/sun/star/awt/XPopupMenuExtended.hpp>
#include <com/sun/star/system/XSystemShellExecute.hpp>
#endif

#include "manager.hpp"
#include "executor.hpp"
#include "tagpmc.hpp"

using namespace ::rtl;
using namespace ::com::sun::star;

using ::com::sun::star::uno::UNO_QUERY;
using ::com::sun::star::uno::UNO_QUERY_THROW;


#define IMPLE_NAME      "bookmarks.AnotherBookmarksPopup"
#define SERVICE_NAME    IMPLE_NAME

#define TAG_POPUP_IMPLE_NAME "bookmarks.AnotherTagPopup"
#define EXT_ID        "mytools.bookmarks.AnotherBookmarksPopup"
#define BKMRKS_EXT_ID "mytools.bookmarks.BookmarksMenu"
#define OPEN_ALL_ID   -0xfff
#define ID_OPENALL "id.menu.open.all"

#define PROTOCOL_BOOKMARKS  BKMRKS_EXT_ID ":"
#define PROTOCOL_LENGTH     32
#define PROTOCOL_CUSTOM     "mytools.frame"
#define TAG_POPUP_PREFIX    "mytools.frame:TagPopupMenu"

#define CONFIG_NODE_SETTINGS "/mytools.Bookmarks/Settings"
#define NAME_WEB_BROWSER  "WebBrowser"
#define NAME_FILE_MANAGER "FileManger"
#define NAME_OPEN_COMMAND "OpenCommand"
#define NAME_USE_CUSTOM_WEB_BROWSER  "UseCustomWebBrowser"
#define NAME_USE_CUSTOM_FILE_MANAGER "UseCustomFileManager"
#define NAME_USE_CUSTOM_OPEN_COMMAND "UseCustomOpenCommand"

#define ORGANIZER   "bookmarks.BookmarksMenuManager"

#define OUSTR( str ) ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( str ) )
#define CHARTOOUSTR( s ) OUString( (const sal_Char*)s, (sal_Int32)strlen( s ), RTL_TEXTENCODING_UTF8 )

#include <stdio.h>

#define PRINT( str ) printf("%s\n", ::rtl::OUStringToOString( str, RTL_TEXTENCODING_UTF8 ).getStr())

namespace mytools { 

// tools.cpp
uno::Reference< uno::XInterface > getConfig( 
        const uno::Reference< uno::XComponentContext > & xContext, 
        const OUString & sNodeName ) throw (uno::RuntimeException);
bool getConfigBoolean( 
                const uno::Reference< uno::XComponentContext > & xContext, 
                const OUString & sNodeName, 
                const OUString & sPropertyVane );
OUString getConfigString(
                const uno::Reference< uno::XComponentContext > & xContext, 
                const OUString & sNodeName, 
                const OUString & sPropertyVane );

uno::Reference< resource::XStringResourceWithLocation > getResourceLoader(
        const uno::Reference< uno::XComponentContext > & xContext );


static OUString lcl_unquote( const OUString & s )
{
    static const OUString eq = OUSTR( "=" );
    static const OUString am = OUSTR( "&" );
    
    OUString ret = OUString( s );
    
    sal_Int32 nPos = 0;
    while (1)
    {
        nPos = ret.indexOfAsciiL( "%3D", 3, nPos );
        if ( nPos < 0 ) break;
        ret = ret.replaceAt( nPos, 3, eq );
    }
    nPos = 0;
    while (1)
    {
        nPos = ret.indexOfAsciiL( "%3d", 3, nPos );
        if ( nPos < 0 ) break;
        ret = ret.replaceAt( nPos, 3, eq );
    }
    nPos = 0;
    while (1)
    {
        nPos = ret.indexOfAsciiL( "%26", 3, nPos );
        if ( nPos < 0 ) break;
        ret = ret.replaceAt( nPos, 3, am );
    }
    return ret;
}


static uno::Sequence< OUString > lcl_parseCommand( const OUString & sCommand )
{
    OUString sMain, sScheme, sPath, sQuery;
    
    sal_Int32 nPos = sCommand.indexOfAsciiL( "?", 1 );
    if ( nPos > 0 )
    {
        sMain = sCommand.copy( 0, nPos );
        sQuery = sCommand.copy( nPos + 1 );
    }
    else
        sMain = sCommand;
    nPos = sMain.indexOfAsciiL( ":", 1 );
    if ( nPos > 0 )
    {
        sScheme = sMain.copy( 0, nPos );
        sPath = sMain.copy( nPos + 1 );
    }
    uno::Sequence< OUString >aRet( 4 );
    OUString * pArray = aRet.getArray();
    pArray[0] = sMain;
    pArray[1] = sScheme;
    pArray[2] = sPath;
    pArray[3] = sQuery;
    return aRet;
}


static OUString lcl_decodeQsl( const OUString & sQs_ )
{
    const OUString sQs = sQs_.concat( OUSTR( "&" ) );
    OUStringBuffer buff;
    
    sal_Int32 nPos, nPos2, nStart = 0;
    nPos = sQs.indexOfAsciiL( "&", 1, 0 );
    while (1)
    {
        const OUString pair = sQs.copy( nStart, nPos - nStart );
        nStart = nPos + 1;
        nPos2 = pair.indexOfAsciiL( "=", 1 );
        if ( nPos2 >= 0 )
        {
            buff.append( lcl_unquote( pair.copy( 0, nPos2 ) ) )
                .appendAscii( "=" )
                .append( lcl_unquote( pair.copy( nPos2 + 1 ) ) );
        }
        nPos = sQs.indexOfAsciiL( "&", 1, nStart );
        if ( nPos < 0 ) break;
        buff.appendAscii( "&" );
    }
    if ( buff.getLength() == 0 )
        return sQs;
    return buff.makeStringAndClear();
}


static OUString lcl_getQueryValue( const OUString & sQs, const OUString & sName )
{
    const sal_Int32 nLength = sName.getLength();
    if ( nLength > 0 )
    {
        sal_Int32 nPos, nSepPos, nStart = 0;
        nPos = sQs.indexOfAsciiL( "&", 1, nStart );
        while (1)
        {
            nStart = nPos + 1;
            nSepPos = sQs.indexOfAsciiL( "=", 1, nPos + 1 );
            if ( nSepPos < 0 ) break; // no more pair?
            if ( ( nSepPos - nStart ) == nLength && 
                 sQs.match( sName, nPos + 1 ) )
            {
                if ( nPos > 0 )
                    return sQs.copy( nSepPos + 1, nSepPos - nPos );
                else
                    return sQs.copy( nSepPos + 1 );
            }
            nPos = sQs.indexOfAsciiL( "&", 1, nStart );
            if ( nPos < 0 ) break;
        }
    }
    return OUString();
}


static OUString lcl_decodeCommand( const OUString & sCommand )
{
    const uno::Sequence< OUString > ar = lcl_parseCommand( sCommand );
    if ( ar.getLength() == 4 )
    {
        const OUString sQuery = ar[3];
        if ( sQuery.getLength() )
        {
            OUStringBuffer buff;
            buff.append( ar[0] )
                .appendAscii( "?" )
                .append( lcl_decodeQsl( sQuery ) );
            return buff.makeStringAndClear();
        }
        else
            return ar[0];
    }
    return sCommand;
}


static CommandType lcl_extractValues( const OUString & sCommand, 
                        OUString & sValue1, OUString & sValue2 )
{
    CommandType t = UNKNOWN;
    uno::Sequence< OUString > ar = lcl_parseCommand( sCommand );
    if ( ar.getLength() == 4 )
    {
        const OUString sPath = ar[2];
        if ( sPath.equalsAsciiL( "Program", 7 ) )
        {
            t = PROGRAM;
            sValue1 = lcl_getQueryValue( ar[3], OUSTR( QUERY_NAME_PATH ) );
            sValue2 = lcl_getQueryValue( ar[3], OUSTR( QUERY_NAME_ARGUMENTS ) );
        }
        else if ( sPath.equalsAsciiL( "File", 4 ) )
        {
            t = FILE;
            sValue1 = lcl_getQueryValue( ar[3], OUSTR( QUERY_NAME_PATH ) );
        }
        else if ( sPath.equalsAsciiL( "Folder", 6 ) )
        {
            t = FOLDER;
            sValue1 = lcl_getQueryValue( ar[3], OUSTR( QUERY_NAME_PATH ) );
        }
        else if ( sPath.equalsAsciiL( "Web", 3 ) )
        {
            t = WEB;
            sValue1 = lcl_getQueryValue( ar[3], OUSTR( QUERY_NAME_PATH ) );
        }
        else if ( sPath.equalsAsciiL( "Edit", 4 ) )
            t = EDIT;
        else if ( sPath.equalsAsciiL( "AddThis", 7 ) )
            t = ADDTHIS;
        else
        {
            sValue1 = sCommand;
        }
    }
    return t;
}


void BasePopup::exec( const OUString & sCommand, const OUString & sArguments )
{
    uno::Reference< system::XSystemShellExecute > xSse;
    try
    {
#ifdef AOO4
            xSse.set( system::SystemShellExecute::create( m_xContext ) );
#else
            xSse.set( 
                m_xContext->getServiceManager()->createInstanceWithContext(
                OUSTR( "com.sun.star.system.SystemShellExecute" ), m_xContext ), UNO_QUERY );
#endif
    }
    catch ( uno::Exception & )
    {
    }
    if ( xSse.is() )
    {
        ExecutionThread * thread = new ExecutionThread( xSse, sCommand, sArguments );
        if ( thread->createSuspended() )
            thread->resume();
    }
}


void BasePopup::openFolder( const OUString & sPath ) const
{
    OUString sCommand;
    OUString sPath_ = sPath;
    if ( getConfigBoolean( m_xContext, 
                           OUSTR( CONFIG_NODE_SETTINGS ), 
                           OUSTR( NAME_USE_CUSTOM_OPEN_COMMAND ) ) )
    {
        sCommand = getConfigString( m_xContext, 
                OUSTR( CONFIG_NODE_SETTINGS ), OUSTR( NAME_OPEN_COMMAND ) );
    }
    else
    {
        sCommand = m_sExecCommand;
        if ( m_sExecOption.getLength() )
        {
            OUStringBuffer buff;
            buff.append( m_sExecOption )
                .appendAscii( " " )
                .append( sPath );
            sPath_ = buff.makeStringAndClear();
        }
    }
    exec( sCommand, sPath_ );
}


void BasePopup::openFile( const OUString & sPath ) const
{
    
    OUString sCommand;
    OUString sPath_ = sPath;
    if ( getConfigBoolean( m_xContext, 
                           OUSTR( CONFIG_NODE_SETTINGS ), 
                           OUSTR( NAME_USE_CUSTOM_FILE_MANAGER ) ) )
    {
        sCommand = getConfigString( m_xContext, 
                OUSTR( CONFIG_NODE_SETTINGS ), OUSTR( NAME_OPEN_COMMAND ) );
    }
    else
    {
        sCommand = m_sFileManager;
        if ( m_sFileManagerOption.getLength() )
        {
            OUStringBuffer buff;
            buff.append( m_sFileManagerOption )
                .appendAscii( " " )
                .append( sPath );
            sPath_ = buff.makeStringAndClear();
        }
    }
    exec( sCommand, sPath_ );
}


void BasePopup::openWeb( const OUString & sURL ) const
{
    OUString sURL_ = sURL;
    OUString sCommand;
    if ( getConfigBoolean( m_xContext, 
                           OUSTR( CONFIG_NODE_SETTINGS ), 
                           OUSTR( NAME_USE_CUSTOM_WEB_BROWSER ) ) )
    {
        sCommand = getConfigString( m_xContext, 
                OUSTR( CONFIG_NODE_SETTINGS ), OUSTR( NAME_OPEN_COMMAND ) );
    }
    else
    {
        sCommand = m_sExecCommand;
        if ( m_sExecOption.getLength() )
        {
            OUStringBuffer buff;
            buff.append( m_sExecOption )
                .appendAscii( " " )
                .append( sURL );
            sURL_ = buff.makeStringAndClear();
        }
    }
    exec( sCommand, sURL_ );
}


void BasePopup::executeCommand( const OUString &sCommand )
{
    if ( sCommand.indexOfAsciiL( PROTOCOL_BOOKMARKS, PROTOCOL_LENGTH ) == 0 )
    {
        OUString sValue1, sValue2;
        CommandType t = lcl_extractValues( sCommand, sValue1, sValue2 );
        switch ( t )
        {
            case PROGRAM:
                exec( sValue1, sValue2 );
                break;
            case FILE:
                openFile( sValue1 );
                break;
            case FOLDER:
                openFolder( sValue1 );
                break;
            case WEB:
                openWeb( sValue1 );
                break;
            case EDIT:
            case ADDTHIS:
            {
                uno::Any rExecutor( m_xContext->getServiceManager()->createInstanceWithContext(
                        OUSTR( ORGANIZER ), m_xContext ) );
                uno::Reference< task::XJobExecutor > xExecutor( rExecutor, UNO_QUERY );
                if ( xExecutor.is() )
                {
                    OUString sArg;
                    if ( t == EDIT )
                    {
                        OUStringBuffer buff(10);
                        buff.appendAscii( "Edit&" )
                            .append( m_sCommandURL.copy( PROTOCOL_LENGTH ) );
                        sArg = buff.makeStringAndClear();
                    }
                    else if ( t == ADDTHIS )
                    {
                        uno::Reference< lang::XInitialization > xInit(
                                                rExecutor, UNO_QUERY );
                        if ( xInit.is() )
                        {
                            uno::Sequence< uno::Any > aArgs( 1 );
                            uno::Any *pArgs = aArgs.getArray();
                            pArgs[0] <<= beans::PropertyValue( 
                                            OUSTR( "Frame" ), 0, m_aFrame, 
                                            beans::PropertyState_DIRECT_VALUE);
                            xInit->initialize( aArgs );
                            
                            OUStringBuffer buff(10);
                            buff.appendAscii( "AddThis&" )
                                .append( m_sCommandURL.copy( PROTOCOL_LENGTH ) );
                            sArg = buff.makeStringAndClear();
                        }
                    }
                    xExecutor->trigger( sArg );
                }
                break;
            }
            default:
                break;
        }
    }
    else
    {
        const OUString cmd = lcl_decodeCommand( sCommand );
        const uno::Reference< frame::XDispatchHelper > xDispHelper( 
                m_xContext->getServiceManager()->createInstanceWithContext(
                    OUSTR( "com.sun.star.frame.DispatchHelper" ), m_xContext ), UNO_QUERY );
        if ( xDispHelper.is() )
        {
            const uno::Reference< frame::XDispatchProvider > xDispProv( m_aFrame, UNO_QUERY );
            if ( xDispProv.is() )
            {
                uno::Sequence< beans::PropertyValue > aArgs;
                xDispHelper->executeDispatch( 
                    xDispProv, cmd, OUSTR( "_self" ), 0, aArgs );
            }
        }
    }
}
// create editor and pass XUpdate interface to tell

OUString BasePopup::m_sOpenAllLabel = OUString();
OUString BasePopup::m_sExecCommand = OUString();
OUString BasePopup::m_sExecOption = OUString();
OUString BasePopup::m_sFileManager = OUString();
OUString BasePopup::m_sFileManagerOption = OUString();
StringMap BasePopup::m_aPopupMenus;
bool BasePopup::m_bEnvDetected = false;


BasePopup::BasePopup( )
 : Popup_Base( m_aMutex )
 , m_bUpdated( false )
 , m_pManager( NULL )
 , m_pPopupManager( NULL )
{
}

void BasePopup::initVars()
{
    ::osl::MutexGuard const g( m_aMutex );
    
    // read popup controller names
    if ( BasePopup::m_aPopupMenus.empty() )
    {
        try
        {
            const uno::Reference< uno::XInterface > xConfig = getConfig( 
                    m_xContext, 
                    OUSTR( "/org.openoffice.Office.UI.Controller/Registered/PopupMenu" ) );
            const uno::Reference< container::XNameAccess > xNameAcc( xConfig, UNO_QUERY_THROW );
            static const OUString aNameCommand = OUSTR( "Command" );
            static const OUString aNameController = OUSTR( "Controller" );
            
            uno::Sequence< OUString > aNames = xNameAcc->getElementNames();
            const OUString *pNames = aNames.getConstArray();
            const sal_Int32 nCount = aNames.getLength();
            for ( int i = 0; i < nCount; ++i )
            {
                const uno::Reference< beans::XPropertySet > xProps( 
                            xNameAcc->getByName( pNames[i] ), UNO_QUERY );
                if ( xProps.is() )
                {
                    OUString aCommand, aController;
                    xProps->getPropertyValue( aNameCommand ) >>= aCommand;
                    xProps->getPropertyValue( aNameController ) >>= aController;
                    BasePopup::m_aPopupMenus.insert( 
                        StringMap::value_type( aCommand, aController ) );
                }
            }
        }
        catch ( uno::Exception & )
        {
        }
    }
    
    // load resource
    if ( !BasePopup::m_sOpenAllLabel.getLength() )
    {
        try
        {
            uno::Reference< resource::XStringResourceWithLocation > xRes( 
                                getResourceLoader( m_xContext ) );
            if ( xRes.is() )
                BasePopup::m_sOpenAllLabel = xRes->resolveString( OUSTR( ID_OPENALL ) );
        }
        catch ( uno::Exception & )
        {
            BasePopup::m_sOpenAllLabel = OUSTR( "Open ~All" );
        }
    }
}


void BasePopup::initCommandVars()
{
    ::osl::MutexGuard const g( m_aMutex );
    
    // detect current environment, ToDo store value in configuration?
    OUString sExecCommand, sFileManager;
#ifdef WNT
    sExecCommand = OUSTR( "rundll32.exe" );
    m_sExecOption = OUSTR( "url.dll,FileProtocolHandler" );
    sFileManager = OUSTR( "explorer.exe" );
#elif MACOSX
    sExecCommand = OUSTR( "open" );
    sFileManager = OUSTR( "open" );
#else
    bool bDetected = false;
    OUString sEnv;
    if ( osl_getEnvironment( OUSTR( "DESKTOP_SESSION" ).pData, &sEnv.pData ) == osl_Process_E_None )
    {
        if ( sEnv.equals( OUSTR( "xfce" ) ) || sEnv.equals( OUSTR( "xubuntu" ) ) )
        {
            sExecCommand = OUSTR( "exo-open" );
            sFileManager = OUSTR( "thunar" );
        }
        else if ( sEnv.equals( OUSTR( "ubuntu" ) ) || sEnv.equals( OUSTR( "ubuntu-2d" ) ) )
        {
            sExecCommand = OUSTR( "xdg-open" );
            sFileManager = OUSTR( "nautilus" );
        }
        else if ( sEnv.equals( OUSTR( "gnome" ) ) )
        {
            sExecCommand = OUSTR( "gnome-open" );
            sFileManager = OUSTR( "nautilus" );
        }
        else if ( sEnv.equals( OUSTR( "mate" ) ) )
        {
            sExecCommand = OUSTR( "mate-open" );
            sFileManager = OUSTR( "caja" );
        }
        bDetected = true;
    }
    else if ( osl_getEnvironment( 
            OUSTR( "GDMSESSION" ).pData, &sEnv.pData ) == osl_Process_E_None )
    {
        bDetected = true;
    }
    // check if not set KDE_SESSION_VERSION
    if ( !bDetected && 
            osl_getEnvironment( 
                OUSTR( "KDE_SESSION_VERSION" ).pData, &sEnv.pData ) == osl_Process_E_None )
    {
        if ( sEnv.equals( OUSTR( "4" ) ) )
        {
            // kde4
            sExecCommand = OUSTR( "kioclient" );
            m_sExecOption = OUSTR( "exec" );
            sFileManager = OUSTR( "kioclient" );
            m_sFileManagerOption = OUSTR( "exec" );
        }
        else
        {
            // kde3
            sExecCommand = OUSTR( "kfmclient" );
            m_sExecOption = OUSTR( "exec" );
            sFileManager = OUSTR( "kfmclient" );
            m_sFileManagerOption = OUSTR( "exec" );
        }
    }
#endif
    m_sExecCommand = sExecCommand;
    m_sFileManager = sFileManager;
    m_bEnvDetected = true;
}


BasePopup::~BasePopup()
{
    clearPopupItems();
    clearPopupControllers();
    
    if ( m_pManager )
    {
        m_pManager = NULL;
        if ( m_pPopupManager )
        {
            m_pPopupManager->removeManager();
            delete m_pPopupManager;
            m_pPopupManager = NULL;
        }
    }
    m_aPopupMenus.clear();
    
    if ( m_xPopupMenu.is() )
        m_xPopupMenu.clear();
}


void BasePopup::clearPopupItems()
{
    ::osl::MutexGuard const g( m_aMutex );
    
    if ( !m_aPopupItems.empty() )
    {
        ItemMap::reverse_iterator rit;
        for ( rit = m_aPopupItems.rbegin(); rit != m_aPopupItems.rend(); ++rit )
        {
            delete rit->second;
        }
        m_aPopupItems.clear();
    }
}


void BasePopup::clearPopupControllers()
{
    ::osl::MutexGuard const g( m_aMutex );
    
    if ( !m_aPopupControllers.empty() )
    {
        PopupControllerMap::iterator it;
        for ( it = m_aPopupControllers.begin(); it != m_aPopupControllers.end(); ++it )
        {
            uno::Reference< lang::XComponent > xComp( it->second->xController, UNO_QUERY );
            if ( xComp.is() )
                xComp->release();
            delete it->second;
        }
        m_aPopupControllers.clear();
    }
}


// XInitialization
void SAL_CALL BasePopup::initialize( const uno::Sequence< uno::Any >& aArguments ) 
        throw (uno::Exception, uno::RuntimeException)
{
    ::osl::MutexGuard const g( m_aMutex );
    
    static const OUString sFrameName = OUSTR( "Frame" );
    static const OUString sCommandURLName = OUSTR( "CommandURL" );
    
    const uno::Any * pArgs = aArguments.getConstArray();
    const uno::Any * pArgsEnd = aArguments.getConstArray() + aArguments.getLength();
    for ( ; pArgs < pArgsEnd; ++pArgs )
    {
        beans::PropertyValue aProp;
        if ( *pArgs >>= aProp )
        {
            if ( aProp.Name.equals( sFrameName ) )
                m_aFrame = aProp.Value;
            else if ( aProp.Name.equals( sCommandURLName ) )
                aProp.Value >>= m_sCommandURL;
        }
    }
    initVars();
    initCommandVars();
    init();
}


void BasePopup::init()
{
}


// XEventListener
void SAL_CALL BasePopup::disposing( const lang::EventObject& Source ) 
            throw (uno::RuntimeException)
{
    ::osl::MutexGuard const g( m_aMutex );
    
    if ( m_xPopupMenu.is() )
        m_xPopupMenu.clear();
}


// XPopupMenuController
void SAL_CALL BasePopup::setPopupMenu( 
    const uno::Reference< awt::XPopupMenu >& PopupMenu ) throw (uno::RuntimeException)
{
    ::osl::MutexGuard const g( m_aMutex );
    m_xPopupMenu = PopupMenu;
    prepareMenu( false, false );
}


void SAL_CALL BasePopup::updatePopupMenu(  ) throw (uno::RuntimeException)
{
    ::osl::MutexGuard const g( m_aMutex );
    if ( m_bUpdated )
    {
        prepareMenu( true, false );
        m_bUpdated = false;
    }
}


void BasePopup::fillPopup( const uno::Reference< awt::XPopupMenu > & rPopupMenu, 
                            Container * container, 
                            const bool bOpenAll ) throw (uno::RuntimeException)
{
    if ( !container )
        throw uno::RuntimeException();
    
    uno::Reference< awt::XMenu > xMenu( rPopupMenu, UNO_QUERY);
#ifndef AOO4
    uno::Reference< awt::XPopupMenuExtended > xPopupExt( rPopupMenu, UNO_QUERY );
    uno::Reference< awt::XMenuExtended > xMenuExt( rPopupMenu, UNO_QUERY );
#endif
    bool bHasItem = false;
    const int nCount = container->getChildCount();
    for ( int i = 0; i < nCount; ++i )
    {
        BaseItem *item = container->getChild( i );
        if ( !item )
            continue;
        
        const sal_Int16 id = item->getId();
        const ItemType t = item->getType();
        switch ( t )
        {
            case ITEM:
            case CONTAINER:
            {
                bool bNeedPopup = true;
                xMenu->insertItem( id, item->getName(), 0, i );
                const OUString aDesc = item->getDescription();
                if ( aDesc.getLength() )
                {
#ifdef AOO4
                    rPopupMenu->setTipHelpText( id, aDesc );
#else
                    xPopupExt->setTipHelpText( id, aDesc );
#endif
                }
                if ( t == ITEM )
                {
                    Item * ite = dynamic_cast< Item* >( item );
                    if ( ite )
                    {
#ifdef AOO4
                        rPopupMenu->setCommand( id, ite->getCommand() );
#else
                        xMenuExt->setCommand( id, ite->getCommand() );
#endif
                        bHasItem = true;
                        
                        const OUString aCommand = ite->getCommand( true );
                        bNeedPopup = ( m_aPopupMenus.count( aCommand ) && 
                                       ( aCommand.indexOfAsciiL( PROTOCOL_CUSTOM, 13 ) == 0 || 
                                         !ite->hasArguments() ) );
                    }
                }
                if ( bNeedPopup )
                {
                    uno::Reference< awt::XPopupMenu > xPopupMenu(
                        m_xContext->getServiceManager()->createInstanceWithContext( 
                            OUSTR( "com.sun.star.awt.PopupMenu" ), 
                            m_xContext ), UNO_QUERY);
                    xMenu->setPopupMenu( id, xPopupMenu );
                    if ( t == ITEM )
                        m_aPopupItems.insert( 
                            ItemMap::value_type( id, 
                                    new Item( dynamic_cast< Item * >( item ) ) ) );
                    else
                        m_aPopupItems.insert( 
                            ItemMap::value_type( id, 
                                    new ContainerItem( dynamic_cast< ContainerItem * >( item ) ) ) );
                }
                break;
            }
            case SEPARATOR:
            {
                rPopupMenu->insertSeparator( i );
                break;
            }
            default:
                break;
        }
    }
    if ( bOpenAll && bHasItem )
    {
        const sal_Int32 nCount = xMenu->getItemCount();
        rPopupMenu->insertSeparator( nCount );
        xMenu->insertItem( OPEN_ALL_ID, m_sOpenAllLabel, 0, nCount + 1);
    }
}


Container * BasePopup::getContainer() throw ( ::com::sun::star::uno::RuntimeException )
{
    throw uno::RuntimeException();
}

// ToDo move content in to setPopupMenu method
void BasePopup::prepareMenu( const bool bClear, const bool bOpenAll )
{
    if ( bClear )
    {
        // ToDo child popup menus should be cleard?
#ifdef AOO4
        if ( m_xPopupMenu.is() )
            m_xPopupMenu->clear();
#else
        uno::Reference< awt::XMenuExtended2 > xMenu( m_xPopupMenu, UNO_QUERY );
        if ( xMenu.is() )
            xMenu->clear();
#endif
        if ( !m_aPopupItems.empty() )
            clearPopupItems();
        if ( !m_aPopupControllers.empty() )
            clearPopupControllers();
    }
    Container *container = NULL;
    try
    {
        container = getContainer();
        if ( container )
            fillPopup( m_xPopupMenu, container, false );
    }
    catch (uno::Exception &)
    {
    }
    delete container;
    if ( !bClear )
    {
        m_xPopupMenu->addMenuListener( 
                uno::Reference< awt::XMenuListener >( (OWeakObject*)this, UNO_QUERY ) );
    }
}


// XDispatchProvider
uno::Reference< frame::XDispatch > SAL_CALL BasePopup::queryDispatch( 
        const util::URL& /* URL */, const ::rtl::OUString& /* TargetFrameName */, 
        ::sal_Int32 /* SearchFlags */ ) 
        throw (uno::RuntimeException)
{
}

uno::Sequence< uno::Reference< frame::XDispatch > > SAL_CALL BasePopup::queryDispatches( 
        const uno::Sequence< frame::DispatchDescriptor >& /* Requests */ ) 
        throw (uno::RuntimeException)
{
}

// XStatusListener
void SAL_CALL BasePopup::statusChanged( const frame::FeatureStateEvent & /* State */ ) 
        throw (uno::RuntimeException)
{
}


static OUString lcl_getModuleName( 
        const uno::Reference< uno::XComponentContext > & xContext, 
        const uno::Any & a )
{
    try
    {
        uno::Reference< frame::XModuleManager > xModManager( 
            xContext->getServiceManager()->createInstanceWithContext(
                OUSTR( "com.sun.star.frame.ModuleManager" ), xContext ), UNO_QUERY_THROW );
        uno::Reference< uno::XInterface > xInterface( a, UNO_QUERY );
        return xModManager->identify( xInterface );
    }
    catch ( uno::Exception & )
    { }
    return OUString();
}


// XMenuListener
#ifdef UNIFIED_MENU_API
void SAL_CALL BasePopup::itemHighlighted( const awt::MenuEvent& rEvent )
        throw (uno::RuntimeException)
#else
void SAL_CALL BasePopup::highlight( const awt::MenuEvent& rEvent ) 
        throw (uno::RuntimeException)
#endif
{
    const sal_Int16 nMenuId = rEvent.MenuId;
    uno::Reference< awt::XMenu > xMenu( rEvent.Source, uno::UNO_QUERY );
    uno::Reference< awt::XPopupMenu > xPopupMenu( xMenu->getPopupMenu( nMenuId ) );
    if ( !xPopupMenu.is() )
        return;
    
    ItemMap::iterator it = m_aPopupItems.find( nMenuId );
    if ( it == m_aPopupItems.end() )
        return;
    
    BaseItem * item = it->second;
    
    if ( item->getType() == CONTAINER )
    {
        if ( !xPopupMenu->getItemCount() )
        {
            Container *container = dynamic_cast< Container * >( item );
            if ( container && container->getChildCount() )
            {
                fillPopup( xPopupMenu, container, true );
                xPopupMenu->addMenuListener( 
                    uno::Reference< awt::XMenuListener >( (OWeakObject*)this, UNO_QUERY ) );
            }
        }
    }
    else
    {
        Item * ite = dynamic_cast< Item* >( item );
        if ( !ite )
            return; // json file broken?
        
        PopupControllerMap::iterator ipc = m_aPopupControllers.find( ite->getId() );
        if ( ipc != m_aPopupControllers.end() )
        {
            uno::Reference< frame::XPopupMenuController > xController;
            xController.set( ipc->second->xController );
            if ( xController.is() )
                xController->updatePopupMenu();
        }
        else
        {
            const OUString sCommand = ite->getCommand( true );
            
            if ( m_aPopupMenus.count( sCommand ) && 
                  ( sCommand.indexOfAsciiL( PROTOCOL_CUSTOM, 13 ) == 0 || 
                    !ite->hasArguments() ) )
            {
                StringMap::iterator iit = m_aPopupMenus.find( sCommand );
                if ( iit != m_aPopupMenus.end() )
                {
                    uno::Reference< frame::XPopupMenuController > xController;
                    const OUString sName = iit->second;
                    const OUString sFullCommand = ite->getCommand();
                    
                    uno::Sequence< uno::Any > aArgs( 3 );
                    uno::Any *pArgs = aArgs.getArray();
                    pArgs[0] = uno::makeAny( beans::PropertyValue( OUSTR( "ModuleName" ), 
                            0, uno::makeAny( lcl_getModuleName( m_xContext, m_aFrame ) ), 
                            beans::PropertyState_DIRECT_VALUE ) );
                    pArgs[1] = uno::makeAny( beans::PropertyValue( OUSTR( "Frame" ), 
                            0, m_aFrame, beans::PropertyState_DIRECT_VALUE ) );
                    pArgs[2] = uno::makeAny( beans::PropertyValue( OUSTR( "CommandURL" ), 
                            0, uno::makeAny( sFullCommand ), com::sun::star::beans::PropertyState_DIRECT_VALUE) );
                    
                    if ( sCommand.equalsAscii( TAG_POPUP_PREFIX ) )
                    {
                        const uno::Sequence< OUString > aParts = lcl_parseCommand( sFullCommand );
                        const OUString sTagName = lcl_getQueryValue( aParts[3], OUSTR( "Tag:string" ) );
                        xController.set(
                            TagPopup::create( m_xContext, m_pManager, sTagName ), UNO_QUERY );
                        
                        uno::Reference< lang::XInitialization > xInit( xController, UNO_QUERY );
                        if ( xInit.is() )
                            xInit->initialize( aArgs );
                    }
                    else
                    {
                        xController.set( 
                            m_xContext->getServiceManager()->createInstanceWithArgumentsAndContext(
                                    sName, aArgs, m_xContext ), UNO_QUERY );
                    }
                    if ( xController.is() )
                    {
                        xController->setPopupMenu( xPopupMenu );
                        m_aPopupControllers.insert( 
                            PopupControllerMap::value_type( ite->getId(), 
                                new ControllerWrapper( xController ) ) );
                    }
                }
            }
        }
    }
}

#ifdef UNIFIED_MENU_API
void SAL_CALL BasePopup::itemSelected( const awt::MenuEvent& rEvent ) 
        throw (uno::RuntimeException)
#else
void SAL_CALL BasePopup::select( const awt::MenuEvent& rEvent ) 
        throw (uno::RuntimeException)
#endif
{
    const sal_Int16 nMenuId = rEvent.MenuId;
#ifdef UNIFIED_MENU_API
    uno::Reference< awt::XPopupMenu > xPopupMenu( rEvent.Source, uno::UNO_QUERY );
#else
    uno::Reference< awt::XMenuExtended2 > xPopupMenu( rEvent.Source, uno::UNO_QUERY );
#endif
    OUString sCommand = xPopupMenu->getCommand( nMenuId );
    if ( sCommand.getLength() )
    {
        executeCommand( sCommand );
    }
    else if ( nMenuId == OPEN_ALL_ID )
    {
        const sal_Int32 nCount = xPopupMenu->getItemCount();
        for ( sal_Int32 i = 0; i < nCount; ++i )
        {
            OUString cmd = xPopupMenu->getCommand( xPopupMenu->getItemId( i ) );
            if ( cmd.getLength() && m_aPopupMenus.count( cmd ) )
            {
                try
                {
                    executeCommand( cmd );
                }
                catch ( uno::Exception & )
                {
                }
            }
        }
    }
}

#ifdef UNIFIED_MENU_API
void SAL_CALL BasePopup::itemActivated( const awt::MenuEvent & /* rEvent */ ) 
        throw (uno::RuntimeException)
#else
void SAL_CALL BasePopup::activate( const awt::MenuEvent& /* rEvent */ ) 
        throw (uno::RuntimeException)
#endif
{
}

#ifdef UNIFIED_MENU_API
void SAL_CALL BasePopup::itemDeactivated( const awt::MenuEvent & /* rEvent */ ) 
        throw (uno::RuntimeException)
#else
void SAL_CALL BasePopup::deactivate( const awt::MenuEvent& /* rEvent */ ) 
        throw (uno::RuntimeException)
#endif
{
}


sal_Int16 BasePopup::message( const ::rtl::OUString& aTitle, 
                const ::rtl::OUString& aMessage, const bool bError )
{
    sal_Int16 nRet = -1;
     
    uno::Reference< frame::XFrame > xFrame( m_aFrame, UNO_QUERY );
    if ( !xFrame.is() )
        throw uno::RuntimeException();
    uno::Reference< awt::XWindowPeer > xWindowPeer( xFrame->getContainerWindow(), UNO_QUERY );
    uno::Reference< awt::XToolkit > xToolkit( xWindowPeer->getToolkit() );
    uno::Reference< awt::XMessageBoxFactory > xMessBoxFac( xToolkit, UNO_QUERY );
    uno::Reference< awt::XMessageBox > xMessageBox( xMessBoxFac->createMessageBox(
        xWindowPeer, 
#ifdef AOO4
        ( bError ? awt::MessageBoxType_ERRORBOX : awt::MessageBoxType_MESSAGEBOX ), 
#else
        awt::Rectangle(), 
        ( bError ? OUSTR( "errorbox" ) : OUSTR( "messbox" ) ), 
#endif
        1, aTitle, aMessage ) );
    if ( xMessageBox.is() )
    {
        nRet = xMessageBox->execute();
    }
    uno::Reference< lang::XComponent > xComponent( xMessageBox, UNO_QUERY );
    xComponent->dispose();
    return nRet;
}

} // namespace
