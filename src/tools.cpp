
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/resource/StringResourceWithLocation.hpp>

using namespace ::rtl;
using namespace ::com::sun::star;

using ::com::sun::star::uno::UNO_QUERY;

#define OUSTR( str ) ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( str ) )

#define BKMRKS_EXT_ID       "mytools.bookmarks.BookmarksMenu"

#define BKMRKS_EXT_DIR      "vnd.sun.star.extension://" BKMRKS_EXT_ID "/"
#define BKMRKS_RES_DIR      BKMRKS_EXT_DIR "resources"
#define BKMRKS_RES_FILE     "strings"

namespace mytools
{

uno::Reference< uno::XInterface > getConfig( 
        const uno::Reference< uno::XComponentContext > & xContext, 
        const OUString & sNodeName ) throw (uno::RuntimeException)
{
    uno::Reference< uno::XInterface > xAccess;
    try
    {
        uno::Reference< lang::XMultiServiceFactory > xMsf(
            xContext->getServiceManager()->createInstanceWithContext( 
                OUSTR( "com.sun.star.configuration.ConfigurationProvider" ), xContext ), UNO_QUERY);
        
        uno::Sequence< uno::Any > arguments( 1 );
        arguments[0] = ::com::sun::star::uno::makeAny(
            ::com::sun::star::beans::PropertyValue( OUSTR( "nodepath" ), 0, 
                ::com::sun::star::uno::makeAny( sNodeName ), 
                ::com::sun::star::beans::PropertyState_DIRECT_VALUE));
        
        xAccess = xMsf->createInstanceWithArguments(
            OUSTR( "com.sun.star.configuration.ConfigurationAccess" ), arguments);
    }
    catch ( uno::Exception & )
    {
    }
    return xAccess;
}


bool getConfigBoolean( 
                const uno::Reference< uno::XComponentContext > & xContext, 
                const OUString & sNodeName, 
                const OUString & sPropertyVane )
{
    bool ret = false;
    uno::Reference< beans::XPropertySet > xPropSet;
    try
    {
        xPropSet.set( getConfig( xContext, sNodeName ), UNO_QUERY );
    }
    catch (uno::Exception &)
    {
    }
    if ( xPropSet.is() )
    {
        try
        {
            sal_Bool b;
            if ( xPropSet->getPropertyValue( sPropertyVane ) >>= b )
                ret = b ? true : false;
        }
        catch (uno::Exception &)
        {
        }
    }
    return ret;
}


OUString getConfigString(
                const uno::Reference< uno::XComponentContext > & xContext, 
                const OUString & sNodeName, 
                const OUString & sPropertyVane )
{
    OUString ret;
    uno::Reference< beans::XPropertySet > xPropSet;
    try
    {
        xPropSet.set( getConfig( xContext, sNodeName ), UNO_QUERY );
    }
    catch (uno::Exception &)
    {
    }
    if ( xPropSet.is() )
    {
        try
        {
            xPropSet->getPropertyValue( sPropertyVane ) >>= ret;
        }
        catch (uno::Exception &)
        {
        }
    }
    return ret;
}


uno::Reference< resource::XStringResourceWithLocation > getResourceLoader(
        const uno::Reference< uno::XComponentContext > & xContext )
{
    lang::Locale aLocale;
    try
    {
        uno::Reference< beans::XPropertySet > xPropSet( 
                getConfig( xContext, OUSTR( "/org.openoffice.Setup/L10N" ) ), UNO_QUERY );
        if ( xPropSet.is() )
        {
            OUString aLoc;
            xPropSet->getPropertyValue( OUSTR( "ooLocale" ) ) >>= aLoc;
            const sal_Int32 nPos = aLoc.indexOfAsciiL( "-", 1 );
            OUString aLanguage, aCountry;
            if ( nPos > 0 && aLoc.getLength() > nPos )
            {
                aLanguage = aLoc.copy( 0, nPos +1 );
                aCountry = aLoc.copy( nPos + 1 );
            }
            else
            {
                aLanguage = aLoc;
            }
            aLocale = lang::Locale( aLanguage, aCountry, OUString() );
        }
    }
    catch ( uno::Exception & )
    {
    }
    try
    {
        return resource::StringResourceWithLocation::create( 
                    xContext, 
                    OUSTR( BKMRKS_RES_DIR ), sal_True, aLocale, OUSTR( BKMRKS_RES_FILE ), 
                    OUString(), uno::Reference< task::XInteractionHandler >() );
    }
    catch (uno::Exception &)
    {
    }
    return uno::Reference< resource::XStringResourceWithLocation >();
}

} // namespace