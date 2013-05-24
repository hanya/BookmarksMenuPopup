
#ifndef _BKPMC_HPP_
#define _BKPMC_HPP_

#include "basepopup.hpp"

#include <cppuhelper/implbase2.hxx>

#include <com/sun/star/lang/XServiceInfo.hpp>

namespace mytools
{

typedef ::cppu::ImplInheritanceHelper2
                < 
                    BasePopup, 
                    ::com::sun::star::util::XModifyListener, 
                    ::com::sun::star::lang::XServiceInfo
                 > BookmarksPopup_Base;

class BookmarksPopup : public BookmarksPopup_Base
{
public :
    explicit BookmarksPopup( ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext > const &xComponentContext );
    ~BookmarksPopup();
    
    static ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > create( const ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext > &xComponentContext);
    
    // XEventListener
    virtual void SAL_CALL disposing( const ::com::sun::star::lang::EventObject& Source ) throw (::com::sun::star::uno::RuntimeException);
    
    // XModifyListener
    virtual void SAL_CALL modified( const ::com::sun::star::lang::EventObject& aEvent ) throw (::com::sun::star::uno::RuntimeException);
    
    // XServiceInfo
    virtual ::rtl::OUString SAL_CALL getImplementationName(  ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::sal_Bool SAL_CALL supportsService( const ::rtl::OUString& ServiceName ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames(  ) throw (::com::sun::star::uno::RuntimeException);
    
    static ::rtl::OUString SAL_CALL getImplementationName_Static();
    static ::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames_Static();
    
    virtual Container * getContainer() throw ( ::com::sun::star::uno::RuntimeException );
protected:
    virtual void init();
};


} // namespace

#endif
