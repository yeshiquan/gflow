#include <inttypes.h>
#include <gtest/gtest.h>
#include <gflags/gflags.h>

//#include <base/logging.h>
//#include "baidu/streaming_log.h"
//#include "base/comlog_sink.h"
//#include "base/strings/stringprintf.h"
//#include "com_log.h"
//#include "cronoapd.h"

DEFINE_bool(use_log, false, "use_log");

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    ::gflags::ParseCommandLineFlags(&argc, &argv, false);

    std::string log_conf_file = "./conf/log.conf";

    return RUN_ALL_TESTS();
}
