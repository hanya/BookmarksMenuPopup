
#ifndef _TAG_PMC_HPP_
#define _TAG_PMC_HPP_

#include "basepopup.hpp"

#include <cppuhelper/implbase1.hxx>

namespace mytools
{
/**
 * This controller is bound to the main popup. If the popup have to be 
 * updated, this tag popup is destructed and new instance is created. 
 * So, this controller does not need update function.
 */

class TagPopup : BasePopup
{
public :
    
    explicit TagPopup( ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext > const &xComponentContext, 
                        Manager * pManager, 
                        const ::rtl::OUString & sTagName );
    virtual ~TagPopup();
    
    static ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > create(
                ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext > const &xComponentContext, 
                Manager * pManager, 
                const ::rtl::OUString & sTagName );
protected:
    
    const OUString m_sTagName;
    
    virtual void init();
    virtual Container * getContainer() throw ( ::com::sun::star::uno::RuntimeException );
};


} // namespace

#endif
