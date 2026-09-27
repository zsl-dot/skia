/*
 * Copyright 2022 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkPerfettoTrace_DEFINED
#define SkPerfettoTrace_DEFINED

#include "include/utils/SkEventTracer.h"
#include "tools/trace/EventTracingPriv.h"
#include "perfetto.h"

// MSVC 不接受零长数组（C2466 / C2131 / C2070）：空参数的 PERFETTO_DEFINE_CATEGORIES()
// 会展开成
//     constexpr ::perfetto::Category kCategories[] = {};
//     constexpr size_t kCategoryCount = sizeof(kCategories) / sizeof(kCategories[0]);
// GCC/Clang 把零长数组当扩展放行，MSVC 直接报错。这正是上游把 skia_use_perfetto 默认
// 限定在 linux/mac/android 的实际原因（gn/skia.gni:83 只是「没在 Windows 上验证过」的
// 保守默认值）。Skia 的追踪事件全部走 perfetto::DynamicCategory，动态类别在运行期按
// 名字解析，静态类别表不被使用，所以补一个占位类别让数组非空即可，事件语义不变。
PERFETTO_DEFINE_CATEGORIES(PERFETTO_CATEGORY(skia));

/**
 * This class is used to support Perfetto tracing. It hooks into the SkEventTracer system.
 */
class SkPerfettoTrace : public SkEventTracer {
public:
    SkPerfettoTrace();

    SkEventTracer::Handle addTraceEvent(char phase,
                                        const uint8_t* categoryEnabledFlag,
                                        const char* name,
                                        uint64_t id,
                                        int numArgs,
                                        const char** argNames,
                                        const uint8_t* argTypes,
                                        const uint64_t* argValues,
                                        uint8_t flags) override;


    void updateTraceEventDuration(const uint8_t* categoryEnabledFlag,
                                  const char* name,
                                  SkEventTracer::Handle handle) override;

    const uint8_t* getCategoryGroupEnabled(const char* name) override;

    const char* getCategoryGroupName(const uint8_t* categoryEnabledFlag) override;

    void newTracingSection(const char* name) override;

private:
    SkPerfettoTrace(const SkPerfettoTrace&) = delete;
    SkPerfettoTrace& operator=(const SkPerfettoTrace&) = delete;
    SkEventTracingCategories fCategories;
    std::unique_ptr<perfetto::TracingSession> tracingSession;
    int fd{-1};

    /** Store the perfetto trace file output path, name, and extension separately. This isolation
     * of name components becomes useful when splitting traces up by sections, where we want to
     * alter the base file name but keep the trace output path and file extension the same.
     */
    std::string fOutputPath;
    std::string fOutputFileExtension;
    std::string fCurrentSessionFullOutputPath;

    void openNewTracingSession(const std::string& baseFileName);
    void closeTracingSession();

    void onExit() override { this->closeTracingSession(); }

    /** Overloaded private methods to initiate a trace event with 0-2 arguments. Perfetto supports
     * adding an arbitrary number of debug annotations or arguments, but the existing Skia trace
     * structure only supports 0-2 so that is all we accommodate.
     */
    void triggerTraceEvent(const uint8_t* categoryEnabledFlag, const char* eventName);
    void triggerTraceEvent(const uint8_t* categoryEnabledFlag, const char* eventName,
                           const char* arg1Name, const uint8_t& arg1Type, const uint64_t& arg1Val);
    void triggerTraceEvent(const uint8_t* categoryEnabledFlag, const char* eventName,
                           const char* arg1Name, const uint8_t& arg1Type, const uint64_t& arg1Val,
                           const char* arg2Name, const uint8_t& arg2Type, const uint64_t& arg2Val);

    void triggerTraceCount(const uint8_t* categoryEnabledFlag, const char* eventName,
                           const uint8_t& arg1Type, const uint64_t& arg1Val);
};

#endif
