/***************************************************************************
                     qgsidentifyresults.cpp  -  description
                              -------------------
      begin                : Fri Oct 25 2002
      copyright            : (C) 2002 by Gary E.Sherman
      email                : sherman at mrcc dot com
      Romans 3:23=>Romans 6:23=>Romans 5:8=>Romans 10:9,10=>Romans 12
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include "qgsidentifyresults.h"
#include "qgscontexthelp.h"
#include "qgsapplication.h"

#include <QCloseEvent>
#include <QLabel>
#include <QAction>
#include <QTreeWidgetItem>
#include <QPixmap>
#include <QSettings>
#include <QMenu>

#include <iostream>

QgsIdentifyResults::QgsIdentifyResults(const QgsAttributeAction& actions,
    QWidget *parent, Qt::WFlags f)
: QDialog(parent, f),
  mActions(actions),
  mClickedOnValue(0),
  mActionPopup(0)
{
  setupUi(this);
  lstResults->setColumnCount(2);
  setColumnText(0, tr("Feature"));
  setColumnText(1, tr("Value"));

  connect( buttonCancel, SIGNAL(clicked()),
      this, SLOT(close()) );
  connect( lstResults, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
      this, SLOT(clicked(QTreeWidgetItem *)) );
  connect( lstResults, SIGNAL(itemExpanded(QTreeWidgetItem*)),
           this, SLOT(itemExpanded(QTreeWidgetItem*)));
}

QgsIdentifyResults::~QgsIdentifyResults()
{
  saveWindowLocation();
  delete mActionPopup;
}

// Call to show the dialog box.
void QgsIdentifyResults::show()
{
  // Enfore a few things before showing the dialog box
  lstResults->sortItems(0, Qt::Ascending);
  expandColumnsToFit();

  QDialog::show();
}
// Slot called when user clicks the Close button
// (saves the current window size/position)
void QgsIdentifyResults::close()
{
  saveWindowLocation();
  done(0);
}
// Save the current window size/position before closing 
// from window menu or X in titlebar
void QgsIdentifyResults::closeEvent(QCloseEvent *e)
{
  // We'll close in our own good time thanks...
  e->ignore();
  close();
}

// Popup (create if necessary) a context menu that contains a list of
// actions that can be applied to the data in the identify results
// dialog box.

void QgsIdentifyResults::contextMenuEvent(QContextMenuEvent* event)
{
  QTreeWidgetItem* item = lstResults->itemAt(lstResults->viewport()->mapFrom(this, event->pos()));
  // if the user clicked below the end of the attribute list, just return
  if (item == NULL)
    return;
  
  // The assumption is made that an instance of QgsIdentifyResults is
  // created for each new Identify Results dialog box, and that the
  // contents of the popup menu doesn't change during the time that
  // such a dialog box is around.
  if (mActionPopup == 0)
  {
    mActionPopup = new QMenu();
    QAction* a = mActionPopup->addAction( tr("Run action") );
    a->setEnabled(false);
    mActionPopup->addSeparator();

    QgsAttributeAction::aIter iter = mActions.begin();
    for (int j = 0; iter != mActions.end(); ++iter, ++j)
    {
      QAction* a = mActionPopup->addAction(iter->name());
      // The menu action stores an integer that is used later on to
      // associate an menu action with an actual qgis action.
      a->setData(QVariant::fromValue(j));
    }
    connect(mActionPopup, SIGNAL(triggered(QAction*)),
            this, SLOT(popupItemSelected(QAction*)));
  }
  // Save the attribute values as these are needed for substituting into
  // the action. 
  // A little bit complicated because the user could of right-clicked
  // on a parent or a child in the dialog box. We also want to keep
  // track of which row in the identify results table was actually
  // clicked on. This is stored as an index into the mValues vector.

  QTreeWidgetItem* parent = item->parent();
  if (parent == 0)
    parent = item;

  mValues.clear();
  for (int j = 0; j < parent->childCount(); ++j)
  {
    if ( parent->child(j)->text(0) != "action" ) {
      mValues.push_back(std::make_pair(parent->child(j)->text(0), 
                                       parent->child(j)->text(1)));
      // Need to do the comparison on the text strings rather than the
      // pointers because if the user clicked on the parent, we need
      // to pick up which child that actually is (the parent in the
      // identify results dialog box is just one of the children
      // that has been chosen by some method).
      if (parent->child(j)->text(0) == item->text(0))
        mClickedOnValue = j;
    }
  }

  if (mActions.size() > 0)
    mActionPopup->popup(event->globalPos());
}

// Restore last window position/size and show the window
void QgsIdentifyResults::restorePosition()
{

  QSettings settings;
  int ww = settings.readNumEntry("/Windows/Identify/w", 281);
  int wh = settings.readNumEntry("/Windows/Identify/h", 316);
  int wx = settings.readNumEntry("/Windows/Identify/x", 100);
  int wy = settings.readNumEntry("/Windows/Identify/y", 100);
  //std::cerr << "Setting geometry: " << wx << ", " << wy << ", " << ww << ", " << wh << std::endl;
  resize(ww,wh);
  move(wx,wy);
  show();
  //std::cerr << "Current geometry: " << x() << ", " << y() << ", " << width() << ", " << height() << std::endl; 
}
// Save the current window location (store in ~/.qt/qgisrc)
void QgsIdentifyResults::saveWindowLocation()
{
  QSettings settings;
  QPoint p = this->pos();
  QSize s = this->size();
  settings.writeEntry("/Windows/Identify/x", p.x());
  settings.writeEntry("/Windows/Identify/y", p.y());
  settings.writeEntry("/Windows/Identify/w", s.width());
  settings.writeEntry("/Windows/Identify/h", s.height());
} 

/** add an attribute and its value to the list */
void QgsIdentifyResults::addAttribute(QTreeWidgetItem * fnode, QString field, QString value)
{
  QStringList labels;
  labels << field << value;
  new QTreeWidgetItem(fnode, labels);
}

void QgsIdentifyResults::addAttribute(QString field, QString value)
{
  QStringList labels;
  labels << field << value;
  new QTreeWidgetItem(lstResults, labels);
}

void QgsIdentifyResults::addDerivedAttribute(QTreeWidgetItem * fnode, QString field, QString value)
{
  QTreeWidgetItem * daRootNode;

  // Determine if this is the first derived attribute for this
  // feature or not
  if (mDerivedAttributeRootNodes.find(fnode) != mDerivedAttributeRootNodes.end())
  {
    // Reuse existing derived-attribute root node
    daRootNode = mDerivedAttributeRootNodes[fnode];
  }
  else
  {
    // Create new derived-attribute root node
    daRootNode = new QTreeWidgetItem(fnode, QStringList(tr("(Derived)")));
    QFont font = daRootNode->font(0);
    font.setItalic(true);
    daRootNode->setFont(0, font);
  }

  QStringList labels;
  labels << field << value;
  new QTreeWidgetItem(daRootNode, labels);
}

void QgsIdentifyResults::addAction(QTreeWidgetItem * fnode, int id, QString field, QString value)
{
  QStringList labels;
  labels << field << value << "action" << QString::number(id);
  QTreeWidgetItem *item = new QTreeWidgetItem(fnode, labels );

  QPixmap pm ( QgsApplication::themePath() + "/mAction.png" );
  item->setIcon ( 0, QIcon(pm) ); 
}

/** Add a feature node to the list */
QTreeWidgetItem *QgsIdentifyResults::addNode(QString label)
{
  return (new QTreeWidgetItem(lstResults, QStringList(label)));
}

void QgsIdentifyResults::setTitle(QString title)
{
  setWindowTitle(tr("Identify Results - ") + title);
}

void QgsIdentifyResults::setColumnText ( int column, const QString & label )
{
  QTreeWidgetItem* header = lstResults->headerItem();
  header->setText ( column, label );
}

// Run the action that was selected in the popup menu
void QgsIdentifyResults::popupItemSelected(QAction* menuAction)
{
  int id = menuAction->data().toInt();
  mActions.doAction(id, mValues, mClickedOnValue);
}

/** Expand all the identified features (show their attributes). */
void QgsIdentifyResults::showAllAttributes() {
  QTreeWidgetItemIterator qlvii(lstResults);
  for ( ; *qlvii; ++qlvii)
    lstResults->setItemExpanded(*qlvii, true);
}

void QgsIdentifyResults::expandColumnsToFit()
{
  lstResults->resizeColumnToContents(0);
  lstResults->resizeColumnToContents(1);
}

void QgsIdentifyResults::clear()
{
  lstResults->clear();
}

void QgsIdentifyResults::setMessage( QString shortMsg, QString longMsg )
{
  QStringList labels;
  labels << shortMsg << longMsg;
  new QTreeWidgetItem(lstResults, labels );
}

void QgsIdentifyResults::setActions( const QgsAttributeAction& actions  )
{
  mActions = actions;
}

void QgsIdentifyResults::clicked ( QTreeWidgetItem *item )
{
  if ( !item ) return;

  if ( item->text(2) != "action" ) return;

  int id = item->text(3).toInt();

  QTreeWidgetItem* parent = item->parent();
  if (parent == 0)
    parent = item;

  mValues.clear();

  for (int j = 0; j < parent->childCount(); ++j)
  {
    if ( parent->child(j)->text(0) != "action" ) {
      mValues.push_back(std::make_pair(parent->child(j)->text(0), 
                                       parent->child(j)->text(1)));
      if (parent->child(j)->text(0) == item->text(0))
        mClickedOnValue = j;
    }
  }

  mActions.doAction(id, mValues, mClickedOnValue);
}
void QgsIdentifyResults::on_buttonHelp_clicked()
{
  QgsContextHelp::run(context_id);
}

void QgsIdentifyResults::itemExpanded(QTreeWidgetItem* item)
{
  expandColumnsToFit();
}
