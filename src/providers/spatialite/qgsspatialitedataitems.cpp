#include "qgsspatialitedataitems.h"

#include "qgsspatialiteprovider.h"
#include "qgsspatialiteconnection.h"
#include "qgsspatialitesourceselect.h"

#include "qgslogger.h"
#include "qgsmimedatautils.h"
#include "qgsvectorlayerimport.h"

#include <QAction>
#include <QMessageBox>
#include <QSettings>

QGISEXTERN bool deleteLayer( const QString& dbPath, const QString& tableName, QString& errCause );

QgsSLLayerItem::QgsSLLayerItem( QgsDataItem* parent, QString name, QString path, QString uri, LayerType layerType )
    : QgsLayerItem( parent, name, path, uri, layerType, "spatialite" )
{
  mPopulated = true; // no children are expected
}

QList<QAction*> QgsSLLayerItem::actions()
{
  QList<QAction*> lst;

  QAction* actionDeleteLayer = new QAction( tr( "Delete layer" ), this );
  connect( actionDeleteLayer, SIGNAL( triggered() ), this, SLOT( deleteLayer() ) );
  lst.append( actionDeleteLayer );

  return lst;
}

void QgsSLLayerItem::deleteLayer()
{
  QgsDataSourceURI uri( mUri );
  QString errCause;
  bool res = ::deleteLayer( uri.database(), uri.table(), errCause );
  if ( !res )
  {
    QMessageBox::warning( 0, tr( "Delete layer" ), errCause );
  }
  else
  {
    QMessageBox::information( 0, tr( "Delete layer" ), tr( "Layer deleted successfully." ) );
    mParent->refresh();
  }
}

// ------

QgsSLConnectionItem::QgsSLConnectionItem( QgsDataItem* parent, QString name, QString path )
    : QgsDataCollectionItem( parent, name, path )
{
  mDbPath = QgsSpatiaLiteConnection::connectionPath( name );
  mToolTip = mDbPath;
}

QgsSLConnectionItem::~QgsSLConnectionItem()
{
}

static QgsLayerItem::LayerType _layerTypeFromDb( QString dbType )
{
  if ( dbType == "POINT" || dbType == "MULTIPOINT" )
  {
    return QgsLayerItem::Point;
  }
  else if ( dbType == "LINESTRING" || dbType == "MULTILINESTRING" )
  {
    return QgsLayerItem::Line;
  }
  else if ( dbType == "POLYGON" || dbType == "MULTIPOLYGON" )
  {
    return QgsLayerItem::Polygon;
  }
  else if ( dbType == "qgis_table" )
  {
    return QgsLayerItem::Table;
  }
  else
  {
    return QgsLayerItem::NoType;
  }
}

QVector<QgsDataItem*> QgsSLConnectionItem::createChildren()
{
  QgsDebugMsg( "Entered" );
  QVector<QgsDataItem*> children;
  QgsSpatiaLiteConnection connection( mName );

  QgsSpatiaLiteConnection::Error err = connection.fetchTables( false ); // TODO: allow geometryless tables
  if ( err != QgsSpatiaLiteConnection::NoError )
  {
    QString msg;
    switch ( err )
    {
      case QgsSpatiaLiteConnection::NotExists: msg = tr( "Database does not exist" ); break;
      case QgsSpatiaLiteConnection::FailedToOpen: msg = tr( "Failed to open database" ); break;
      case QgsSpatiaLiteConnection::FailedToCheckMetadata: msg = tr( "Failed to check metadata" ); break;
      case QgsSpatiaLiteConnection::FailedToGetTables: msg = tr( "Failed to get list of tables" ); break;
      default: msg = tr( "Unknown error" ); break;
    }
    QString msgDetails = connection.errorMessage();
    if ( !msgDetails.isEmpty() )
      msg = QString( "%1 (%2)" ).arg( msg ).arg( msgDetails );
    children.append( new QgsErrorItem( this, msg, mPath + "/error" ) );
    return children;
  }

  QString connectionInfo = QString( "dbname='%1'" ).arg( QString( connection.path() ).replace( "'", "\\'" ) );
  QgsDataSourceURI uri( connectionInfo );

  foreach( const QgsSpatiaLiteConnection::TableEntry& entry, connection.tables() )
  {
    uri.setDataSource( QString(), entry.tableName, entry.column, QString(), QString() );
    QgsSLLayerItem * layer = new QgsSLLayerItem( this, entry.tableName, mPath + "/" + entry.tableName, uri.uri(), _layerTypeFromDb( entry.type ) );
    children.append( layer );
  }
  return children;
}

bool QgsSLConnectionItem::equal( const QgsDataItem *other )
{
  if ( type() != other->type() )
  {
    return false;
  }
  const QgsSLConnectionItem *o = dynamic_cast<const QgsSLConnectionItem *>( other );
  return ( mPath == o->mPath && mName == o->mName );
}

QList<QAction*> QgsSLConnectionItem::actions()
{
  QList<QAction*> lst;

  //QAction* actionEdit = new QAction( tr( "Edit..." ), this );
  //connect( actionEdit, SIGNAL( triggered() ), this, SLOT( editConnection() ) );
  //lst.append( actionEdit );

  QAction* actionDelete = new QAction( tr( "Delete" ), this );
  connect( actionDelete, SIGNAL( triggered() ), this, SLOT( deleteConnection() ) );
  lst.append( actionDelete );

  return lst;
}

void QgsSLConnectionItem::editConnection()
{
}

void QgsSLConnectionItem::deleteConnection()
{
  QgsSpatiaLiteConnection::deleteConnection( mName );
  // the parent should be updated
  mParent->refresh();
}

bool QgsSLConnectionItem::handleDrop( const QMimeData * data, Qt::DropAction )
{
  if ( !QgsMimeDataUtils::isUriList( data ) )
    return false;

  // TODO: probably should show a GUI with settings etc

  QgsDataSourceURI destUri;
  destUri.setDatabase( mDbPath );

  qApp->setOverrideCursor( Qt::WaitCursor );

  QStringList importResults;
  bool hasError = false;
  QgsMimeDataUtils::UriList lst = QgsMimeDataUtils::decodeUriList( data );
  foreach( const QgsMimeDataUtils::Uri& u, lst )
  {
    if ( u.layerType != "vector" )
    {
      importResults.append( tr( "%1: Not a vector layer!" ).arg( u.name ) );
      hasError = true; // only vectors can be imported
      continue;
    }

    // open the source layer
    QgsVectorLayer* srcLayer = new QgsVectorLayer( u.uri, u.name, u.providerKey );

    if ( srcLayer->isValid() )
    {
      destUri.setDataSource( QString(), u.name, "geomm" );
      QgsDebugMsg( "URI " + destUri.uri() );
      QgsVectorLayerImport::ImportError err;
      QString importError;
      err = QgsVectorLayerImport::importLayer( srcLayer, destUri.uri(), "spatialite", &srcLayer->crs(), false, &importError );
      if ( err == QgsVectorLayerImport::NoError )
        importResults.append( tr( "%1: OK!" ).arg( u.name ) );
      else
      {
        importResults.append( QString( "%1: %2" ).arg( u.name ).arg( importError ) );
        hasError = true;
      }
    }
    else
    {
      importResults.append( tr( "%1: OK!" ).arg( u.name ) );
      hasError = true;
    }

    delete srcLayer;
  }

  qApp->restoreOverrideCursor();

  if ( hasError )
  {
    QMessageBox::warning( 0, tr( "Import to SpatiaLite database" ), tr( "Failed to import some layers!\n\n" ) + importResults.join( "\n" ) );
  }
  else
  {
    QMessageBox::information( 0, tr( "Import to SpatiaLite database" ), tr( "Import was successful." ) );
  }

  refresh();

  return true;
}


// ---------------------------------------------------------------------------

QgsSLRootItem::QgsSLRootItem( QgsDataItem* parent, QString name, QString path )
    : QgsDataCollectionItem( parent, name, path )
{
  mIcon = QIcon( getThemePixmap( "mIconSpatialite.png" ) );
  populate();
}

QgsSLRootItem::~QgsSLRootItem()
{
}

QVector<QgsDataItem*> QgsSLRootItem::createChildren()
{
  QVector<QgsDataItem*> connections;
  foreach( QString connName, QgsSpatiaLiteConnection::connectionList() )
  {
    QgsDataItem * conn = new QgsSLConnectionItem( this, connName, mPath + "/" + connName );
    connections.push_back( conn );
  }
  return connections;
}

QList<QAction*> QgsSLRootItem::actions()
{
  QList<QAction*> lst;

  QAction* actionNew = new QAction( tr( "New Connection..." ), this );
  connect( actionNew, SIGNAL( triggered() ), this, SLOT( newConnection() ) );
  lst.append( actionNew );

  QAction* actionCreateDatabase = new QAction( tr( "Create database..." ), this );
  connect( actionCreateDatabase, SIGNAL( triggered() ), this, SLOT( createDatabase() ) );
  lst.append( actionCreateDatabase );

  return lst;
}

QWidget * QgsSLRootItem::paramWidget()
{
  QgsSpatiaLiteSourceSelect *select = new QgsSpatiaLiteSourceSelect( 0, 0, true );
  connect( select, SIGNAL( connectionsChanged() ), this, SLOT( connectionsChanged() ) );
  return select;
}

void QgsSLRootItem::connectionsChanged()
{
  refresh();
}

void QgsSLRootItem::newConnection()
{
  if ( QgsSpatiaLiteSourceSelect::newConnection( NULL ) )
  {
    refresh();
  }
}

QGISEXTERN bool createDb( const QString& dbPath, QString& errCause );

void QgsSLRootItem::createDatabase()
{
  QSettings settings;
  QString lastUsedDir = settings.value( "/UI/lastSpatiaLiteDir", "." ).toString();

  QString filename = QFileDialog::getSaveFileName( 0, tr( "New SpatiaLite Database File" ),
                     lastUsedDir,
                     tr( "SpatiaLite" ) + " (*.sqlite *.db)" );
  if ( filename.isEmpty() )
    return;

  QString errCause;
  if ( ::createDb( filename, errCause ) )
  {
    QMessageBox::information( 0, tr( "Create SpatiaLite database" ), tr( "The database has been created" ) );

    // add connection
    QString baseKey = "/SpatiaLite/connections/";
    QSettings settings;
    settings.setValue( baseKey + "/sqlitepath", filename );

    refresh();
  }
  else
  {
    QMessageBox::critical( 0, tr( "Create SpatiaLite database" ), tr( "Failed to create the database:\n" ) + errCause );
  }

}

// ---------------------------------------------------------------------------

QGISEXTERN QgsSpatiaLiteSourceSelect * selectWidget( QWidget * parent, Qt::WFlags fl )
{
  // TODO: this should be somewhere else
  return new QgsSpatiaLiteSourceSelect( parent, fl, false );
}

QGISEXTERN int dataCapabilities()
{
  return  QgsDataProvider::Database;
}

QGISEXTERN QgsDataItem * dataItem( QString thePath, QgsDataItem* parentItem )
{
  Q_UNUSED( thePath );
  return new QgsSLRootItem( parentItem, "SpatiaLite", "spatialite:" );
}
