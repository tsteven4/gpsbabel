TEMPLATE = subdirs

#SUBDIRS += acconfigfiles
SUBDIRS += mkstyle

cli.file = GPSBabel.pro
cli.depends += mkstyle
SUBDIRS += cli
