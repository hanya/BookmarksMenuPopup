
#ifndef BKMPOPUP_HPP
#define BKMPOPUP_HPP

#include <map>

#include <cppuhelper/basemutex.hxx>
#include <cppuhelper/compbase5.hxx>

#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/frame/XPopupMenuController.hpp>
#include <com/sun/star/frame/XStatusListener.hpp>
#include <com/sun/star/awt/XMenuListener.hpp>
#include <com/sun/star/awt/XPopupMenu.hpp>

#include "bookmarks.hpp"
#include "manager.hpp"

#ifdef AOO4
#define UNIFIED_MENU_API
#else
#undef  UNIFIED_MENU_API
#endif
#include <stdio.h>

namespace mytools { 

class Manager;
class BaseItem;
class Item;
class Container;
class ContainerItem;

class BookmarksPopupManager;

struct ControllerWrapper
{
    ::com::sun::star::uno::Reference< ::com::sun::star::frame::XPopupMenuController > xController;
    
    ControllerWrapper( ::uno::Reference< ::com::sun::star::frame::XPopupMenuController > & xC )
    {
        xController = xC;
    }
    
    ~ControllerWrapper()
    {
        xController.clear();
    }
};

typedef ::std::map< ::sal_Int16, BaseItem* > ItemMap;
typedef ::std::map< ::rtl::OUString, ::rtl::OUString > StringMap;
typedef ::std::map< ::sal_Int16, ControllerWrapper* > PopupControllerMap;/*
                    const ::uno::Reference< 
                        ::com::sun::star::frame::XPopupMenuController > > PopupControllerMap; */

/**
 * C++ implementation of popup menu provided by BookmarksMenu extension.
 */

typedef ::cppu::WeakComponentImplHelper5< 
            ::com::sun::star::frame::XPopupMenuController, 
            ::com::sun::star::lang::XInitialization, 
            ::com::sun::star::frame::XDispatchProvider, 
            ::com::sun::star::frame::XStatusListener, 
            ::com::sun::star::awt::XMenuListener
            > Popup_Base;

class BasePopup : protected ::cppu::BaseMutex, 
                       public Popup_Base
{
public :
    
    explicit BasePopup( );
    virtual ~BasePopup();
    
    // XEventListener
    virtual void SAL_CALL disposing( const ::com::sun::star::lang::EventObject& Source ) throw (::com::sun::star::uno::RuntimeException);
    
    // XPopupMenuController
    virtual void SAL_CALL setPopupMenu( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XPopupMenu >& PopupMenu ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL updatePopupMenu(  ) throw (::com::sun::star::uno::RuntimeException);
    
    // XInitialization
    virtual void SAL_CALL initialize( const ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any >& aArguments ) throw (::com::sun::star::uno::Exception, ::com::sun::star::uno::RuntimeException);
    
    // XDispatchProvider
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatch > SAL_CALL queryDispatch( const ::com::sun::star::util::URL& URL, const ::rtl::OUString& TargetFrameName, ::sal_Int32 SearchFlags ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Reference< ::com::sun::star::frame::XDispatch > > SAL_CALL queryDispatches( const ::com::sun::star::uno::Sequence< ::com::sun::star::frame::DispatchDescriptor >& Requests ) throw (::com::sun::star::uno::RuntimeException);
    
    // XStatusListener
    virtual void SAL_CALL statusChanged( const ::com::sun::star::frame::FeatureStateEvent& State ) throw (::com::sun::star::uno::RuntimeException);
    
    // XMenuListener
#ifdef UNIFIED_MENU_API
    virtual void SAL_CALL itemHighlighted( const ::com::sun::star::awt::MenuEvent& rEvent ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL itemSelected( const ::com::sun::star::awt::MenuEvent& rEvent ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL itemActivated( const ::com::sun::star::awt::MenuEvent& rEvent ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL itemDeactivated( const ::com::sun::star::awt::MenuEvent& rEvent ) throw (::com::sun::star::uno::RuntimeException);
#else
    virtual void SAL_CALL highlight( const ::com::sun::star::awt::MenuEvent& rEvent ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL select( const ::com::sun::star::awt::MenuEvent& rEvent ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL activate( const ::com::sun::star::awt::MenuEvent& rEvent ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL deactivate( const ::com::sun::star::awt::MenuEvent& rEvent ) throw (::com::sun::star::uno::RuntimeException);
#endif
protected : 
    
    ::osl::Mutex m_aMutex;
    
    // Default command to execute programs.
    static ::rtl::OUString m_sExecCommand;
    static ::rtl::OUString m_sExecOption;
    
    // Default path to the file manager.
    static ::rtl::OUString m_sFileManager;
    static ::rtl::OUString m_sFileManagerOption;
    
    // Environment has been detected for exec command and file manager.
    static bool m_bEnvDetected;
    
    // Open All command used in menus.
    static ::rtl::OUString m_sOpenAllLabel;
    
    // Popup command to controller name.
    static StringMap m_aPopupMenus;
    
    ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext > m_xContext;
    
    // Command URL of this popup controller.
    ::rtl::OUString m_sCommandURL;
    // Frame that bounded by this popup.
    ::com::sun::star::uno::Any m_aFrame;
    // Managed popup.
    ::com::sun::star::uno::Reference< ::com::sun::star::awt::XPopupMenu > m_xPopupMenu;
    
    // Item id to item having popup menu.
    ItemMap m_aPopupItems;
    
    PopupControllerMap m_aPopupControllers;
    
    // Fill popup menu with items from the container.
    void fillPopup( 
            const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XPopupMenu > & rPopupMenuExt, 
            Container *container, 
            const bool bOpenAll ) throw (::com::sun::star::uno::RuntimeException);
    
    // Prepare to work popup menu.
    void prepareMenu( const bool bClear, const bool bOpenAll );
    
    // Shows message to user.
    sal_Int16 message( const ::rtl::OUString& aTitle, 
                    const ::rtl::OUString& aMessage, const bool bError );
    
    bool m_bUpdated;
    
    Manager * m_pManager;
    BookmarksPopupManager * m_pPopupManager;
    
    virtual void init();
    
    void clearPopupItems();
    
    void clearPopupControllers();
    
    void executeCommand( const ::rtl::OUString & sCommand );
    
    void exec( const OUString & sCommand, const OUString & sArguments );
    
    void openFile( const ::rtl::OUString & sCommand ) const;
    void openFolder( const ::rtl::OUString & sCommand ) const;
    void openWeb( const ::rtl::OUString & sURL ) const;
    
    void initVars();
    void initCommandVars();
    
    virtual Container * getContainer() throw ( ::com::sun::star::uno::RuntimeException );
};

} // namesapce

#endif
