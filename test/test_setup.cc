#include <clocale>
#include <QtResource>

void test_setup()
{
  setlocale(LC_NUMERIC,"C");
  setlocale(LC_TIME,"C");

  Q_INIT_RESOURCE(gpsbabel);
}
