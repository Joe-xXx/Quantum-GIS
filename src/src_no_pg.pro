######################################################################
# Automatically generated by qmake (1.02a) Thu Jul 4 08:58:21 2002
######################################################################
TARGET = qgis
TEMPLATE = app
LIBS += -lgdal

CONFIG += qt thread debug

# Input
HEADERS += qgisapp.h \
           qgisappbase.ui.h \
           qgsdatasource.h \
           qgsmapcanvas.h \
           qgsmaplayer.h \
           qgsrasterlayer.h \
           qgsshapefilelayer.h \
           qgstable.h \
           qgsrect.h \
           qgspoint.h \
           qgscoordinatetransform.h \
           qgssymbol.h \
           qgsmarkersymbol.h \
           qgslinesymbol.h \
           qgspolygonsymbol.h \
           qgslegend.h  \
           qgslegenditem.h \
           qgslayerproperties.h \
           qgsidentifyresults.h \
	   qgsattributetable.h \
	   qgsattributetabledisplay.h
           
INTERFACES += 				 qgisappbase.ui \
					 qgslegenditembase.ui \
					 qgsabout.ui \
					 qgslayerpropertiesbase.ui \
					 qgsidentifyresultsbase.ui \
					 qgsattributetablebase.ui
SOURCES += main.cpp \
           qgisapp.cpp \
           qgsdatasource.cpp \
           qgsmapcanvas.cpp \
           qgsmaplayer.cpp \
           qgsrasterlayer.cpp \
	    qgsshapefilelayer.cpp \
           qgsrect.cpp \
           qgspoint.cpp \
           qgscoordinatetransform.cpp \
           qgslegend.cpp \
           qgslegenditem.cpp \
           qgssymbol.cpp \
           qgsmarkersymbol.cpp \
           qgslinesymbol.cpp \
           qgspolygonsymbol.cpp \
           qgslayerproperties.cpp \
	   qgsidentifyresults.cpp \
	   qgsattributetable.cpp \
	   qgsattributetabledisplay.cpp
