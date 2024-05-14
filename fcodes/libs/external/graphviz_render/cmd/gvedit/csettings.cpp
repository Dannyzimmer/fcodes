/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/
#ifdef _WIN32
#include "windows.h"
#endif
#include <cassert>
#include "csettings.h"
#include <cstdint>
#include "qmessagebox.h"
#include "qfiledialog.h"
#include <QtWidgets>
#include <qfile.h>
#include "mdichild.h"
#include "string.h"
#include "mainwindow.h"
#include <QTemporaryFile>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#ifdef __FreeBSD__
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

#if !defined(_WIN32)
#include <unistd.h>
#endif

extern int errorPipe(char *errMsg);

#define WIDGET(t,f)  ((t*)findChild<t *>(#f))
typedef struct {
    const char *data;
    int len;
    int cur;
} rdr_t;

#if !defined(__APPLE__) && !defined(_WIN32)
/// read the given symlink in the /proc file system
static std::string read_proc(const std::string &path) {

  std::vector<char> buf(PATH_MAX + 1, '\0');
  if (readlink(path.c_str(), buf.data(), buf.size()) < 0)
    return "";

  // was the path too long?
  if (buf.back() != '\0')
    return "";

  return buf.data();
}
#endif

/// find an absolute path to the current executable
static std::string find_me(void) {

  // macOS
#ifdef __APPLE__
  {
    // determine how many bytes we will need to allocate
    uint32_t buf_size = 0;
    int rc = _NSGetExecutablePath(NULL, &buf_size);
    assert(rc != 0);
    assert(buf_size > 0);

    std::vector<char> path(buf_size);

    // retrieve the actual path
    if (_NSGetExecutablePath(path.data(), &buf_size) < 0) {
      errout << "failed to get path for executable.\n";
      return "";
    }

    // try to resolve any levels of symlinks if possible
    while (true) {
      std::vector<char> buf(PATH_MAX + 1, '\0');
      if (readlink(path.data(), buf.data(), buf.size()) < 0)
        return path.data();

      path = buf;
    }
  }
#elif defined(_WIN32)
  {
    std::vector<char> path;
    DWORD rc = 0;

    do {
      {
        size_t size = path.empty() ? 1024 : (path.size() * 2);
        path.resize(size);
      }

      rc = GetModuleFileNameA(NULL, path.data(), path.size());
      if (rc == 0) {
        errout << "failed to get path for executable.\n";
        return "";
      }

    } while (rc == path.size());

    return path.data();
  }
#else

  // Linux
  std::string path = read_proc("/proc/self/exe");
  if (path != "")
    return path;

  // DragonFly BSD, FreeBSD
  path = read_proc("/proc/curproc/file");
  if (path != "")
    return path;

  // NetBSD
  path = read_proc("/proc/curproc/exe");
  if (path != "")
    return path;

// /proc-less FreeBSD
#ifdef __FreeBSD__
  {
    int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    static const size_t MIB_LENGTH = sizeof(mib) / sizeof(mib[0]);
    std::vector<char> buf(PATH_MAX + 1, '\0');
    size_t buf_size = buf.size();
    if (sysctl(mib, MIB_LENGTH, buf.data(), &buf_size, NULL, 0) == 0)
      return buf.data();
  }
#endif
#endif

  errout << "failed to get path for executable.\n";
  return "";
}

/// find an absolute path to where Smyrna auxiliary files are stored
static std::string find_share(void) {

#ifdef _WIN32
  const char PATH_SEPARATOR = '\\';
#else
  const char PATH_SEPARATOR = '/';
#endif

  // find the path to the `gvedit` binary
  std::string gvedit_exe = find_me();
  if (gvedit_exe == "")
    return "";

  // assume it is of the form …/bin/gvedit[.exe] and construct
  // …/share/graphviz/gvedit

  size_t slash = gvedit_exe.rfind(PATH_SEPARATOR);
  if (slash == std::string::npos) {
    errout << "no path separator in path to self, " << gvedit_exe.c_str()
           << '\n';
    return "";
  }

  std::string bin = gvedit_exe.substr(0, slash);
  slash = bin.rfind(PATH_SEPARATOR);
  if (slash == std::string::npos) {
    errout << "no path separator in directory containing self, "
           << bin.c_str() << '\n';
    return "";
  }

  std::string install_prefix = bin.substr(0, slash);

  return install_prefix + PATH_SEPARATOR + "share" + PATH_SEPARATOR + "graphviz"
    + PATH_SEPARATOR + "gvedit";
}
 
bool loadAttrs(const QString fileName, QComboBox * cbNameG,
	       QComboBox * cbNameN, QComboBox * cbNameE)
{
    QStringList lines;
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
	QTextStream stream(&file);
	QString line;
	while (!stream.atEnd()) {
	    line = stream.readLine();	// line of text excluding '\n'
	    if (line.left(1) == ":") {
		QString attrName;
		QStringList sl = line.split(":");
		for (int id = 0; id < sl.count(); id++) {
		    if (id == 1)
			attrName = sl[id];
		    if (id == 2) {
			if (sl[id].contains("G"))
			    cbNameG->addItem(attrName);
			if (sl[id].contains("N"))
			    cbNameN->addItem(attrName);
			if (sl[id].contains("E"))
			    cbNameE->addItem(attrName);
		    }
		};
	    }
	}
	file.close();
    } else {
	errout << "Could not open attribute name file \"" << fileName <<
	    "\" for reading\n";
	errout.flush();
	return true;
    }

    return false;
}

QString stripFileExtension(QString fileName)
{
    int idx;
    for (idx = fileName.length(); idx >= 0; idx--) {
	if (fileName.mid(idx, 1) == ".")
	    break;
    }
    return fileName.left(idx);
}

CFrmSettings::CFrmSettings()
{
    this->gvc = gvContext();
    Ui_Dialog tempDia;
    tempDia.setupUi(this);
    graph = nullptr;
    activeWindow = nullptr;
    QString path;
    char *s = NULL;
#ifndef _WIN32
    s = getenv("GVEDIT_PATH");
#endif
    if (s)
	path = s;
    else
	path = QString::fromStdString(find_share());

    connect(WIDGET(QPushButton, pbAdd), SIGNAL(clicked()), this,
	    SLOT(addSlot()));
    connect(WIDGET(QPushButton, pbNew), SIGNAL(clicked()), this,
	    SLOT(newSlot()));
    connect(WIDGET(QPushButton, pbOpen), SIGNAL(clicked()), this,
	    SLOT(openSlot()));
    connect(WIDGET(QPushButton, pbSave), SIGNAL(clicked()), this,
	    SLOT(saveSlot()));
    connect(WIDGET(QPushButton, btnOK), SIGNAL(clicked()), this,
	    SLOT(okSlot()));
    connect(WIDGET(QPushButton, btnCancel), SIGNAL(clicked()), this,
	    SLOT(cancelSlot()));
    connect(WIDGET(QPushButton, pbOut), SIGNAL(clicked()), this,
	    SLOT(outputSlot()));
    connect(WIDGET(QPushButton, pbHelp), SIGNAL(clicked()), this,
	    SLOT(helpSlot()));

    connect(WIDGET(QComboBox, cbScope), SIGNAL(currentIndexChanged(int)),
	    this, SLOT(scopeChangedSlot(int)));
    scopeChangedSlot(0);

    if (path != "") {
	loadAttrs(path + "/attrs.txt", WIDGET(QComboBox, cbNameG),
	          WIDGET(QComboBox, cbNameN), WIDGET(QComboBox, cbNameE));
    }
    setWindowIcon(QIcon(":/images/icon.png"));
}

void CFrmSettings::outputSlot()
{
    QString _filter =
	"Output File(*." + WIDGET(QComboBox,
				  cbExtension)->currentText() + ")";
    QString fileName =
	QFileDialog::getSaveFileName(this, tr("Save Graph As.."), "/",
				     _filter);
    if (!fileName.isEmpty())
	WIDGET(QLineEdit, leOutput)->setText(fileName);
}

void CFrmSettings::scopeChangedSlot(int id)
{
    WIDGET(QComboBox, cbNameG)->setVisible(id == 0);
    WIDGET(QComboBox, cbNameN)->setVisible(id == 1);
    WIDGET(QComboBox, cbNameE)->setVisible(id == 2);
}

void CFrmSettings::addSlot()
{
    QString _scope = WIDGET(QComboBox, cbScope)->currentText();
    QString _name;
    switch (WIDGET(QComboBox, cbScope)->currentIndex()) {
    case 0:
	_name = WIDGET(QComboBox, cbNameG)->currentText();
	break;
    case 1:
	_name = WIDGET(QComboBox, cbNameN)->currentText();
	break;
    case 2:
	_name = WIDGET(QComboBox, cbNameE)->currentText();
	break;
    }
    QString _value = WIDGET(QLineEdit, leValue)->text();

    if (_value.trimmed().isEmpty())
	QMessageBox::warning(this, tr("GvEdit"),
			     tr
			     ("Please enter a value for selected attribute!"),
			     QMessageBox::Ok, QMessageBox::Ok);
    else {
	QString str = _scope + "[" + _name + "=\"";
	if (WIDGET(QTextEdit, teAttributes)->toPlainText().contains(str)) {
	    QMessageBox::warning(this, tr("GvEdit"),
				 tr("Attribute is already defined!"),
				 QMessageBox::Ok, QMessageBox::Ok);
	    return;
	} else {
	    str = str + _value + "\"]";
	    WIDGET(QTextEdit,
		   teAttributes)->setPlainText(WIDGET(QTextEdit,
						      teAttributes)->
					       toPlainText() + str + "\n");

	}
    }
}

void CFrmSettings::helpSlot()
{
    QDesktopServices::
	openUrl(QUrl("http://www.graphviz.org/doc/info/attrs.html"));
}

void CFrmSettings::cancelSlot()
{
    this->reject();
}

void CFrmSettings::okSlot()
{
    saveContent();
    this->done(drawGraph());
}

void CFrmSettings::newSlot()
{
    WIDGET(QTextEdit, teAttributes)->setPlainText(tr(""));
}

void CFrmSettings::openSlot()
{
    QString fileName =
	QFileDialog::getOpenFileName(this, tr("Open File"), "/",
				     tr("Text file (*.*)"));
    if (!fileName.isEmpty()) {
	QFile file(fileName);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
	    QMessageBox::warning(this, tr("MDI"),
				 tr("Cannot read file %1:\n%2.")
				 .arg(fileName)
				 .arg(file.errorString()));
	    return;
	}

	QTextStream in(&file);
	WIDGET(QTextEdit, teAttributes)->setPlainText(in.readAll());
    }

}

void CFrmSettings::saveSlot()
{

    if (WIDGET(QTextEdit, teAttributes)->toPlainText().trimmed().isEmpty()) {
	QMessageBox::warning(this, tr("GvEdit"), tr("Nothing to save!"),
			     QMessageBox::Ok, QMessageBox::Ok);
	return;


    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Open File"),
						    "/",
						    tr("Text File(*.*)"));
    if (!fileName.isEmpty()) {

	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Text)) {
	    QMessageBox::warning(this, tr("MDI"),
				 tr("Cannot write file %1:\n%2.")
				 .arg(fileName)
				 .arg(file.errorString()));
	    return;
	}

	QTextStream out(&file);
	out << WIDGET(QTextEdit, teAttributes)->toPlainText();
	return;
    }

}

bool CFrmSettings::loadGraph(MdiChild * m)
{
    if (graph) {
	agclose(graph);
	graph = nullptr;
    }
    graphData.clear();
    graphData.append(m->toPlainText());
    setActiveWindow(m);
    return true;

}

bool CFrmSettings::createLayout()
{
    rdr_t rdr;
    //first attach attributes to graph
    int _pos = graphData.indexOf(tr("{"));
    graphData.replace(_pos, 1,
		      "{" + WIDGET(QTextEdit,
				   teAttributes)->toPlainText());

    /* Reset line number and file name;
     * If known, might want to use real name
     */
    agsetfile("<gvedit>");
    QByteArray bytes = graphData.toUtf8();
    rdr.data = bytes.constData();
    rdr.len = strlen(rdr.data);
    rdr.cur = 0;
    graph = agmemread(rdr.data);
    if (!graph)
	return false;
    if (agerrors()) {
	agclose(graph);
	graph = nullptr;
	return false;
    }
    Agraph_t *G = this->graph;
    QString layout;

    layout=WIDGET(QComboBox, cbLayout)->currentText();


    gvLayout(gvc, G, layout.toUtf8().constData());	/* library function */
    return true;
}

static QString buildTempFile()
{
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    tempFile.open();
    QString a = tempFile.fileName();
    tempFile.close();
    return a;
}

void CFrmSettings::doPreview(QString fileName)
{
    if (getActiveWindow()->previewFrm != nullptr) {
	getActiveWindow()->parentFrm->mdiArea->
	    removeSubWindow(getActiveWindow()->previewFrm->subWindowRef);
	getActiveWindow()->previewFrm = nullptr;
    }

    if ((fileName.isNull()) || !(getActiveWindow()->loadPreview(fileName))) {	//create preview
	QString prevFile(buildTempFile());
	gvRenderFilename(gvc, graph, "png", prevFile.toUtf8().constData());
	getActiveWindow()->loadPreview(prevFile);
    }
}

bool CFrmSettings::renderLayout()
{
    if (!graph)
	return false;
    QString sfx = WIDGET(QComboBox, cbExtension)->currentText();
    QString fileName(WIDGET(QLineEdit, leOutput)->text());

    if (fileName == "" || sfx == "NONE")
	doPreview(QString());
    else {
	fileName = stripFileExtension(fileName);
	fileName = fileName + "." + sfx;
	if (fileName != activeWindow->outputFile)
	    activeWindow->outputFile = fileName;

#ifdef _WIN32
	if (!fileName.contains('/') && !fileName.contains('\\'))
#else
	if (!fileName.contains('/'))
#endif
	{  // no directory info => can we create/write the file?
	    QFile outf(fileName);
	    if (outf.open(QIODevice::WriteOnly))
		outf.close();
	    else {
		QString pathName = QDir::homePath();
		pathName.append("/").append(fileName);
		fileName = QDir::toNativeSeparators (pathName);
		QString msg ("Output written to ");
		msg.append(fileName);
		msg.append("\n");
		errorPipe(msg.toLatin1().data());
	    }
	}

	if (gvRenderFilename(gvc, graph, sfx.toUtf8().constData(),
	                     fileName.toUtf8().constData()))
	    return false;

	doPreview(fileName);
    }
    return true;
}



bool CFrmSettings::loadLayouts()
{
    return false;
}

bool CFrmSettings::loadRenderers()
{
    return false;
}

void CFrmSettings::refreshContent()
{

    WIDGET(QComboBox, cbLayout)->setCurrentIndex(activeWindow->layoutIdx);
    WIDGET(QComboBox,
	   cbExtension)->setCurrentIndex(activeWindow->renderIdx);
    if (!activeWindow->outputFile.isEmpty())
	WIDGET(QLineEdit, leOutput)->setText(activeWindow->outputFile);
    else
	WIDGET(QLineEdit,
	       leOutput)->setText(stripFileExtension(activeWindow->
						     currentFile()) + "." +
				  WIDGET(QComboBox,
					 cbExtension)->currentText());

    WIDGET(QTextEdit, teAttributes)->setText(activeWindow->attributes);

    WIDGET(QLineEdit, leValue)->setText("");

}

void CFrmSettings::saveContent()
{
    activeWindow->layoutIdx = WIDGET(QComboBox, cbLayout)->currentIndex();
    activeWindow->renderIdx =
	WIDGET(QComboBox, cbExtension)->currentIndex();
    activeWindow->outputFile = WIDGET(QLineEdit, leOutput)->text();
    activeWindow->attributes =
	WIDGET(QTextEdit, teAttributes)->toPlainText();
}

int CFrmSettings::drawGraph()
{
    int rc;
    if (createLayout() && renderLayout()) {
	getActiveWindow()->settingsSet = false;
	rc = QDialog::Accepted;
    } else
	rc = QDialog::Accepted;
    agreseterrors();

    return rc;
}

int CFrmSettings::runSettings(MdiChild * m)
{
	if (this->loadGraph(m))
	    return drawGraph();


    if (m && m == getActiveWindow()) {
	if (this->loadGraph(m))
	    return drawGraph();
	else
	    return QDialog::Rejected;
    }

    else
	return showSettings(m);

}

int CFrmSettings::showSettings(MdiChild * m)
{

    if (this->loadGraph(m)) {
	refreshContent();
	return this->exec();
    } else
	return QDialog::Rejected;
}

void CFrmSettings::setActiveWindow(MdiChild * m)
{
    this->activeWindow = m;

}

MdiChild *CFrmSettings::getActiveWindow()
{
    return activeWindow;
}
