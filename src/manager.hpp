
#ifndef _MANAGER_HPP_
#define _MANAGER_HPP_

#include <osl/mutex.h>
#include <cppuhelper/basemutex.hxx>
#include <cppuhelper/compbase2.hxx>

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/util/XModifiable.hpp>
#include <com/sun/star/container/XUniqueIDAccess.hpp>

#define MANAGER_IMPLE_NAME   "bookmarks.BookmarksPopupManager"
#define MANAGER_FACTORY_IMPLE_NAME   "bookmarks.BookmarksPopupManagerFactory"

namespace mytools
{

class Manager;
class BookmarksPopup;


typedef ::cppu::WeakComponentImplHelper2< 
                    ::com::sun::star::util::XModifiable, 
                    ::com::sun::star::lang::XServiceInfo
                    > BookmarksPopupManager_Base;


class BookmarksPopupManager : protected ::cppu::BaseMutex, 
                              public BookmarksPopupManager_Base
{
    ::osl::Mutex m_aMutex;
    
    ::rtl::OUString m_sCommandURL;
    ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext > m_xContext;
    // path to data file
    ::rtl::OUString m_sPath;
    
public:
    
    BookmarksPopupManager( 
            ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext > const & xComponentContext, 
            const ::rtl::OUString & sCommandURL );
    virtual ~BookmarksPopupManager();
    
    Manager * getManager();
    void removeManager();
    
    // XModifyBroadcaster
    virtual void SAL_CALL addModifyListener( 
            const ::com::sun::star::uno::Reference< ::com::sun::star::util::XModifyListener >& aListener ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL removeModifyListener( 
            const ::com::sun::star::uno::Reference< ::com::sun::star::util::XModifyListener >& aListener ) throw (::com::sun::star::uno::RuntimeException);
    
    // XModifiable
    virtual sal_Bool SAL_CALL isModified(  ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setModified( sal_Bool bModified ) throw (::com::sun::star::uno::RuntimeException, ::com::sun::star::beans::PropertyVetoException);
    
    // XServiceInfo
    virtual ::rtl::OUString SAL_CALL getImplementationName(  ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::sal_Bool SAL_CALL supportsService( const ::rtl::OUString& ServiceName ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames(  ) throw (::com::sun::star::uno::RuntimeException);
    
    static ::rtl::OUString SAL_CALL getImplementationName_Static();
    static ::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames_Static();
};


typedef ::cppu::WeakComponentImplHelper2< 
                    ::com::sun::star::container::XUniqueIDAccess, 
                    ::com::sun::star::lang::XServiceInfo
                    > BookmarksPopupManagerFactory_Base;


class BookmarksPopupManagerFactory : 
                protected ::cppu::BaseMutex, 
                public BookmarksPopupManagerFactory_Base
{
protected:
    ::osl::Mutex m_aMutex;
    
    ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext > m_xContext;
    
public:
    
    BookmarksPopupManagerFactory( ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext > const & xComponentContext );
    virtual ~BookmarksPopupManagerFactory();
    
    static ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > create( const ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext > &xComponentContext);
    
    virtual ::com::sun::star::uno::Any SAL_CALL getByUniqueID( const ::rtl::OUString& ID ) throw (::com::sun::star::container::NoSuchElementException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL removeByUniqueID( const ::rtl::OUString& ID ) throw (::com::sun::star::container::NoSuchElementException, ::com::sun::star::uno::RuntimeException);
    
    // XServiceInfo
    virtual ::rtl::OUString SAL_CALL getImplementationName(  ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::sal_Bool SAL_CALL supportsService( const ::rtl::OUString& ServiceName ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames(  ) throw (::com::sun::star::uno::RuntimeException);
    
    static ::rtl::OUString SAL_CALL getImplementationName_Static();
    static ::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames_Static();
};

}

#endif
