#include <UnitTest++/UnitTest++.h>
#include <fstream>
#include <cstdio>
#include <string>
#include <cmath>
#include <limits>

#include "Interface.h"
#include "Base.h"
#include "Calc.h"
#include "Logger.h"
#include "Communicator.h"
#include "Errors.h"

#define BASE_FILE "test_base.txt"
#define LOG_FILE  "test_log.txt"


//создает правильный файл базы
struct BaseFileOk {
    BaseFileOk() {
        std::ofstream f(BASE_FILE);
        f << "loginov:SupErP@$$w0rd\n";
        f << "piter_parket:ow8ahra8uv\n";
    }
    ~BaseFileOk() { std::remove(BASE_FILE); }
};

//создает пустой файл базы
struct BaseFileEmpty {
    BaseFileEmpty() { std::ofstream f(BASE_FILE); }
    ~BaseFileEmpty() { std::remove(BASE_FILE); }
};

//создает файл с ошибкой
struct BaseFileBad {
    BaseFileBad() {
        std::ofstream f(BASE_FILE);
        f << "no_colon_here\n";
    }
    ~BaseFileBad() { std::remove(BASE_FILE); }
};

//для тестов логгера
struct LogFileTest {
    LogFileTest()  { std::remove(LOG_FILE); }
    ~LogFileTest() { std::remove(LOG_FILE); }
    
    std::string readLog() {
        std::ifstream f(LOG_FILE);
        std::string s((std::istreambuf_iterator<char>(f)),
                       std::istreambuf_iterator<char>());
        return s;
    }
};

//для тестов коммуникатора
struct CommTest {
    Base base;
    Logger logger;
    Calc calc;
    
    CommTest() : logger(LOG_FILE) {
        std::remove(BASE_FILE);
        std::ofstream f(BASE_FILE);
        f << "loginov:SupErP@$$w0rd\n";
        f.close();
        base.load(BASE_FILE);
    }
    ~CommTest() {
        std::remove(BASE_FILE);
        std::remove(LOG_FILE);
    }
};

//тесты для базы
SUITE(BaseTests) {
    //пустой файл
    TEST_FIXTURE(BaseFileEmpty, EmptyFile) {
        Base b;
        b.load(BASE_FILE);
        CHECK(!b.findLogin("anyone"));
    }

    //неверный формат
    TEST_FIXTURE(BaseFileBad, BadFormat) {
        Base b;
        CHECK_THROW(b.load(BASE_FILE), BaseError);
    }

    //нет файла
    TEST(NoFile) {
        Base b;
        CHECK_THROW(b.load("no_such_file.txt"), BaseError);
    }

    //норм файл
    TEST_FIXTURE(BaseFileOk, NormalFile) {
        Base b;
        b.load(BASE_FILE);
        CHECK(b.findLogin("loginov"));
        CHECK_EQUAL("SupErP@$$w0rd", b.getPassword("loginov"));
    }
}

//тесты для интерфейса

SUITE(InterfaceParams) {
    //порт строка
    TEST(PortIsString) {
        Interface i;
        const char* argv[] = {"vcalc", "-p", "abc", nullptr};
        CHECK_THROW(i.parseArguments(3, const_cast<char**>(argv)), InterfaceError);
    }

    //порт ноль
    TEST(PortZero) {
        Interface i;
        const char* argv[] = {"vcalc", "-p", "0", nullptr};
        CHECK_THROW(i.parseArguments(3, const_cast<char**>(argv)), InterfaceError);
    }

    //порт большой
    TEST(PortTooBig) {
        Interface i;
        const char* argv[] = {"vcalc", "-p", "65536", nullptr};
        CHECK_THROW(i.parseArguments(3, const_cast<char**>(argv)), InterfaceError);
    }

    //норм порт
    TEST(ValidPort) {
        Interface i;
        const char* argv[] = {"vcalc", "-p", "8080", nullptr};
        i.parseArguments(3, const_cast<char**>(argv));
        CHECK_EQUAL(8080, i.getParams().port);
    }
}

//тесты для интерфейса (ошибки и справка)

SUITE(InterfaceErrors) {
    //неизвестный параметр
    TEST(UnknownParam) {
        Interface i;
        const char* argv[] = {"vcalc", "-x", nullptr};
        CHECK_THROW(i.parseArguments(2, const_cast<char**>(argv)), InterfaceError);
    }

    //забыли значение
    TEST(NoValue) {
        Interface i;
        const char* argv[] = {"vcalc", "-b", nullptr};
        CHECK_THROW(i.parseArguments(2, const_cast<char**>(argv)), InterfaceError);
    }

    //справка
    TEST(Help) {
        Interface i;
        const char* argv[] = {"vcalc", "-h", nullptr};
        // Должно вернуть false, так как это справка
        CHECK(!i.parseArguments(2, const_cast<char**>(argv)));
    }
}

//тесты для калькулятора и логгера

SUITE(CalcAndLogger) {
    //пустой вектор
    TEST(CalcEmpty) {
        Calc c;
        CHECK_EQUAL(0.0f, c.sumSquares({}));
    }

    //обычные числа
    TEST(CalcNormal) {
        Calc c;
        // 1*1 + 2*2 + 3*3 = 14
        CHECK_EQUAL(14.0f, c.sumSquares({1.0f, 2.0f, 3.0f}));
    }

    //переполнение
    TEST(CalcInf) {
        Calc c;
        float big = std::numeric_limits<float>::max() / 2.0f;
        CHECK(std::isinf(c.sumSquares({big, big})));
    }

    //запись в лог
    TEST_FIXTURE(LogFileTest, WriteInfo) {
        {
            Logger log(LOG_FILE);
            log.writeLog("hello test", false);
        }
        std::string text = readLog();
        CHECK(text.find("[INFO]") != std::string::npos);
        CHECK(text.find("hello test") != std::string::npos);
    }

    //лог в плохую папку
    TEST(LoggerBadPath) {
        CHECK_THROW(Logger("/no_such_dir_xyz/log.txt"), InterfaceError);
    }
}

//тесты для коммуникатора

SUITE(CommunicatorTests) {
    const char* SALT = "4F9C429F5C6884DB726445BE";
    const char* PASS = "SupErP@$$w0rd";
    
    //хеш
    const char* HASH = "9C67ABCFAD8E46E78F89173497800E5EE143D67C28765A60C947221E7114FF68";

    //gроверка хеша
    TEST_FIXTURE(CommTest, Sha256) {
        Communicator c(0, base, logger, calc);
        std::string input = std::string(SALT) + PASS;
       CHECK_EQUAL(HASH, c.sha256Hex(input).c_str());
    }

    //короткое сообщение
    TEST_FIXTURE(CommTest, ShortMessage) {
        Communicator c(0, base, logger, calc);
        CHECK_THROW(c.parseAuthMessage("short"), CryptoError);
    }

    //успешная проверка пароля
    TEST_FIXTURE(CommTest, VerifyOk) {
        Communicator c(0, base, logger, calc);
        std::string h = c.sha256Hex(std::string(SALT) + PASS);
        CHECK(c.verifyHash("loginov", SALT, h));
    }

    //неверный пароль
    TEST_FIXTURE(CommTest, VerifyFail) {
        Communicator c(0, base, logger, calc);
        std::string bad_hash(64, '0');
        CHECK(!c.verifyHash("loginov", SALT, bad_hash));
    }
}

int main() {
    return UnitTest::RunAllTests();
}
