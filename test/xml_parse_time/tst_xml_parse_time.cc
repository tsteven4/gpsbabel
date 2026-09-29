#include <QDateTime>
#include <QTest>
#include "test_setup.h"

#include "defs.h"
#include "src/core/datetime.h"


class TestXmlParseTime: public QObject
{
  Q_OBJECT
private slots:
  void initTestCase()
  {
    test_setup();
  }
  void xml_parse_time_data();
  void xml_parse_time();
};

void TestXmlParseTime::xml_parse_time_data()
{
  QTest::addColumn<QString>("string");
  QTest::addColumn<gpsbabel::DateTime>("result");

  QTest::newRow("dateTime") << "2010-10-16T14:27:17" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(14, 27, 17), QtUTC));
  QTest::newRow("dateTime-Msec") << "2010-10-16T14:27:17.123" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(14, 27, 17, 123), QtUTC));
  QTest::newRow("dateTime-Zulu") << "2010-10-16T14:27:17Z" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(14, 27, 17), QtUTC));
  QTest::newRow("dateTime-MsecZulu") << "2010-10-16T14:27:17.123Z" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(14, 27, 17, 123), QtUTC));
  QTest::newRow("dateTime-PosOffset") << "2010-10-16T14:27:17+01:30" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(12, 57, 17), QtUTC));
  QTest::newRow("dateTime-NegOffset") << "2010-10-16T14:27:17-01:30" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(15, 57, 17), QtUTC));

  QTest::newRow("date") << "2010-10-16" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(0, 0, 0), QtUTC));
  QTest::newRow("date-Zulu") << "2010-10-16Z" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(0, 0, 0), QtUTC));
  QTest::newRow("date-PosOffset") << "2010-10-16+01:30" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 15), QTime(22, 30, 0), QtUTC));
  QTest::newRow("date-NegOffset") << "2010-10-16-01:30" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(1, 30, 0), QtUTC));

  QTest::newRow("gYearMonth") << "2010-10" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 1), QTime(0, 0, 0), QtUTC));
  QTest::newRow("gYearMonth-Zulu") << "2010-10Z" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 1), QTime(0, 0, 0), QtUTC));
  QTest::newRow("gYearMonth-PosOffset") << "2010-10+02:30" << gpsbabel::DateTime(QDateTime(QDate(2010, 9, 30), QTime(21, 30, 0), QtUTC));
  QTest::newRow("gYearMonth-NegOffset") << "2010-10-02:30" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 1), QTime(2, 30, 0), QtUTC));

  QTest::newRow("gYear") << "2010" << gpsbabel::DateTime(QDateTime(QDate(2010, 1, 1), QTime(0, 0, 0), QtUTC));
  QTest::newRow("gYear-Zulu") << "2010Z" << gpsbabel::DateTime(QDateTime(QDate(2010, 1, 1), QTime(0, 0, 0), QtUTC));
  QTest::newRow("gYear-PosOffset") << "2010+01:30" << gpsbabel::DateTime(QDateTime(QDate(2009, 12, 31), QTime(22, 30, 0), QtUTC));
  QTest::newRow("gYear-NegOffset") << "2010-01:30" << gpsbabel::DateTime(QDateTime(QDate(2010, 1, 1), QTime(1, 30, 0), QtUTC));

  // not legal, but assume missing low order time components are zero
  QTest::newRow("dateTime-noseconds") << "2010-10-16T14:27" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(14, 27, 0), QtUTC));
  QTest::newRow("dateTime-noseconds-Zulu") << "2010-10-16T14:27Z" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(14, 27, 0), QtUTC));
  QTest::newRow("dateTime-noseconds-PosOffset") << "2010-10-16T14:27+01:30" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(12, 57, 0), QtUTC));
  QTest::newRow("dateTime-noseconds-NegOffset") << "2010-10-16T14:27-01:30" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(15, 57, 0), QtUTC));
  QTest::newRow("dateTime-nominutes") << "2010-10-16T14" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(14, 0, 0), QtUTC));
  QTest::newRow("dateTime-nominutes-Zulu") << "2010-10-16T14Z" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(14, 0, 0), QtUTC));
  QTest::newRow("dateTime-nominutes-PosOffset") << "2010-10-16T14+01:30" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(12, 30, 0), QtUTC));
  QTest::newRow("dateTime-nominutes-NegOffset") << "2010-10-16T14-01:30" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(15, 30, 0), QtUTC));
  QTest::newRow("dateTime-nohours") << "2010-10-16T" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(0, 0, 0), QtUTC));
  QTest::newRow("dateTime-nohours-Zulu") << "2010-10-16TZ" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(0, 0, 0), QtUTC));
  QTest::newRow("dateTime-nohours-PosOffset") << "2010-10-16T+01:30" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 15), QTime(22, 30, 0), QtUTC));
  QTest::newRow("dateTime-nohours-NegOffset") << "2010-10-16T-01:30" << gpsbabel::DateTime(QDateTime(QDate(2010, 10, 16), QTime(1, 30, 0), QtUTC));

  // bad input
  QTest::newRow("missingdatepiece") << "2010-16T14:27:17Z" << gpsbabel::DateTime();
  QTest::newRow("missingtimepiece") << "2010-10-16T14:17.123+01:30" << gpsbabel::DateTime();
  QTest::newRow("missingzonepiece") << "2010-10-16T14:27:17+01" << gpsbabel::DateTime();
  QTest::newRow("shortyear") << "10-10-16T14:27:17.123" << gpsbabel::DateTime();
  QTest::newRow("nowholesecsonds") << "2010-10-16T14:27:.123" << gpsbabel::DateTime();
  QTest::newRow("nofracsecsonds") << "2010-10-16T14:27:17." << gpsbabel::DateTime();
  QTest::newRow("nodatetimeOffset") << "+01:30" << gpsbabel::DateTime();
  QTest::newRow("nodatetimeZulu") << "Z" << gpsbabel::DateTime();
  QTest::newRow("empty") << "" << gpsbabel::DateTime();
}

void TestXmlParseTime::xml_parse_time()
{
  QFETCH(QString, string);
  QFETCH(gpsbabel::DateTime, result);
  QCOMPARE(::xml_parse_time(string), result);
  gpsbabel::DateTime dt;
#ifdef ENABLE_BENCHMARK
  QBENCHMARK{ dt = ::xml_parse_time(string); }
#endif
}

QTEST_MAIN(TestXmlParseTime);
#include "tst_xml_parse_time.moc"
