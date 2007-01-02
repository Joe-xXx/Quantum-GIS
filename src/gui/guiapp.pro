#################################################################
#
#         QMAKE Project File for Quantum GIS 
# 
#                   Tim Sutton 2006
#
# NOTE: Do not place any hard coded external paths in this file
#       all libs and includes should be specified in settings.pro
#       in the top level qgis directory.
# 
#################################################################

#
# This file builds the gui library - the app is built in a separate pro file
#

include(../../settings.pro)
include(../ui/ui.pro)
TEMPLATE=app
TARGET=qgis
#suffix debug to target if applicable
CONFIG(debug, debug|release){
  TARGET = $$member(TARGET, 0)-debug
}
LIBS += $${GDALLIBADD}
#LIBS += $${SQLITELIBADD}
#LIBS += $${GEOSLIBADD}
#LIBS += $${PROJLIBADD}
LIBS += $${QGISCORELIBADD}
LIBS += $${QGISGUILIBADD}
#LIBS += $${QGISPROJECTIONSELECTORLIBADD}
DESTDIR=$${QGISBINDIR}
QT += qt3support svg core gui xml network
#
# Copy developers list and pics to the share dir
#
win32:system(copy "c:\dev\cpp\qgis\AUTHORS" "c:\dev\cpp\qgis\qgis-release\share\qgis\doc\") 
win32:system(copy "c:\dev\cpp\qgis\SPONSORS" "c:\dev\cpp\qgis\qgis-release\share\qgis\doc\") 
win32:system(copy "c:\dev\cpp\qgis\images\developers\*.jpg" "c:\dev\cpp\qgis\qgis-release\share\qgis\images\developers\")
win32:system(copy "c:\dev\cpp\qgis\doc\index.html" "c:\dev\cpp\qgis\qgis-release\share\qgis\doc\")
win32:system(copy "c:\dev\cpp\qgis\doc\userguide.pdf" "c:\dev\cpp\qgis\qgis-release\share\qgis\doc\")
win32:system(copy "c:\dev\cpp\qgis\resources\*.db" "c:\dev\cpp\qgis\qgis-release\share\qgis\resources\")
#this way didnt work but would be nicer than above
#win32:system(copy "c:\dev\cpp\qgis\AUTHORS" $${QGISDOCDIR}
#win32:system(copy ../../SPONSORS $${QGISDOCDIR})
#win32:system(copy ../../images/developers* $${QGISDEVELOPERSIMAGEDIR})
message("Building libs into $${DESTDIR}")

win32: RC_FILE = ../../win_build/qgis.rc

SOURCES = main.cpp					\
