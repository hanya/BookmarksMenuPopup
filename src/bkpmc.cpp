
#include "bkpmc.hpp"

using namespace ::rtl;
using namespace ::com::sun::star;

using ::com::sun::star::uno::UNO_QUERY;

#define IMPLE_NAME      "bookmarks.AnotherBookmarksPopup"
#define SERVICE_NAME    IMPLE_NAME

#define OUSTR( str ) ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( str ) )

namespace mytools
{

BookmarksPopup::BookmarksPopup( 
        uno::Reference< uno::XComponentContext > const & xComponentContext )
 : BookmarksPopup_Base(  )
{
    m_xContext = xComponentContext;
}

BookmarksPopup::~BookmarksPopup()
{
}

uno::Reference< uno::XInterface > BookmarksPopup::create( 
            const uno::Reference< uno::XComponentContext > &xComponentContext )
{
    return static_cast< ::cppu::OWeakObject* >( new BookmarksPopup( xComponentContext ) );
}


Container * BookmarksPopup::getContainer() throw ( ::com::sun::star::uno::RuntimeException )
{
    if ( !m_pManager )
        throw uno::RuntimeException();
    return static_cast< Container * >( m_pManager->getRoot() );
}


// ToDo move addModifyListener to bkpmc, so make init as protected and call it from inherited classes
void BookmarksPopup::init()
{
    ::osl::MutexGuard const g( m_aMutex );
    
    m_pPopupManager = new BookmarksPopupManager( m_xContext, m_sCommandURL );
    m_pManager = m_pPopupManager->getManager();
    m_pPopupManager->addModifyListener(
        uno::Reference< util::XModifyListener >( (OWeakObject*)this, UNO_QUERY ) );
}


// XEventListener
void SAL_CALL BookmarksPopup::disposing( const lang::EventObject& Source ) 
            throw (uno::RuntimeException)
{
    ::osl::MutexGuard const g( m_aMutex );
    
    if ( m_xPopupMenu.is() )
        m_xPopupMenu.clear();
    m_pPopupManager->removeModifyListener( 
        uno::Reference< util::XModifyListener >( (OWeakObject*)this, UNO_QUERY ) );
}


// XModifyListener
void SAL_CALL BookmarksPopup::modified( const lang::EventObject& aEvent ) throw (uno::RuntimeException)
{
    m_bUpdated = true;
}


// XServiceInfo
::rtl::OUString SAL_CALL BookmarksPopup::getImplementationName(  ) throw (uno::RuntimeException)
{
    return BookmarksPopup::getImplementationName_Static();
}

::sal_Bool SAL_CALL BookmarksPopup::supportsService( const ::rtl::OUString& ServiceName ) 
        throw (uno::RuntimeException)
{
    return ( ServiceName.equals( OUSTR( "com.sun.star.frame.PopupMenuController" ) ) );
}

uno::Sequence< ::rtl::OUString > SAL_CALL BookmarksPopup::getSupportedServiceNames(  ) 
        throw (uno::RuntimeException)
{
    return BookmarksPopup::getSupportedServiceNames_Static();
}


OUString BookmarksPopup::getImplementationName_Static() 
{
    return OUString::createFromAscii( IMPLE_NAME );
}

uno::Sequence< OUString > BookmarksPopup::getSupportedServiceNames_Static() 
{
    uno::Sequence< OUString > aRet( 2 );
    OUString * pArray = aRet.getArray();
    pArray[0] = OUString::createFromAscii( "com.sun.star.frame.PopupMenuController" );
    pArray[1] = OUString::createFromAscii( SERVICE_NAME );
    return aRet;
}

} // namespace
