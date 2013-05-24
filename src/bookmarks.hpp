
#ifndef _BOOKMARKS_HPP_
#define _BOOKMARKS_HPP_

#include <vector>
#include <string.h>
#include "parson.h"

#include <rtl/ustring.hxx>

#include <com/sun/star/uno/RuntimeException.hpp>

using ::rtl::OUString;
using namespace ::com::sun::star;

#define NAME_BOOKMARKS "bookmarks"

#define NAME_ID          "id"
#define NAME_TYPE        "type"
#define NAME_PARENT      "parent"
#define NAME_NAME        "name"
#define NAME_COMMAND     "command"
#define NAME_DESCRIPTION "description"
#define NAME_CHILDREN    "children"
#define NAME_TAGS        "tags"

#define TYPE_ITEM       "item"
#define TYPE_SEPARATOR  "separator"
#define TYPE_CONTAINER  "container"

#define QUERY_NAME_PATH      "Path:string"
#define QUERY_NAME_ARGUMENTS "Arguments:string"


namespace mytools {

/**
 * Type of command kept by bookmark item.
 */
enum CommandType {
    UNKNOWN, 
    PROGRAM, 
    FILE, 
    FOLDER, 
    WEB, 
    EDIT, 
    ADDTHIS
};

/**
 * Type of bookmark item.
 */
enum ItemType {
    ILLEGAL, 
    ITEM, 
    CONTAINER, 
    SEPARATOR
};

/**
 * Item base.
 */
class BaseItem
{
protected:
    JSON_Object *obj;
    const ItemType mType;
public:
    BaseItem( JSON_Object *obj );
    BaseItem( JSON_Object *obj, const ItemType t );
    virtual ~BaseItem();
    
    virtual ItemType getType();
    virtual sal_Int16 getId();
    
    virtual OUString getName();
    virtual OUString getDescription();
};


/**
 * Normal item having commands.
 */
class Item : public BaseItem
{
public:
    Item( JSON_Object *obj );
    Item( Item * i );
    virtual ~Item();
    
    virtual OUString getCommand( const bool bNoArguments = 0 );
    virtual bool hasArguments();
};

typedef ::std::vector< BaseItem * > ItemVector;

class Container
{
public:
    Container();
    virtual ~Container();
    
    virtual int getChildCount() = 0;
    virtual BaseItem *getChild( const int index ) = 0;
};


/**
 * Container item.
 */
class ContainerItem : public BaseItem, public Container
{
public:
    ContainerItem( JSON_Object *o );
    ContainerItem( ContainerItem * i );
    virtual ~ContainerItem();
    
    virtual int getChildCount();
    virtual BaseItem *getChild( const int index );
protected:
    bool m_bLoaded;
    JSON_Array *ar;
    ItemVector m_aItems;
    void readContainer( JSON_Array * ar );
};



/**
 * Keeps items haveing specific tag.
 */
class TagContainer : public Container
{
public:
    TagContainer( JSON_Object * o, const char * name );
    virtual ~TagContainer();
    
    
    virtual int getChildCount();
    virtual BaseItem *getChild( const int index );
protected:
    ItemVector m_aItems;
    void readContainer( JSON_Object * o, const char * tag_name );
};


/**
 * Keeps root element of JSON data.
 */
class Manager 
{
    JSON_Value *root;
public:
    Manager( JSON_Value *value );
    ~Manager();
    
    ContainerItem *getRoot();
    
    // ToDo cache tag names into hash
    bool hasTag( const char * tag );
    
    // collect items
    TagContainer * getTagContainer( const char * name );
    
    /** Replace current root value. */
    void update( const char * s )  throw ( uno::RuntimeException );
    /**
     * Load data from path.
     */
    static Manager * load( const char * s ) throw ( uno::RuntimeException );
};


} // namespace

#endif
