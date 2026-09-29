#include <QTest>
#include "test_setup.h"

#include "defs.h"
#include "parse.h"


class TestParse: public QObject
{
  Q_OBJECT
private slots:
  void initTestCase()
  {
    test_setup();
  }
  void parse_double_data();
  void parse_double();
  void parse_int_data();
  void parse_int();
  void parse_distance_data();
  void parse_distance();
  void parse_speed_data();
  void parse_speed();
  void parse_coordinates_data();
  void parse_coordinates();
};

void TestParse::parse_double_data()
{
  QTest::addColumn<QString>("string");
  QTest::addColumn<double>("result");
  QTest::addColumn<bool>("ok");
  QTest::addColumn<QString>("end");

  QTest::newRow("double") << "3.14159" << 3.14159 << true << QString();
  QTest::newRow("trailing") << "3.14trail" << 3.14 << true << "trail";
  QTest::newRow("badconversion") << "x" << 0.0 << false << QString();
  QTest::newRow("outofrange") << "1.79769e+309" << 0.0 << false << QString();
}

void TestParse::parse_double()
{
  struct dut_t{
    bool ok;
    double result;
    QString end;
  };
  dut_t dut;

  QFETCH(QString, string);
  QFETCH(double, result);
  QFETCH(bool, ok);
  QFETCH(QString, end);

  // double parse_double(const QString& str, const QString& id, bool* ok = nullptr, QString* end = nullptr);
  dut.result = ::parse_double(string, "unit", &dut.ok, &dut.end);
  QCOMPARE(dut.result, result);
  QCOMPARE(dut.ok, ok);
  QCOMPARE(dut.end, end);
#ifdef ENABLE_BENCHMARK
#endif
}

void TestParse::parse_int_data()
{
  QTest::addColumn<QString>("string");
  QTest::addColumn<int>("base");
  QTest::addColumn<int>("result");
  QTest::addColumn<bool>("ok");
  QTest::addColumn<QString>("end");

  QTest::newRow("integer") << "3" << 10 << 3 << true << QString();
  QTest::newRow("trailing") << "3trail" << 10 << 3 << true << "trail";
  QTest::newRow("badconversion") << "x" << 10 << 0 << false << QString();
  QTest::newRow("outofrange") << "99999999999999999" << 10 << 0 << false << QString();
  QTest::newRow("hex") << "f" << 16 << 15 << true << QString();
  QTest::newRow("autohex") << "0xf" << 0 << 15 << true << QString();
}

void TestParse::parse_int()
{
  struct dut_t{
    int result;
    bool ok;
    QString end;
  };
  dut_t dut;

  QFETCH(QString, string);
  QFETCH(int, base);
  QFETCH(int, result);
  QFETCH(bool, ok);
  QFETCH(QString, end);

  // int parse_integer(const QString& str, const QString& id, bool* ok = nullptr, QString* end = nullptr, int base = 10);
  dut.result = ::parse_integer(string, "unit", &dut.ok, &dut.end, base);
  QCOMPARE(dut.result, result);
  QCOMPARE(dut.ok, ok);
  QCOMPARE(dut.end, end);
#ifdef ENABLE_BENCHMARK
#endif
}

void TestParse::parse_distance_data()
{
  QTest::addColumn<QString>("string");
  QTest::addColumn<double>("scale");
  QTest::addColumn<int>("result");
  QTest::addColumn<double>("val");

  QTest::newRow("no-unit") << "3.00" << 4.0 << 1 << 12.0;
  QTest::newRow("m-explicit") << "3.00m" << 0.0 << 2 << 3.0;
  QTest::newRow("ft-explicit") << "3.00ft" << 0.0 << 2 << FEET_TO_METERS(3.0);
  QTest::newRow("feet-explicit") << "3.00feet" << 0.0 << 2 << FEET_TO_METERS(3.0);
  QTest::newRow("k-explicit") << "3.00k" << 0.0 << 2 << (1000.0*3.0);
  QTest::newRow("km-explicit") << "3.00km" << 0.0 << 2 << (1000.0*3.0);
  QTest::newRow("nm-explicit") << "3.00nm" << 0.0 << 2 << NMILES_TO_METERS(3.0);
  QTest::newRow("mi-explicit") << "3.00mi" << 0.0 << 2 << MILES_TO_METERS(3.0);
  QTest::newRow("fa-explicit") << "3.00fa" << 0.0 << 2 << FATHOMS_TO_METERS(3.0);
  //QTest::newRow("unkown-unit") << "3.00z" << 0.0 << 2 << 3.0; // generates fatal
}

void TestParse::parse_distance()
{
  struct dut_t{
    int result;
    double val;
  };
  dut_t dut;

  QFETCH(QString, string);
  QFETCH(double, scale);
  QFETCH(int, result);
  QFETCH(double, val);

  // int parse_distance(const QString& str, double* val, double scale);
  dut.result = ::parse_distance(string, &dut.val, scale);
  QCOMPARE(dut.result, result);
  QCOMPARE(dut.val, val);
}

void TestParse::parse_speed_data()
{
  QTest::addColumn<QString>("string");
  QTest::addColumn<double>("scale");
  QTest::addColumn<int>("result");
  QTest::addColumn<double>("val");

  QTest::newRow("no-unit") << "3.00" << 4.0 << 1 << 12.0;
  QTest::newRow("m/s-explicit") << "3.00m/s" << 0.0 << 2 << 3.0;
  QTest::newRow("mps-explicit") << "3.00mps" << 0.0 << 2 << 3.0;
  QTest::newRow("kph-explicit") << "3.00kph" << 0.0 << 2 << KPH_TO_MPS(3.0);
  QTest::newRow("km/h-explicit") << "3.00km/h" << 0.0 << 2 << KPH_TO_MPS(3.0);
  QTest::newRow("kmh-explicit") << "3.00kmh" << 0.0 << 2 << KPH_TO_MPS(3.0);
  QTest::newRow("kt-explicit") << "3.00kt" << 0.0 << 2 << KNOTS_TO_MPS(3.0);
  QTest::newRow("knot-explicit") << "3.00knot" << 0.0 << 2 << KNOTS_TO_MPS(3.0);
  QTest::newRow("mph-explicit") << "3.00mph" << 0.0 << 2 << MPH_TO_MPS(3.0);
  QTest::newRow("mi/h-explicit") << "3.00mi/h" << 0.0 << 2 << MPH_TO_MPS(3.0);
  QTest::newRow("mih-explicit") << "3.00mih" << 0.0 << 2 << MPH_TO_MPS(3.0);
  QTest::newRow("unkown-unit") << "3.00z" << 0.0 << 2 << 3.0; // generates warning
}

void TestParse::parse_speed()
{
  struct dut_t{
    int result;
    double val;
  };
  dut_t dut;

  QFETCH(QString, string);
  QFETCH(double, scale);
  QFETCH(int, result);
  QFETCH(double, val);

  // int parse_speed(const QString& str, double* val, double scale);
  dut.result = ::parse_speed(string, &dut.val, scale);
  QCOMPARE(dut.result, result);
  QCOMPARE(dut.val, val);
}

void TestParse::parse_coordinates_data()
{
  QTest::addColumn<QString>("string");
  QTest::addColumn<int>("datum");
  QTest::addColumn<grid_type>("grid");
  QTest::addColumn<int>("result");
  QTest::addColumn<double>("latitude");
  QTest::addColumn<double>("longitude");

  QTest::newRow("lat_lon_ddd_nw") << "N40.0 W105.0" << kDatumWGS84 << grid_lat_lon_ddd << 12 << 40.0 << -105.0;
  QTest::newRow("lat_lon_ddd_se") << "S40.0 E105.0" << kDatumWGS84 << grid_lat_lon_ddd << 12 << -40.0 << 105.0;
  QTest::newRow("lat_lon_dmm_nw") << "N40 30.0 W105 30.0" << kDatumWGS84 << grid_lat_lon_dmm << 18 << 40.5 << -105.5;
  QTest::newRow("lat_lon_dmm_se") << "S40 30.0 E105 30.0" << kDatumWGS84 << grid_lat_lon_dmm << 18 << -40.5 << 105.5;
  QTest::newRow("lat_lon_dms_nw") << "N40 30 45.0 W105 30 45.0" << kDatumWGS84 << grid_lat_lon_dms << 24 << 40.5125 << -105.5125;
  QTest::newRow("lat_lon_dms_se") << "S40 30 45.0 E105 30 45.0" << kDatumWGS84 << grid_lat_lon_dms << 24 << -40.5125 << 105.5125;
  QTest::newRow("bng-bigben") << "TQ 30366 80369" << kDatumOSGB36 << grid_bng << 14 << 51.50722265830027 << -0.122947217295196;
  QTest::newRow("utm") << "13 T 476911.0 4429455.0" << kDatumWGS84 << grid_utm << 23 << 40.01498084979945 << -105.2705495996336;
  QTest::newRow("swiss-matterhorn") << "617052.660 91680.657" << 123 << grid_swiss << 20 << 45.97648197853842 << 7.658661144525154;
}

void TestParse::parse_coordinates()
{
  struct dut_t {
    int result;
    double latitude;
    double longitude;
  };
  dut_t dut;

  QFETCH(QString, string);
  QFETCH(int, datum);
  QFETCH(grid_type, grid);
  QFETCH(int, result);
  QFETCH(double, latitude);
  QFETCH(double, longitude);

// int parse_coordinates(const QString& str, int datum, grid_type grid,
//                       double* latitude, double* longitude);
  dut.result = ::parse_coordinates(string, datum, grid, &dut.latitude, &dut.longitude);
  QCOMPARE(dut.result, result);
  QCOMPARE(dut.latitude, latitude);
  QCOMPARE(dut.longitude, longitude);
}

QTEST_MAIN(TestParse);
#include "tst_parse.moc"
