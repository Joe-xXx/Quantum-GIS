/***************************************************************************
                          qgsdlgvectorlayerproperties.cpp
                   Unified property dialog for vector layers
                             -------------------
    begin                : 2004-01-28
    copyright            : (C) 2004 by Gary E.Sherman
    email                : sherman at mrcc.com
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
#include <qstring.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qlistview.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qtextstream.h>
#include <qtable.h>
#include <qlayout.h>

#include "qgis.h"
#include "qgsrect.h"
#include "qgsfield.h"
#include "qgsdlgvectorlayerproperties.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"
#include "qgssinglesymrenderer.h"
#include "qgsgraduatedmarenderer.h"
#include "qgsgraduatedsymrenderer.h"
#include "qgscontinuouscolrenderer.h"
#include "qgsuniquevalrenderer.h"
#include "qgsuvalmarenderer.h"
#include "qgssimarenderer.h"
#include "qgssimadialog.h"
#include "qgslegenditem.h"
#include <qwidgetstack.h>
#include <qpushbutton.h>
#include "qgssisydialog.h"
#include "qgsgramadialog.h"
#include "qgsgrasydialog.h"
#include "qgscontcoldialog.h"
#include "qgsuvaldialog.h"
#include "qgsuvalmadialog.h"
#include "qobjectlist.h"
#include "qgsgramadialog.h"
#include "qgslabelattributes.h"
#include "qgslabel.h"
#include "qgslabeldialog.h"


QgsDlgVectorLayerProperties::QgsDlgVectorLayerProperties(QgsVectorLayer * lyr, QWidget * parent, const char *name):QgsDlgVectorLayerPropertiesBase(parent, name), layer(lyr), rendererDirty(false), bufferDialog(layer->rendererDialog()),
bufferRenderer(layer->
               renderer())
{
  // populate the general information
  QString source = layer->source();
  source = source.left(source.find("password"));
  lblSource->setText(source);
  txtDisplayName->setText(layer->name());
  // display type and feature count
  lblGeometryType->setText(QGis::qgisVectorGeometryType[layer->vectorType()]);
  QgsVectorDataProvider *dp = layer->getDataProvider();
  QString numFeatures;
  numFeatures = numFeatures.setNum(dp->featureCount());
  lblFeatureCount->setText(numFeatures);
  QgsRect *extent = dp->extent();
  QString ll;
  //QTextOStream (&ll) << extent->xMin() << ", " << extent->yMin();
  lblLowerLeft->setText(ll.sprintf("%16f, %16f", extent->xMin(), extent->yMin()));
  QString ur;
//  QTextOStream (&ur) << extent->xMax() << ", " << extent->yMax(); 
  lblUpperRight->setText(ur.sprintf("%16f, %16f", extent->xMax(), extent->yMax()));
  std::vector < QgsField > fields = dp->fields();
  // populate the table and the display field drop-down with the field information
  for (int i = 0; i < fields.size(); i++)
    {
      QgsField fld = fields[i];
      QListViewItem *lvi = new QListViewItem(listViewFields, fld.name(),
                                             fld.type(), QString("%1").arg(fld.length()),
                                             QString("%1").arg(fld.precision()));
    }
  // symbology initialization
  legendtypecombobox->insertItem(tr("Single Symbol"));
  legendtypecombobox->insertItem(tr("Graduated Symbol"));
  legendtypecombobox->insertItem(tr("Continuous Color"));
  legendtypecombobox->insertItem(tr("Unique Value"));
  if( layer->vectorType()==QGis::Point )
  {
      legendtypecombobox->insertItem(tr("Single Marker"));
      legendtypecombobox->insertItem(tr("Graduated Marker"));
      legendtypecombobox->insertItem(tr("Unique Value Marker"));
  }

  QVBoxLayout *layout = new QVBoxLayout( labelOptionsFrame );
  labelDialog = new QgsLabelDialog ( layer->label(),labelOptionsFrame);
  layout->addWidget( labelDialog );

  QObject::connect(legendtypecombobox, SIGNAL(activated(const QString &)), this, SLOT(alterLayerDialog(const QString &)));
  QObject::connect(btnApply, SIGNAL(clicked()), this, SLOT(apply()));
  QObject::connect(btnClose, SIGNAL(clicked()), this, SLOT(close()));

  //insert the renderer dialog of the vector layer into the widget stack
  widgetStackRenderers->addWidget(bufferDialog);
  widgetStackRenderers->raiseWidget(bufferDialog);

}

QgsDlgVectorLayerProperties::~QgsDlgVectorLayerProperties()
{
  widgetStackRenderers->removeWidget(bufferDialog);
  if (rendererDirty)
    {
      delete bufferDialog;
      delete bufferRenderer;
    }
}

void QgsDlgVectorLayerProperties::alterLayerDialog(const QString & string)
{
  if (rendererDirty)
    {
      widgetStackRenderers->removeWidget(bufferDialog);
      delete bufferDialog;
      delete bufferRenderer;
    }
  //create a new Dialog
  if (string == tr("Single Symbol"))
    {
      bufferRenderer = new QgsSingleSymRenderer();
  } else if (string == tr("Graduated Symbol"))
    {
      bufferRenderer = new QgsGraduatedSymRenderer();
  } else if (string == tr("Continuous Color"))
    {
      bufferRenderer = new QgsContinuousColRenderer();
  } else if (string == tr("Single Marker"))
  {
      bufferRenderer = new QgsSiMaRenderer();
  } else if (string == tr("Graduated Marker"))
  {
      bufferRenderer = new QgsGraduatedMaRenderer();
  } else if(string == tr("Unique Value"))
  {
      bufferRenderer = new QgsUniqueValRenderer();
  } else if(string == tr("Unique Value Marker"))
  {
      bufferRenderer = new QgsUValMaRenderer();
  }
  bufferRenderer->initializeSymbology(layer, this);

  widgetStackRenderers->addWidget(bufferDialog);
  widgetStackRenderers->raiseWidget(bufferDialog);
  rendererDirty = true;
}

void QgsDlgVectorLayerProperties::setRendererDirty(bool enabled)
{
  rendererDirty = enabled;
}

void QgsDlgVectorLayerProperties::apply()
{
  if (rendererDirty)
    {
      layer->setRenderer(bufferRenderer);
      layer->setRendererDialog(bufferDialog);
    }
  
  labelDialog->apply();
  layer->setLabelOn(labelCheckBox->isChecked());

  QgsSiSyDialog *sdialog = dynamic_cast < QgsSiSyDialog * >(layer->rendererDialog());
  QgsGraSyDialog *gdialog = dynamic_cast < QgsGraSyDialog * >(layer->rendererDialog());
  QgsContColDialog *cdialog = dynamic_cast < QgsContColDialog * >(layer->rendererDialog());
  QgsSiMaDialog* smdialog = dynamic_cast < QgsSiMaDialog * >(layer->rendererDialog());
  QgsGraMaDialog* gmdialog = dynamic_cast< QgsGraMaDialog * >(layer->rendererDialog());
  QgsUValDialog* udialog = dynamic_cast< QgsUValDialog * > (layer->rendererDialog());
  QgsUValMaDialog* umdialog = dynamic_cast< QgsUValMaDialog * > (layer->rendererDialog());

  if (sdialog)
    {
      sdialog->apply();
  } else if (gdialog)
    {
      gdialog->apply();
  } else if (cdialog)
    {
      cdialog->apply();
    }
  else if(smdialog)
  {
      smdialog->apply();
  }
  else if(gmdialog)
  {
      gmdialog->apply();
  }
  else if(udialog)
  {
      udialog->apply();
  }
  else if(umdialog)
  {
      umdialog->apply();
  }

  rendererDirty = false;
}

void QgsDlgVectorLayerProperties::closeEvent(QCloseEvent* e)
{
    close();
}

void QgsDlgVectorLayerProperties::close()
{
  if (rendererDirty)
    {
      widgetStackRenderers->removeWidget(bufferDialog);
      delete bufferDialog;
      delete bufferRenderer;
      bufferDialog = layer->rendererDialog();
      bufferRenderer = layer->renderer();
      widgetStackRenderers->addWidget(bufferDialog);
      widgetStackRenderers->raiseWidget(bufferDialog);
      rendererDirty = false;
      //restore the right name in the combobox
      if(bufferRenderer)
      {
	  legendtypecombobox->setCurrentText(tr(bufferRenderer->name()));
      }
    }
  reject();
}

QDialog *QgsDlgVectorLayerProperties::getBufferDialog()
{
  return bufferDialog;
}

QgsRenderer *QgsDlgVectorLayerProperties::getBufferRenderer()
{
  return bufferRenderer;
}

void QgsDlgVectorLayerProperties::setLegendType(QString type)
{
  legendtypecombobox->setCurrentText(tr(type));
}

void QgsDlgVectorLayerProperties::reset( void )
{
    labelDialog->init();
    labelCheckBox->setChecked(layer->labelOn());
}
