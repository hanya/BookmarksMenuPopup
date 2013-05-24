
#include "bookmarks.hpp"

using namespace ::std;

#define CHARTOOUSTR( s ) ::rtl::OUString( (const sal_Char*)s, (sal_Int32)strlen( s ), RTL_TEXTENCODING_UTF8 )

namespace mytools
{

inline ItemType lcl_getType( const JSON_Object * o )
{
    const char * t = json_object_get_string( o, NAME_TYPE );
    if ( t )
    {
        if ( strcmp( t, TYPE_ITEM ) == 0 )
            return ITEM;
        else if ( strcmp( t, TYPE_CONTAINER ) == 0 )
            return CONTAINER;
        else if ( strcmp( t, TYPE_SEPARATOR ) == 0 )
            return SEPARATOR;
    }
    return ILLEGAL;
}


BaseItem::BaseItem( JSON_Object *o )
 : obj( o )
 , mType( ILLEGAL )
{
}


BaseItem::BaseItem( JSON_Object *o, const ItemType t )
 : obj( o )
 , mType( t )
{
}


BaseItem::~BaseItem()
{
}


ItemType BaseItem::getType()
{
    return mType;
}


sal_Int16 BaseItem::getId()
{
    return (sal_Int16)json_object_get_number( obj, NAME_ID );
}


OUString BaseItem::getName()
{
    const char * s = json_object_get_string( obj, NAME_NAME );
    if ( s != NULL )
        return CHARTOOUSTR( s );
    return OUString();
}


OUString BaseItem::getDescription()
{
    const char * s = json_object_get_string( obj, NAME_DESCRIPTION );
    if ( s )
        return CHARTOOUSTR( s );
    return OUString();
}


Item::Item( JSON_Object *object )
 : BaseItem( object, ITEM )
{
}


Item::Item( Item * i )
 : BaseItem( i->obj, ITEM )
{
}


Item::~Item()
{
}


OUString Item::getCommand( const bool bNoArguments )
{
    const char * s = json_object_get_string( obj, NAME_COMMAND );
    if ( s )
    {
        const OUString sCommand = CHARTOOUSTR( s );
        if ( bNoArguments )
        {
            const sal_Int32 nPos = sCommand.indexOfAsciiL( "?", 1 );
            if ( nPos > 0 )
                return sCommand.copy( 0, nPos );
        }
        return sCommand;
    }
    return OUString();
}


bool Item::hasArguments()
{
    const OUString sCommand = getCommand();
    const sal_Int32 nLength = sCommand.getLength();
    if ( nLength > 0 )
    {
        const sal_Int32 nPos = sCommand.indexOfAsciiL( "?", 1 );
        return ( nPos > 0 && nPos < nLength -1 );
    }
    return false;
}


Container::Container()
{
}


Container::~Container()
{
}


ContainerItem::ContainerItem( JSON_Object *o )
 : BaseItem( o, CONTAINER )
 , Container()
 , ar( NULL )
 , m_bLoaded( false )
{
    JSON_Array *a = json_object_get_array( obj, NAME_CHILDREN );
    if ( a )
        ar = a;
}


ContainerItem::ContainerItem( ContainerItem * i )
 : BaseItem( i->obj, CONTAINER )
 , Container()
 , ar( i->ar )
 , m_bLoaded( i->m_bLoaded )
{
}


ContainerItem::~ContainerItem()
{
    ar = NULL; // will be destroyed by the Manager
    ItemVector::reverse_iterator it;
    for ( it = m_aItems.rbegin(); it != m_aItems.rend(); ++it )
        delete *it;
    m_aItems.clear();
}


int ContainerItem::getChildCount()
{
    return ar ? (int)json_array_get_count( ar ) : 0;
}


BaseItem * ContainerItem::getChild( const int index )
{
    if ( !m_bLoaded )
    {
        m_bLoaded = true;
        readContainer( ar );
    }
    if ( index >= 0 && index < m_aItems.size() )
        return m_aItems.at( (size_t)index );
    return NULL;
}


void ContainerItem::readContainer( JSON_Array * a )
{
    for ( size_t i = 0; i < json_array_get_count( a ); ++i )
    {
        JSON_Object * o = json_array_get_object( a, i );
        switch ( lcl_getType( o ) )
        {
            case ITEM:
                m_aItems.push_back( new Item( o ) );
                break;
            case CONTAINER:
                m_aItems.push_back( new ContainerItem( o ) );
                break;
            case SEPARATOR:
                m_aItems.push_back( new BaseItem( o, SEPARATOR ) );
                break;
            default:
                break;
        }
    }
}


TagContainer::TagContainer( JSON_Object * o, const char * name )
 : Container()
{
    readContainer( o, name );
}


TagContainer::~TagContainer()
{
    ItemVector::reverse_iterator it;
    for ( it = m_aItems.rbegin(); it != m_aItems.rend(); ++it )
        delete *it;
    m_aItems.clear();
}


void TagContainer::readContainer( JSON_Object * o, const char * tag_name )
{
    if ( lcl_getType( o ) == CONTAINER )
    {
        JSON_Array * a = json_object_get_array( o, NAME_CHILDREN );
        if ( a )
        {
            for ( size_t i = 0; i < json_array_get_count( a ); ++i )
            {
                JSON_Object * obj = json_array_get_object( a, i );
                if ( obj )
                {
                    const ItemType t = lcl_getType( obj );
                    if ( t == ITEM )
                    {
                        JSON_Array * tags = json_object_get_array( obj, NAME_TAGS );
                        if ( tags )
                        {
                            for ( size_t j = 0; j < json_array_get_count( tags ); ++j )
                            {
                                if ( strcmp( json_array_get_string( tags, j ), tag_name ) == 0 )
                                {
                                    m_aItems.push_back( new Item( obj ) );
                                    break;
                                }
                            }
                        }
                    }
                    else if ( t == CONTAINER )
                    {
                        readContainer( obj, tag_name );
                    }
                }
            }
        }
    }
}


int TagContainer::getChildCount()
{
    return (int)m_aItems.size();
}


BaseItem * TagContainer::getChild( const int index )
{
    if ( index >= 0 && index < m_aItems.size() )
        return m_aItems.at( (size_t) index );
    return NULL;
}


Manager::Manager( JSON_Value *value )
 : root( value )
{
}


Manager::~Manager()
{
    json_value_free( root );
}


/**
 * Returns root container of Bookmarks data.
 */
ContainerItem * Manager::getRoot()
{
    JSON_Object * obj = json_value_get_object( root );
    JSON_Object * bkmks = json_object_get_object( obj, NAME_BOOKMARKS );
    if ( bkmks )
        return new ContainerItem( bkmks );
    return NULL;
}


void Manager::update( const char * s ) throw ( uno::RuntimeException )
{
    JSON_Value * v = json_parse_string( s );
    if ( json_value_get_type( v ) != JSONObject )
        throw uno::RuntimeException();
    json_value_free( root );
    root = v;
}


Manager * Manager::load( const char * s ) throw ( uno::RuntimeException )
{
    JSON_Value * v = json_parse_string( s );
    if ( json_value_get_type( v ) != JSONObject )
        throw uno::RuntimeException();
    return new Manager( v );
}


bool Manager::hasTag( const char * tag )
{
    if ( root )
    {
        JSON_Object * obj = json_value_get_object( root );
        JSON_Object * tags = json_object_get_object( obj, NAME_TAGS );
        if ( tags )
        {
            size_t count = json_object_get_count( tags );
            for ( size_t i = 0; i < count; ++i )
            {
                if ( strcmp( json_object_get_name( tags, i ), tag ) == 0 )
                    return true;
            }
        }
    }
    return false;
}


TagContainer * Manager::getTagContainer( const char * name )
{
    JSON_Object * obj = json_value_get_object( root );
    JSON_Object * bkmks = json_object_get_object( obj, NAME_BOOKMARKS );
    if ( bkmks )
        return new TagContainer( bkmks, name );
    return NULL;
}


} // namespace
