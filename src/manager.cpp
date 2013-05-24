
#include "manager.hpp"

#include <map>

#include <osl/mutex.h>
#include <cppuhelper/interfacecontainer.h>
#include <rtl/ustrbuf.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/ucb/XSimpleFileAccess.hpp>
#include <com/sun/star/resource/XStringResourceWithLocation.hpp>

#include "bookmarks.hpp"

#include <stdio.h>


using namespace ::rtl;
using namespace ::com::sun::star;

using ::com::sun::star::uno::UNO_QUERY;

#define OUSTR( str ) ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( str ) )

#define ID_BOOKMRK_THIS     "id.menu.add.this"
#define ID_EDIT             "id.menu.edit"
#define ID_UNSORTED         "id.item.unsorted"
#define LABEL_BOOKMRK_THIS  "Bookmark ~This Document..."
#define LABEL_EDIT          "~Edit Bookmarks..."
#define LABEL_UNSORTED      "Unsorted Bookmarks"

#define CONFIG_NODE_CONTROLLERS "/mytools.Bookmarks/Controllers"

#define NAME_DATA_URL    "DataURL"
#define FILE_NAME_PREFIX "bookmarks_"

#define JSON_EXT    ".json"

#define EXT_ID      "mytools.bookmarks.AnotherBookmarksPopup"
#define EXT_DIR     "vnd.sun.star.extension://" EXT_ID "/"
#define BASE_FILE   EXT_DIR "base" JSON_EXT

namespace mytools
{

// tools.cpp
uno::Reference< uno::XInterface > getConfig( 
        const uno::Reference< uno::XComponentContext > & xContext, 
        const OUString & sNodeName ) throw (uno::RuntimeException);

uno::Reference< resource::XStringResourceWithLocation > getResourceLoader(
        const uno::Reference< uno::XComponentContext > & xContext );


struct Managed
{
    Manager * pManager;
    sal_Int32 nRef;
    
    ::osl::Mutex m_aListenerMutex;
    ::cppu::OInterfaceContainerHelper * m_pModifyListeners;
    
    Managed()
    {
        m_pModifyListeners = new ::cppu::OInterfaceContainerHelper( m_aListenerMutex );
    }
    
    ~Managed()
    {
        lang::EventObject ev;
        m_pModifyListeners->disposeAndClear( ev );
        
        delete m_pModifyListeners;
        m_pModifyListeners = NULL;
        
        delete pManager;
        pManager = NULL;
        
    }
};


typedef ::std::map< ::rtl::OUString, Managed * > ManagerMap;


class BookmarksManagerLoader
{
protected:
    static ManagerMap m_aManagers;
    
    static Manager * readBookmarks( uno::Reference< uno::XComponentContext > const & xContext, 
                const ::rtl::OUString & sPath, const bool bFallback );
    
    // get bookmarks path from configuration or path settings
    static OUString getBookmarksPath( 
                uno::Reference< uno::XComponentContext > const & xContext, 
                const OUString & sCommandURL );
    
    // read bytes from file
    static bool readData( 
                uno::Reference< uno::XComponentContext > const & xContext, 
                const OUString & sPath, 
                uno::Sequence< sal_Int8 > & bytes );
    
public:
    
    static Managed * find( const OUString & sCommand );
    
    // request to reload the manager
    static void reload( 
                uno::Reference< uno::XComponentContext > const & xContext, 
                const OUString & sCommand ) throw ( uno::RuntimeException );
    
    // get manager for the command
    static Manager * get( 
                uno::Reference< uno::XComponentContext > const & xContext, 
                const OUString & sCommand ) throw ( uno::RuntimeException );
    
    // decrement the reference count for the manager
    static void remove( const OUString & sCommand );
};


ManagerMap BookmarksManagerLoader::m_aManagers;


bool BookmarksManagerLoader::readData( 
                uno::Reference< uno::XComponentContext > const & xContext, 
                const OUString & sPath, 
                uno::Sequence< sal_Int8 > & bytes )
{
    uno::Reference< ucb::XSimpleFileAccess > xSimpleFileAcc(
            xContext->getServiceManager()->createInstanceWithContext( 
                OUSTR( "com.sun.star.ucb.SimpleFileAccess" ), xContext ), UNO_QUERY);
    if ( xSimpleFileAcc.is() )
    {
        if ( xSimpleFileAcc->exists( sPath ) && 
            !xSimpleFileAcc->isFolder( sPath ) )
        {
            uno::Reference< io::XInputStream > xStream;
            try
            {
                xStream.set( xSimpleFileAcc->openFileRead( sPath ) );
                sal_Int32 nRead = 0;
                for (;;)
                {
                    uno::Sequence< sal_Int8 > bs;
                    nRead = xStream->readBytes( bs, 1024 );
                    if ( !nRead )
                        break;
                    const sal_Int32 nPos = bytes.getLength();
                    bytes.realloc( nPos + nRead );
                    ::rtl_copyMemory( bytes.getArray() + nPos, 
                                bs.getConstArray(), (sal_uInt32)nRead );
                }
                xStream->closeInput();
                return true;
            }
            catch ( uno::Exception & )
            {
                if ( xStream.is() )
                    xStream->closeInput();
            }
        }
    }
    return false;
}


Manager * BookmarksManagerLoader::readBookmarks( 
                uno::Reference< uno::XComponentContext > const & xContext, 
                const ::rtl::OUString & sPath, const bool bFallback )
{
    uno::Sequence< sal_Int8 > bytes;
    if ( readData( xContext, sPath, bytes ) && !bFallback )
    {
        return Manager::load( (const char*)bytes.getConstArray() );
    }
    else
    {
        uno::Reference< resource::XStringResourceWithLocation > xRes( 
                                        getResourceLoader( xContext ) );
        if ( xRes.is() )
        {
            uno::Sequence< ::rtl::OUString > aNames( 6 );
            OUString * pNames = aNames.getArray();
            pNames[0] = OUSTR( ID_BOOKMRK_THIS );
            pNames[1] = OUSTR( ID_EDIT );
            pNames[2] = OUSTR( ID_UNSORTED );
            pNames[3] = OUSTR( LABEL_BOOKMRK_THIS );
            pNames[4] = OUSTR( LABEL_EDIT );
            pNames[5] = OUSTR( LABEL_UNSORTED );
            
            OString s( (const sal_Char*)bytes.getConstArray(), bytes.getLength() );
            sal_Int32 nStart = 0;
            for ( int i = 0; i < 3; ++i )
            {
                const OUString sName = pNames[i];
                const OString sName_ = ::rtl::OUStringToOString( 
                                            pNames[i+3], RTL_TEXTENCODING_UTF8 );
                sal_Int32 nPos = s.indexOf( sName_, 0 );
                if ( nPos > 0 )
                {
                    s = s.replaceAt( nPos, sName_.getLength(), 
                            ::rtl::OUStringToOString( xRes->resolveString( sName ), 
                                            RTL_TEXTENCODING_UTF8 ) );
                }
                nStart = nPos + sName_.getLength();
            }
            return Manager::load( (const char*)s.getStr() );
        }
    }
    return NULL;
}


OUString BookmarksManagerLoader::getBookmarksPath( 
            uno::Reference< uno::XComponentContext > const & xContext, 
            const OUString & sCommandURL )
{
    OUString sPath;
    const sal_Int32 nPos = sCommandURL.indexOfAsciiL( ":", 1 );
    if ( nPos > 0 )
    {
        try
        {
            const uno::Reference< uno::XInterface > xConfig = getConfig( 
                                    xContext, OUSTR( CONFIG_NODE_CONTROLLERS ) );
            uno::Reference< container::XNameAccess > xNameAcc( xConfig, UNO_QUERY );
            if ( xNameAcc.is() && xNameAcc->hasByName( sCommandURL ) )
            {
                uno::Reference< beans::XPropertySet > xPropSet( 
                            xNameAcc->getByName( sCommandURL ), UNO_QUERY );
                xPropSet->getPropertyValue( OUSTR( NAME_DATA_URL ) ) >>= sPath;
            }
            if ( sPath.getLength() == 0 )
            {
                uno::Reference< beans::XPropertySet > xPropSet(
                    xContext->getServiceManager()->createInstanceWithContext( 
                        OUSTR( "com.sun.star.util.PathSettings" ), xContext ), UNO_QUERY);
                
                if ( xPropSet.is() )
                {
                    OUString aConfigPath;
                    xPropSet->getPropertyValue( OUSTR( "UserConfig_writable" ) ) >>= aConfigPath;
                    
                    ::rtl::OUStringBuffer buff(60);
                    buff.append( aConfigPath )
                        .appendAscii( "/" )
                        .appendAscii( FILE_NAME_PREFIX )
                        .append( sCommandURL.copy( nPos + 1 ) )
                        .appendAscii( JSON_EXT );
                    sPath = buff.makeStringAndClear();
                }
            }
        }
        catch (uno::Exception &)
        {
        }
    }
    return sPath;
}


void BookmarksManagerLoader::reload( 
                uno::Reference< uno::XComponentContext > const & xContext, 
                const OUString & sCommand ) throw ( uno::RuntimeException )
{
    // ToDo check file date?
    Managed * managed = find( sCommand );
    if ( managed && managed->pManager )
    {
        const OUString sPath = getBookmarksPath( xContext, sCommand );
        uno::Sequence< sal_Int8 > bytes;
        if ( readData( xContext, sPath, bytes ) )
        {
            managed->pManager->update( (const char*)bytes.getConstArray() );
        }
    }
}


Managed * BookmarksManagerLoader::find( const OUString & sCommand )
{
    ManagerMap::iterator it = m_aManagers.find( sCommand );
    if ( it != m_aManagers.end() )
        return it->second;
    return NULL;
}


Manager * BookmarksManagerLoader::get( 
                uno::Reference< uno::XComponentContext > const & xContext, 
                const OUString & sCommand ) throw ( uno::RuntimeException )
{
    Managed * managed = find( sCommand );
    if ( managed )
    {
        managed->nRef++;
        return managed->pManager;
    }
    const OUString sPath = getBookmarksPath( xContext, sCommand );
    Manager * pManager = readBookmarks( xContext, sPath, false );
    if ( !pManager )
    {
        pManager = readBookmarks( xContext, OUSTR( BASE_FILE ), true );
    }
    if ( pManager )
    {
        Managed * managed = new Managed();
        managed->nRef = 1;
        managed->pManager = pManager;
        m_aManagers.insert( ManagerMap::value_type( sCommand, managed ) );
    }
    else
         throw uno::RuntimeException( OUSTR( "Failed to load bookmarks." ), 
                    uno::Reference< uno::XInterface >() );
    return pManager;
}


void BookmarksManagerLoader::remove( const OUString & sCommand )
{
    ManagerMap::iterator it = m_aManagers.find( sCommand );
    if ( it != m_aManagers.end() )
    {
        it->second->nRef--;
        if ( it->second->nRef == 0 )
        {
            delete it->second;
            m_aManagers.erase( it );
        }
    }
}


// --

BookmarksPopupManager::BookmarksPopupManager(
        uno::Reference< uno::XComponentContext > const & xComponentContext, 
        const OUString & sCommandURL )
 : BookmarksPopupManager_Base( m_aMutex )
 , m_xContext( xComponentContext )
 , m_sCommandURL( sCommandURL )
{
    // ToDo check the URL is in the list of controllers
}


BookmarksPopupManager::~BookmarksPopupManager()
{
}


void SAL_CALL BookmarksPopupManager::addModifyListener( 
                const uno::Reference< util::XModifyListener >& aListener ) 
        throw (::com::sun::star::uno::RuntimeException)
{
    ::osl::MutexGuard const g( m_aMutex );
    Managed * managed = BookmarksManagerLoader::find( m_sCommandURL );
    if ( managed )
        managed->m_pModifyListeners->addInterface( aListener );
}


void SAL_CALL BookmarksPopupManager::removeModifyListener( 
                const uno::Reference< util::XModifyListener >& aListener ) 
        throw (::com::sun::star::uno::RuntimeException)
{
    ::osl::MutexGuard const g( m_aMutex );
    Managed * managed = BookmarksManagerLoader::find( m_sCommandURL );
    if ( managed )
        managed->m_pModifyListeners->removeInterface( aListener );
}


sal_Bool SAL_CALL BookmarksPopupManager::isModified()
        throw (::com::sun::star::uno::RuntimeException)
{
    return sal_False;
}

void SAL_CALL BookmarksPopupManager::setModified( sal_Bool bModified ) 
        throw (uno::RuntimeException, beans::PropertyVetoException)
{
    ::osl::MutexGuard const g( m_aMutex );
    if ( m_sCommandURL.getLength() )
    {
        // broadcast to reload the data
        BookmarksManagerLoader::reload( m_xContext, m_sCommandURL );
        Managed * managed = BookmarksManagerLoader::find( m_sCommandURL );
        if ( managed )
        {
            lang::EventObject aEvent;
            managed->m_pModifyListeners->notifyEach( & util::XModifyListener::modified, aEvent );
        }
    }
}


Manager * BookmarksPopupManager::getManager()
{
    ::osl::MutexGuard const g( m_aMutex );
    return BookmarksManagerLoader::get( m_xContext, m_sCommandURL );
}

void BookmarksPopupManager::removeManager()
{
    ::osl::MutexGuard const g( m_aMutex );
    BookmarksManagerLoader::remove( m_sCommandURL );
}


::rtl::OUString SAL_CALL BookmarksPopupManager::getImplementationName(  ) 
    throw (::com::sun::star::uno::RuntimeException)
{
    return BookmarksPopupManager::getImplementationName_Static();
}

::sal_Bool SAL_CALL BookmarksPopupManager::supportsService( 
        const ::rtl::OUString& ServiceName ) throw (::com::sun::star::uno::RuntimeException)
{
    return ServiceName.equalsAscii( MANAGER_IMPLE_NAME );
}

uno::Sequence< ::rtl::OUString > SAL_CALL BookmarksPopupManager::getSupportedServiceNames(  ) 
    throw (::com::sun::star::uno::RuntimeException)
{
    return BookmarksPopupManager::getSupportedServiceNames_Static();
}

::rtl::OUString SAL_CALL BookmarksPopupManager::getImplementationName_Static()
{
    return OUSTR( MANAGER_IMPLE_NAME );
}

uno::Sequence< ::rtl::OUString > SAL_CALL BookmarksPopupManager::getSupportedServiceNames_Static()
{
    uno::Sequence< ::rtl::OUString > aNames( 1 );
    ::rtl::OUString * pNames = aNames.getArray();
    pNames[0] = OUSTR( MANAGER_IMPLE_NAME );
    return aNames;
}

// --

BookmarksPopupManagerFactory::BookmarksPopupManagerFactory( 
    uno::Reference< uno::XComponentContext > const & xComponentContext )
 : BookmarksPopupManagerFactory_Base( m_aMutex )
 , m_xContext( xComponentContext )
{
}

BookmarksPopupManagerFactory::~BookmarksPopupManagerFactory()
{
}

uno::Reference< uno::XInterface > BookmarksPopupManagerFactory::create( 
        const uno::Reference< uno::XComponentContext > &xComponentContext)
{
    return static_cast< ::cppu::OWeakObject* >( 
            new BookmarksPopupManagerFactory( xComponentContext ) );
}

uno::Any SAL_CALL BookmarksPopupManagerFactory::getByUniqueID( 
        const ::rtl::OUString& ID ) 
        throw (container::NoSuchElementException, uno::RuntimeException)
{
    const ::osl::MutexGuard g( m_aMutex );
    uno::Reference< uno::XInterface > xInterface = static_cast< ::cppu::OWeakObject * >( 
                        new BookmarksPopupManager( m_xContext, ID ) );
    uno::Any a;
    a <<= xInterface;
    return a;
}

void SAL_CALL BookmarksPopupManagerFactory::removeByUniqueID( 
        const ::rtl::OUString& ID ) 
        throw (container::NoSuchElementException, uno::RuntimeException)
{ // not supported
}

// XServiceInfo
OUString SAL_CALL BookmarksPopupManagerFactory::getImplementationName(  ) throw (uno::RuntimeException)
{
    return BookmarksPopupManagerFactory::getImplementationName_Static();
}

::sal_Bool SAL_CALL BookmarksPopupManagerFactory::supportsService( 
        const ::rtl::OUString& ServiceName ) throw (uno::RuntimeException)
{
    return ServiceName.equals( OUSTR( MANAGER_FACTORY_IMPLE_NAME ) );
}

uno::Sequence< ::rtl::OUString > SAL_CALL BookmarksPopupManagerFactory::getSupportedServiceNames(  ) 
        throw (uno::RuntimeException)
{
    return BookmarksPopupManagerFactory::getSupportedServiceNames_Static();
}

OUString SAL_CALL BookmarksPopupManagerFactory::getImplementationName_Static()
{
    return OUSTR( MANAGER_FACTORY_IMPLE_NAME );
}

uno::Sequence< ::rtl::OUString > SAL_CALL BookmarksPopupManagerFactory::getSupportedServiceNames_Static()
{
    uno::Sequence< ::rtl::OUString > aNames( 1 );
    ::rtl::OUString * pNames = aNames.getArray();
    pNames[0] = OUSTR( MANAGER_FACTORY_IMPLE_NAME );
    return aNames;
}


} // namespace