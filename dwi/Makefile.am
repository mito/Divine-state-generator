include $(top_srcdir)/Makefile.am.global

bin_PROGRAMS = $(top_srcdir)/bin/$(BINPREFIX)dwi-run
bin_SCRIPTS = $(top_srcdir)/bin/$(BINPREFIX)dwi-prepare \
	$(top_srcdir)/bin/$(BINPREFIX)dwi-cluster-load \
	$(top_srcdir)/bin/$(BINPREFIX)dwi-syntax \
	$(top_srcdir)/bin/$(BINPREFIX)gui-client \
	$(top_srcdir)/bin/$(BINPREFIX)gui
pkgdata_DATA  = truststore
dist_pkgjar_DATA   = dwi.jar	
dist_noinst_DATA = client-start.bat client-start.sh keys/truststore server-start.sh
dist_pkgstore_DATA = store/algorithms store/clusters store/passwd
dist_pkgimages_DATA = images/clock.gif images/dve.gif images/logo.png \
		      images/ltl.gif images/noprop.gif images/pml.gif \
		      images/redcross.gif images/star.gif images/stop.png \
		      images/tick.gif images/ce_back.png images/ce_forw.png \
		      images/ce_begin.png images/ce_cycle.png \
		      images/ce_end.png images/ce_watches.png

CLEANFILES=$(bin_SCRIPTS) $(dist_pkgjar_DATA) $(pkgdata_DATA) #no "+=", but it is understood this way

__top_srcdir__bin___BINPREFIX_dwi_run_SOURCES = dwi-run.cpp

AM_JAVACFLAGS = -cp $(top_srcdir)/dwi/src/:src/junit.jar
JAVAROOT = build/

srcprefix=src/divine
common=$(srcprefix)/common
server=$(srcprefix)/server
client=$(srcprefix)/client
tests=$(srcprefix)/tests

if BUILDSUPPORT

java_JAVA = \
$(common)/Node.java \
$(common)/MapNode.java \
$(common)/Wired.java \
$(common)/Request.java \
$(common)/Protocol.java \
$(common)/Profile.java \
$(common)/User.java \
$(common)/Computer.java \
$(common)/Property.java \
$(common)/Global.java \
$(common)/Model.java \
$(common)/Task.java \
$(common)/Job.java \
$(common)/Profile.java \
$(common)/Algorithm.java \
$(common)/Cluster.java \
$(common)/Logger.java \
$(common)/Worker.java \
\
$(server)/DwiSocket.java \
$(server)/DwiServer.java \
\
$(client)/SpringUtilities.java \
$(client)/Client.java \
$(client)/MainWindow.java \
$(client)/CellRenderer.java \
$(client)/LoginDialog.java \
$(client)/VTextIcon.java \
$(client)/ProfileEditor.java \
$(client)/ModelEditor.java \
$(client)/PropertyEditor.java \
$(client)/DvePropertyEditor.java \
$(client)/TaskEditor.java \
$(client)/PlainTextViewer.java \
$(client)/JobReportViewer.java \
$(client)/JobOutputViewer.java \
$(client)/JobLogViewer.java \
$(client)/JobEditor.java \
$(client)/others/ExtensionFilter.java \
$(client)/JobCeViewer.java \
$(client)/ClusterStatusViewer.java

endif

check_SCRIPTS = testsuite-start.sh
TESTS = testsuite-start.sh

$(top_srcdir)/bin/$(BINPREFIX)dwi-%: dwi-%
	$(COMMON_SUBSTITUTIONS) < $(<) > $(@)
	chmod '+x' $(@)

if BUILDSUPPORT

dwi.jar: classjava.stamp
	cd $(JAVAROOT) && jar cf dwi.jar divine
	mv $(JAVAROOT)/dwi.jar ./

endif

$(top_srcdir)/bin/$(BINPREFIX)gui-client: dwi.jar client-start.sh truststore
	$(COMMON_SUBSTITUTIONS) < client-start.sh > $(@)
	chmod '+x' $(@)

$(top_srcdir)/bin/$(BINPREFIX)gui: $(top_srcdir)/bin/$(BINPREFIX)gui-client
	echo '#!/bin/sh' > $(@)
	echo '$(BINPREFIX)gui-client --local $$@' >> $(@)
	chmod '+x' $(@)

truststore: keys/truststore
	cp keys/truststore ./truststore


EXTRA_DIST = dwi-syntax dwi-prepare dwi-cluster-load $(java_JAVA) src/junit.jar build/dummy
