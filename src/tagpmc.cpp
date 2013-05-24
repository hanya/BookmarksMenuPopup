
#include "tagpmc.hpp"

using namespace ::rtl;
using namespace ::com::sun::star;

using ::com::sun::star::uno::UNO_QUERY;

namespace mytools
{


TagPopup::TagPopup( const uno::Reference< uno::XComponentContext > &rComponentContext, 
                    Manager * pManager, 
                    const OUString & sTagName )
 : BasePopup()
 , m_sTagName( sTagName )
{
    m_xContext = rComponentContext;
    m_pManager = pManager;
}


TagPopup::~TagPopup()
{
}


void TagPopup::init()
{
}


uno::Reference< uno::XInterface > TagPopup::create( 
        const uno::Reference< uno::XComponentContext > &rComponentContext, 
        Manager * pManager, 
        const OUString & sTagName )
{
    return static_cast< ::cppu::OWeakObject* >( 
            new TagPopup( rComponentContext, pManager, sTagName ) );
}


Container * TagPopup::getContainer() throw ( ::com::sun::star::uno::RuntimeException )
{
    if ( !m_pManager )
        throw uno::RuntimeException();
    return static_cast< Container * >( m_pManager->getTagContainer( 
            (const char *)::rtl::OUStringToOString( m_sTagName, RTL_TEXTENCODING_UTF8 ).getStr() ) );
}


} // namespace
