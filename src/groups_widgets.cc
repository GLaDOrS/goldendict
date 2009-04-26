/* This file is (c) 2008-2009 Konstantin Isakov <ikm@users.berlios.de>
 * Part of GoldenDict. Licensed under GPLv3 or later, see the LICENSE file */

#include "groups_widgets.hh"

#include "instances.hh"
#include "config.hh"
#include "langcoder.hh"

#include <QMenu>
#include <QDir>
#include <QIcon>

using std::vector;

/// DictGroupWidget

DictGroupWidget::DictGroupWidget( QWidget * parent,
                                  vector< sptr< Dictionary::Class > > const & dicts,
                                  Config::Group const & group ):
  QWidget( parent ),
  groupId( group.id )
{
  ui.setupUi( this );
  ui.dictionaries->populate( Instances::Group( group, dicts ).dictionaries, dicts );

  // Populate icons' list

  QStringList icons = QDir( ":/flags/" ).entryList( QDir::Files, QDir::NoSort );

  ui.groupIcon->addItem( "None", "" );

  for( int x = 0; x < icons.size(); ++x )
  {
    QString n( icons[ x ] );
    n.chop( 4 );
    n[ 0 ] = n[ 0 ].toUpper();

    ui.groupIcon->addItem( QIcon( ":/flags/" + icons[ x ] ), n, icons[ x ] );

    if ( icons[ x ] == group.icon )
      ui.groupIcon->setCurrentIndex( x + 1 );
  }
}

Config::Group DictGroupWidget::makeGroup() const
{
  Instances::Group g( "" );

  g.id = groupId;

  g.dictionaries = ui.dictionaries->getCurrentDictionaries();

  g.icon = ui.groupIcon->itemData( ui.groupIcon->currentIndex() ).toString();

  return g.makeConfigGroup();
}

/// DictListModel

void DictListModel::populate(
  std::vector< sptr< Dictionary::Class > > const & active,
  std::vector< sptr< Dictionary::Class > > const & available )
{
  dictionaries = active;
  allDicts = &available;

  reset();
}

void DictListModel::setAsSource()
{
  isSource = true;
}

std::vector< sptr< Dictionary::Class > > const &
  DictListModel::getCurrentDictionaries() const
{
  return dictionaries;
}

void DictListModel::removeSelectedRows( QItemSelectionModel * source )
{
  if ( !source )
    return;

  QModelIndexList rows = source->selectedRows();

  if ( !rows.count() )
    return;

  for ( int i = rows.count()-1; i >= 0; --i )
  {
    dictionaries.erase( dictionaries.begin() + rows.at( i ).row() );
  }

  reset();
}

void DictListModel::addSelectedUniqueFromModel( QItemSelectionModel * source )
{
  if ( !source )
    return;

  QModelIndexList rows = source->selectedRows();

  if ( !rows.count() )
    return;

  const DictListModel * baseModel = dynamic_cast< const DictListModel * > ( source->model() );
  if ( !baseModel )
    return;

  QVector< Dictionary::Class * > list;
  QVector< Dictionary::Class * > dicts;
  for ( int i = 0; i < dictionaries.size(); i++ )
    dicts.append( dictionaries.at( i ).get() );

  for ( int i = 0; i < rows.count(); i++ )
  {
    Dictionary::Class * d =
        static_cast< Dictionary::Class * > ( rows.at( i ).internalPointer() );

    //qDebug() << "rows.at( i ).internalPointer() " << rows.at( i ).internalPointer();

    if ( !d )
      continue;

//    sptr< Dictionary::Class > s ( d );
//
//    if ( std::find( dictionaries.begin(), dictionaries.end(), s ) == dictionaries.end() )
//      continue;
//
//    qDebug() << "std::find ";

    if ( !dicts.contains( d ) )
      list.append( d );

    //list.push_back( s );
  }

  if ( list.empty() )
    return;

//  qDebug() << "list " << list.size();

  for ( int i = 0; i < list.size(); i++ )
    dictionaries.push_back( sptr< Dictionary::Class > ( list.at( i ) ) );

  reset();
}

Qt::ItemFlags DictListModel::flags( QModelIndex const & index ) const
{
  Qt::ItemFlags defaultFlags = QAbstractItemModel::flags( index );

  if (index.isValid())
     return Qt::ItemIsDragEnabled | defaultFlags;
  else
     return Qt::ItemIsDropEnabled | defaultFlags;
}

int DictListModel::rowCount( QModelIndex const & ) const
{
  return dictionaries.size();
}

QVariant DictListModel::data( QModelIndex const & index, int role ) const
{
  sptr< Dictionary::Class > const & item = dictionaries[ index.row() ];

  if ( !item )
    return QVariant();

  switch ( index.column() )
  {
    case 0:
    {
      switch ( role )
      {
        case Qt::DisplayRole :
          return QString::fromUtf8( item->getName().c_str() );
    //          + QString("  lang: %1 %2").arg( langCoder.decode(item->getLangFrom()),
    //                                        langCoder.decode(item->getLangTo()) );

        case Qt::EditRole :
          return QString::fromUtf8( item->getId().c_str() );

        case Qt::DecorationRole:
          return item->getIcon();

        default:;
      }

      break;
    }

    case 1:
    {
      switch ( role )
      {
        case Qt::DisplayRole :
          return LangCoder::decode( item->getLangFrom() ).left( 3 );

        case Qt::EditRole :
          break;

        case Qt::DecorationRole:
          return LangCoder::icon( item->getLangFrom() );

        default:;
      }

      break;
    }

    case 2:
    {
      switch ( role )
      {
        case Qt::DisplayRole :
          return LangCoder::decode( item->getLangTo() ).left( 3 );

        case Qt::EditRole :
          break;

        case Qt::DecorationRole:
          return LangCoder::icon( item->getLangTo() );

        default:;
      }

      break;
    }

    default:;

  }

  return QVariant();
}

bool DictListModel::insertRows( int row, int count, const QModelIndex & parent )
{
  if ( isSource )
    return false;

  beginInsertRows( parent, row, row + count - 1 );
  dictionaries.insert( dictionaries.begin() + row, count,
                       sptr< Dictionary::Class >() );
  endInsertRows();

  return true;
}

bool DictListModel::removeRows( int row, int count,
                                const QModelIndex & parent )
{
  if ( isSource )
    return false;

  beginRemoveRows( parent, row, row + count - 1 );
  dictionaries.erase( dictionaries.begin() + row,
                      count == INT_MAX ? dictionaries.end() : dictionaries.begin() + row + count );
  endRemoveRows();

  return true;
}

bool DictListModel::setData( QModelIndex const & index, const QVariant & value,
                             int role )
{
  if ( isSource || !allDicts || !index.isValid() ||
       index.row() >= (int)dictionaries.size() )
    return false;

  if ( ( role == Qt::DisplayRole ) || ( role ==  Qt::DecorationRole ) )
  {
    // Allow changing that, but do nothing
    return true;
  }

//  if ( role == Qt::EditRole )
//  {
//    Config::Group g;
//
//    g.dictionaries.push_back( Config::DictionaryRef( value.toString(), QString() ) );
//
//    Instances::Group i( g, *allDicts );
//
//    if ( i.dictionaries.size() == 1 )
//    {
//      // Found that dictionary
//      dictionaries[ index.row() ] = i.dictionaries.front();
//
//      emit dataChanged( index, index );
//
//      return true;
//    }
//  }

  return false;
}

Qt::DropActions DictListModel::supportedDropActions() const
{
  return 0; //Qt::MoveAction;
}

QModelIndex DictListModel::index( int row, int column, const QModelIndex &parent ) const
{
  return createIndex( row, column, dictionaries.at( row ).get() );
}

QModelIndex DictListModel::parent( const QModelIndex & child ) const
{
  return QModelIndex();
}

QVariant DictListModel::headerData ( int section, Qt::Orientation orientation, int role ) const
{
  if ( role == Qt::DisplayRole )
  {
    switch ( section )
    {
      case 0:     return tr( "Dictionary" );
      case 1:     return tr( "Source" );
      case 2:     return tr( "Target" );
      default:;
    }
  }

  return QAbstractItemModel::headerData( section, orientation, role );
}

/// DictListWidget

DictListWidget::DictListWidget( QWidget * parent ): QTreeView( parent ),
  model( this )
{
  setModel( &model );

  setSelectionMode( ExtendedSelection );

  setDragEnabled( false );
  setAcceptDrops( false );
  setDropIndicatorShown( false );

  setRootIsDecorated( false );
  setItemsExpandable( false );
//  setSortingEnabled( true );

  header()->setAlternatingRowColors( true );
  header()->setResizeMode( QHeaderView::ResizeToContents );
//  header()->setSortIndicatorShown( true );
//  header()->setSortIndicator( 1, Qt::AscendingOrder );
}

DictListWidget::~DictListWidget()
{
  setModel( 0 );
}

void DictListWidget::populate(
  std::vector< sptr< Dictionary::Class > > const & active,
  std::vector< sptr< Dictionary::Class > > const & available )
{
  model.populate( active, available );
}

void DictListWidget::setAsSource()
{
  setDropIndicatorShown( false );
  model.setAsSource();
}

std::vector< sptr< Dictionary::Class > > const &
  DictListWidget::getCurrentDictionaries() const
{
  return model.getCurrentDictionaries();
}

// DictGroupsWidget

DictGroupsWidget::DictGroupsWidget( QWidget * parent ):
  QTabWidget( parent ), nextId( 1 ), allDicts( 0 )
{
  setMovable( true );
}

namespace {

QString escapeAmps( QString const & str )
{
  QString result( str );
  result.replace( "&", "&&" );
  return result;
}

QString unescapeAmps( QString const & str )
{
  QString result( str );
  result.replace( "&&", "&" );
  return result;
}

}

void DictGroupsWidget::populate( Config::Groups const & groups,
                                 vector< sptr< Dictionary::Class > > const & allDicts_ )
{
  while( count() )
    removeCurrentGroup();

  allDicts = &allDicts_;

  for( unsigned x = 0; x < groups.size(); ++x )
    addTab( new DictGroupWidget( this, *allDicts, groups[ x ] ), escapeAmps( groups[ x ].name ) );

  nextId = groups.nextId;
}

/// Creates groups from what is currently set up
Config::Groups DictGroupsWidget::makeGroups() const
{
  Config::Groups result;

  result.nextId = nextId;

  for( int x = 0; x < count(); ++x )
  {
    result.push_back( dynamic_cast< DictGroupWidget & >( *widget( x ) ).makeGroup() );
    result.back().name = unescapeAmps( tabText( x ) );
  }

  return result;
}

DictListModel * DictGroupsWidget::getCurrentModel() const
{
  int current = currentIndex();

  if ( current >= 0 )
  {
    DictGroupWidget * w = ( DictGroupWidget * ) widget( current );
    return w->getModel();
  }

  return 0;
}

QItemSelectionModel * DictGroupsWidget::getCurrentSelectionModel() const
{
  int current = currentIndex();

  if ( current >= 0 )
  {
    DictGroupWidget * w = ( DictGroupWidget * ) widget( current );
    return w->getSelectionModel();
  }

  return 0;
}

void DictGroupsWidget::addNewGroup( QString const & name )
{
  if ( !allDicts )
    return;

  int idx = currentIndex() + 1;

  Config::Group newGroup;

  newGroup.id = nextId++;

  insertTab( idx,
             new DictGroupWidget( this, *allDicts, newGroup ),
             escapeAmps( name ) );

  setCurrentIndex( idx );
}

QString DictGroupsWidget::getCurrentGroupName() const
{
  int current = currentIndex();

  if ( current >= 0 )
    return unescapeAmps( tabText( current ) );

  return QString();
}

void DictGroupsWidget::renameCurrentGroup( QString const & name )
{
  int current = currentIndex();

  if ( current >= 0 )
    setTabText( current, escapeAmps( name ) );
}

void DictGroupsWidget::emptyCurrentGroup()
{
  int current = currentIndex();

  if ( current >= 0 )
  {
    DictGroupWidget * w = ( DictGroupWidget * ) widget( current );
    w->getModel()->removeRows( );
  }
}

void DictGroupsWidget::removeCurrentGroup()
{
  int current = currentIndex();

  if ( current >= 0 )
  {
    QWidget * w = widget( current );
    removeTab( current );
    delete w;
  }
}

void DictGroupsWidget::removeAllGroups()
{
  while ( count() )
  {
    QWidget * w = widget( 0 );
    removeTab( 0 );
    delete w;
  }
}

