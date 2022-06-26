#include <inttypes.h>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <base/logging.h>

#include "baidu/streaming_log.h"
#include "base/comlog_sink.h"
#include "base/strings/stringprintf.h"
#include "com_log.h"
#include "cronoapd.h"

DEFINE_bool(use_log, false, "use_log");

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, false);

    std::string log_conf_file = "./conf/log.conf";

    if (FLAGS_use_log) {
        com_registappender("CRONOLOG", comspace::CronoAppender::getAppender,
                    comspace::CronoAppender::tryAppender);

        auto logger = logging::ComlogSink::GetInstance();
        if (0 != logger->SetupFromConfig(log_conf_file.c_str())) {
            LOG(FATAL) << "load log conf failed";
            return -1;
        }
    }

    return RUN_ALL_TESTS();
}
