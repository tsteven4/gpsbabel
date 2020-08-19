#include <cstdio>                     // for stdout
#include <QtCore/QCoreApplication>    // for QCoreApplication
#include <QtCore/QDebug>              // for QDebug
#include <QtCore/QFile>               // for QFile
#include <QtCore/QFileInfo>           // for QFileInfo
#include <QtCore/QIODevice>           // for QIODevice, QIODevice::ReadOnly, QIODevice::WriteOnly
#include <QtCore/QRegularExpression>  // for QRegularExpression
#include <QtCore/QString>             // for QString
#include <QtCore/QStringList>         // for QStringList
#include <QtCore/QTemporaryFile>
#include <QtCore/QTextStream>         // for QTextStream
#include <QtCore/QVector>             // for QVector
#include <QtCore/QtGlobal>


struct substitution {
  QString pattern;
  QString replacement;
};

int main(int argc, char* argv[])
{
  const QVector<substitution> substitutions = {
    { R"(\\)", R"(\\)" },
    { R"(")", R"(\")" },
    { "^(.)", R"("\1)" },
    { "(.)$", R"(\1\n")" },
    { "^(.)", R"(  \1)" }
  };
    

  const char* prog_name = argv[0]; /* may not match QCoreApplication::arguments().at(0)! */

  QCoreApplication app(argc, argv);
  QStringList qargs = QCoreApplication::arguments();

  if ((qargs.size() < 4) || (qargs.at(1) != "-o")) {
    qCritical().noquote() << "Usage:" << prog_name << "-o outputfile inputfile [inputfile2 [...]]";
    return 1;
  }

  QString fileout = qargs.at(2);
  if (QFile::exists(fileout)) {
    if (!QFile::remove(fileout)) {
      qCritical() << "Cannot remove existing file" << fileout;
      return 1;
    }
  }

  QTemporaryFile tout;
  if (!tout.open()) {
    qCritical() << "Could not open tempoarary file for output.";
    return 1;
  }
  QTextStream sout(&tout);

  sout << "/* This file is machine-generated from the contents of style/ */\n";
  sout << "/* by mkstyle.sh.   Editing it by hand is an exceedingly bad idea. */\n";
  sout << "\n";
  sout << "#include <QtCore/QVector>\n";
  sout << "#include \"defs.h\"\n";
  sout << "#if CSVFMTS_ENABLED\n";

  const QStringList stylefiles = qargs.mid(3);
  QStringList stylelist;
  for (const auto& filename : stylefiles) {
    QFile fin(filename);
    if (!fin.open(QIODevice::ReadOnly)) {
      qCritical() << "Could not open" << filename << "for input.";
      return 1;
    }
    QTextStream sin(&fin);

    QFileInfo fi(fin);
    QString basename = fi.baseName();
    stylelist << basename;
    sout << "static char " << basename << "[] =\n";
    QString line;
    while (sin.readLineInto(&line)) {
      for (const auto& sub : substitutions) {
        QRegularExpression re(sub.pattern);
        line.replace(re, sub.replacement);
      }
      sout << line << '\n';
    }
    sout << "  ;\n";
    fin.close();
  }

  sout << "const QVector<style_vecs_t> style_list = {";
  for (int i = stylelist.size() - 1; i >= 0; --i) {
    sout << "{ \"" << stylelist.at(i) << "\", " << stylelist.at(i) << " }";
    if (i > 0) {
      sout << ", ";
    }
  }
  sout << "};\n";
  sout << "#else /* CSVFMTS_ENABLED */\n";
  sout << "const QVector<style_vecs_t> style_list;\n";
  sout << "#endif /* CSVFMTS_ENABLED */\n";

  sout.flush();
  tout.close();

  // fiddle around such that only good files are created.
  if (!tout.copy(fileout)) {
    qCritical() << "Could not open" << fileout << "for output.";
    return 1;
  }
  
}
