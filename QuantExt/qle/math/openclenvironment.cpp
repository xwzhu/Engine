/*
 Copyright (C) 2023 Quaternion Risk Management Ltd
 All rights reserved.

 This file is part of ORE, a free-software/open-source library
 for transparent pricing and risk analysis - http://opensourcerisk.org

 ORE is free software: you can redistribute it and/or modify it
 under the terms of the Modified BSD License.  You should have received a
 copy of the license along with this program.
 The license is also available online at <http://opensourcerisk.org>

 This program is distributed on the basis that it will form a useful
 contribution to risk analytics and model standardisation, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. See the license for more details.
*/

#include <qle/math/openclenvironment.hpp>
#include <qle/math/randomvariable_opcodes.hpp>

#include <ql/errors.hpp>

#include <boost/algorithm/string/join.hpp>
#include <boost/timer/timer.hpp>

#include <chrono>
#include <iostream>
#include <thread>

#ifdef ORE_ENABLE_OPENCL
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#endif

#define MAX_N_PLATFORMS 4U
#define MAX_N_DEVICES 8U
#define MAX_N_DEV_INFO 256U
#define MAX_BUILD_LOG 65536U
#define MAX_BUILD_LOG_LOGFILE 1024U

namespace QuantExt {

#ifdef ORE_ENABLE_OPENCL
namespace {
std::string errorText(cl_int err) {
    switch (err) {
    case 0:
        return "CL_SUCCESS";
    case -1:
        return "CL_DEVICE_NOT_FOUND";
    case -2:
        return "CL_DEVICE_NOT_AVAILABLE";
    case -3:
        return "CL_COMPILER_NOT_AVAILABLE";
    case -4:
        return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case -5:
        return "CL_OUT_OF_RESOURCES";
    case -6:
        return "CL_OUT_OF_HOST_MEMORY";
    case -7:
        return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case -8:
        return "CL_MEM_COPY_OVERLAP";
    case -9:
        return "CL_IMAGE_FORMAT_MISMATCH";
    case -10:
        return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case -11:
        return "CL_BUILD_PROGRAM_FAILURE";
    case -12:
        return "CL_MAP_FAILURE";
    case -13:
        return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case -14:
        return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case -15:
        return "CL_COMPILE_PROGRAM_FAILURE";
    case -16:
        return "CL_LINKER_NOT_AVAILABLE";
    case -17:
        return "CL_LINK_PROGRAM_FAILURE";
    case -18:
        return "CL_DEVICE_PARTITION_FAILED";
    case -19:
        return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
    case -30:
        return "CL_INVALID_VALUE";
    case -31:
        return "CL_INVALID_DEVICE_TYPE";
    case -32:
        return "CL_INVALID_PLATFORM";
    case -33:
        return "CL_INVALID_DEVICE";
    case -34:
        return "CL_INVALID_CONTEXT";
    case -35:
        return "CL_INVALID_QUEUE_PROPERTIES";
    case -36:
        return "CL_INVALID_COMMAND_QUEUE";
    case -37:
        return "CL_INVALID_HOST_PTR";
    case -38:
        return "CL_INVALID_MEM_OBJECT";
    case -39:
        return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case -40:
        return "CL_INVALID_IMAGE_SIZE";
    case -41:
        return "CL_INVALID_SAMPLER";
    case -42:
        return "CL_INVALID_BINARY";
    case -43:
        return "CL_INVALID_BUILD_OPTIONS";
    case -44:
        return "CL_INVALID_PROGRAM";
    case -45:
        return "CL_INVALID_PROGRAM_EXECUTABLE";
    case -46:
        return "CL_INVALID_KERNEL_NAME";
    case -47:
        return "CL_INVALID_KERNEL_DEFINITION";
    case -48:
        return "CL_INVALID_KERNEL";
    case -49:
        return "CL_INVALID_ARG_INDEX";
    case -50:
        return "CL_INVALID_ARG_VALUE";
    case -51:
        return "CL_INVALID_ARG_SIZE";
    case -52:
        return "CL_INVALID_KERNEL_ARGS";
    case -53:
        return "CL_INVALID_WORK_DIMENSION";
    case -54:
        return "CL_INVALID_WORK_GROUP_SIZE";
    case -55:
        return "CL_INVALID_WORK_ITEM_SIZE";
    case -56:
        return "CL_INVALID_GLOBAL_OFFSET";
    case -57:
        return "CL_INVALID_EVENT_WAIT_LIST";
    case -58:
        return "CL_INVALID_EVENT";
    case -59:
        return "CL_INVALID_OPERATION";
    case -60:
        return "CL_INVALID_GL_OBJECT";
    case -61:
        return "CL_INVALID_BUFFER_SIZE";
    case -62:
        return "CL_INVALID_MIP_LEVEL";
    case -63:
        return "CL_INVALID_GLOBAL_WORK_SIZE";
    case -64:
        return "CL_INVALID_PROPERTY";
    case -65:
        return "CL_INVALID_IMAGE_DESCRIPTOR";
    case -66:
        return "CL_INVALID_COMPILER_OPTIONS";
    case -67:
        return "CL_INVALID_LINKER_OPTIONS";
    case -68:
        return "CL_INVALID_DEVICE_PARTITION_COUNT";
    default:
        return "unknown cl error code " + std::to_string(err);
    }
}

void errorCallback(const char* errinfo, const void* private_info, size_t cb, void* user_data) {
    printf("Error creating context: errinfo = '%s'\n", errinfo);
}

} // namespace

class OpenClContext : public ComputeContext {
public:
    OpenClContext(cl_device_id device, const std::vector<std::pair<std::string, std::string>>& deviceInfo,
                  const bool supportsDoublePrecision);
    ~OpenClContext() override final;
    void init() override final;

    std::pair<std::size_t, bool> initiateCalculation(const std::size_t n, const std::size_t id = 0,
                                                     const std::size_t version = 0,
                                                     const Settings settings = {}) override final;
    void disposeCalculation(const std::size_t n) override final;
    std::size_t createInputVariable(double v) override final;
    std::size_t createInputVariable(double* v) override final;
    std::vector<std::vector<std::size_t>> createInputVariates(const std::size_t dim,
                                                              const std::size_t steps) override final;
    std::size_t applyOperation(const std::size_t randomVariableOpCode,
                               const std::vector<std::size_t>& args) override final;
    void freeVariable(const std::size_t id) override final;
    void declareOutputVariable(const std::size_t id) override final;
    void finalizeCalculation(std::vector<double*>& output) override final;

    std::vector<std::pair<std::string, std::string>> deviceInfo() const override;
    bool supportsDoublePrecision() const override;
    const DebugInfo& debugInfo() const override final;

private:
    void updateVariatesPool();

    void runHealthChecks();
    std::string runHealthCheckProgram(const std::string& source, const std::string& kernelName);

    static void releaseMem(cl_mem& m, const std::string& desc);
    static void releaseKernel(cl_kernel& k, const std::string& desc);
    static void releaseProgram(cl_program& p, const std::string& desc);

    enum class ComputeState { idle, createInput, createVariates, calc };

    bool healthy_ = true;
    bool initialized_ = false;
    cl_device_id device_;
    cl_context context_;
    cl_command_queue queue_;

    // set once in the ctor
    std::vector<std::pair<std::string, std::string>> deviceInfo_;
    bool supportsDoublePrecision_;

    // will be accumulated over all calcs
    ComputeContext::DebugInfo debugInfo_;

    // 1a vectors per current calc id

    std::vector<std::size_t> size_;
    std::vector<bool> disposed_;
    std::vector<bool> hasKernel_;
    std::vector<std::size_t> version_;
    std::vector<cl_program> program_;
    std::vector<cl_kernel> kernel_;
    std::vector<std::size_t> inputBufferSize_;
    std::vector<std::size_t> nOutputVars_;

    // 1b variates (shared pool of mersenne twister based normal variates)

    std::size_t variatesPoolSize_ = 0; // count of single random numbers
    cl_mem variatesPool_;
    cl_mem variatesMtStateBuffer_;
    cl_program variatesProgram_;
    cl_kernel variatesKernelSeedInit_;
    cl_kernel variatesKernelTwist_;
    cl_kernel variatesKernelGenerate_;

    // 2 curent calc

    std::size_t currentId_ = 0;
    ComputeState currentState_ = ComputeState::idle;
    std::size_t nVars_;
    std::size_t nVariates_;
    Settings settings_;

    // 2a indexed by var id
    std::vector<std::size_t> inputVarOffset_;
    std::vector<bool> inputVarIsScalar_;
    std::vector<float> inputVarValues32_;
    std::vector<double> inputVarValues64_;

    // 2b collection of variable ids
    std::vector<std::size_t> freedVariables_;
    std::vector<std::size_t> outputVariables_;

    // 2d kernel ssa
    std::string currentSsa_;
};

OpenClFramework::OpenClFramework() {
    cl_platform_id platforms[MAX_N_PLATFORMS];
    cl_uint nPlatforms;
    clGetPlatformIDs(MAX_N_PLATFORMS, platforms, &nPlatforms);
    for (std::size_t p = 0; p < nPlatforms; ++p) {
        char platformName[MAX_N_DEV_INFO];
        clGetPlatformInfo(platforms[p], CL_PLATFORM_NAME, MAX_N_DEV_INFO, platformName, NULL);
        cl_device_id devices[MAX_N_DEVICES];
        cl_uint nDevices;
        clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_ALL, 3, devices, &nDevices);
        for (std::size_t d = 0; d < nDevices; ++d) {
            char deviceName[MAX_N_DEV_INFO], driverVersion[MAX_N_DEV_INFO], deviceVersion[MAX_N_DEV_INFO],
                deviceExtensions[MAX_N_DEV_INFO];
            cl_device_fp_config doubleFpConfig;
            std::vector<std::pair<std::string, std::string>> deviceInfo;

            clGetDeviceInfo(devices[d], CL_DEVICE_NAME, MAX_N_DEV_INFO, &deviceName, NULL);
            clGetDeviceInfo(devices[d], CL_DRIVER_VERSION, MAX_N_DEV_INFO, &driverVersion, NULL);
            clGetDeviceInfo(devices[d], CL_DEVICE_VERSION, MAX_N_DEV_INFO, &deviceVersion, NULL);
            clGetDeviceInfo(devices[d], CL_DEVICE_EXTENSIONS, MAX_N_DEV_INFO, &deviceExtensions, NULL);

            deviceInfo.push_back(std::make_pair("device_name", std::string(deviceName)));
            deviceInfo.push_back(std::make_pair("driver_version", std::string(driverVersion)));
            deviceInfo.push_back(std::make_pair("device_version", std::string(deviceVersion)));
            deviceInfo.push_back(std::make_pair("device_extensions", std::string(deviceExtensions)));

            bool supportsDoublePrecision = false;
#if CL_VERSION_1_2
            clGetDeviceInfo(devices[d], CL_DEVICE_DOUBLE_FP_CONFIG, sizeof(cl_device_fp_config), &doubleFpConfig, NULL);
            deviceInfo.push_back(std::make_pair(
                "device_double_fp_config",
                ((doubleFpConfig & CL_FP_DENORM) ? std::string("Denorm,") : std::string()) +
                    ((doubleFpConfig & CL_FP_INF_NAN) ? std::string("InfNan,") : std::string()) +
                    ((doubleFpConfig & CL_FP_ROUND_TO_NEAREST) ? std::string("RoundNearest,") : std::string()) +
                    ((doubleFpConfig & CL_FP_ROUND_TO_ZERO) ? std::string("RoundZero,") : std::string()) +
                    ((doubleFpConfig & CL_FP_FMA) ? std::string("FMA,") : std::string()) +
                    ((doubleFpConfig & CL_FP_SOFT_FLOAT) ? std::string("SoftFloat,") : std::string())));
            supportsDoublePrecision = supportsDoublePrecision || (doubleFpConfig != 0);
#else
            deviceInfo.push_back(std::make_pair("device_double_fp_config", "not provided before opencl 1.2"));
            supportsDoublePrecision = supportsDoublePrecision || std::string(deviceExtensions).find("cl_khr_fp64");
#endif
            contexts_["OpenCL/" + std::string(platformName) + "/" + std::string(deviceName)] =
                new OpenClContext(devices[d], deviceInfo, supportsDoublePrecision);
        }
    }
}

OpenClFramework::~OpenClFramework() {
    for (auto& [_, c] : contexts_) {
        delete c;
    }
}

OpenClContext::OpenClContext(cl_device_id device, const std::vector<std::pair<std::string, std::string>>& deviceInfo,
                             const bool supportsDoublePrecision)
    : initialized_(false), device_(device), deviceInfo_(deviceInfo), supportsDoublePrecision_(supportsDoublePrecision) {
}

OpenClContext::~OpenClContext() {
    if (initialized_) {
        cl_int err;

        if (variatesPoolSize_ > 0) {
            releaseMem(variatesPool_, "variates pool");
            releaseMem(variatesMtStateBuffer_, "variates state buffer");
            releaseKernel(variatesKernelSeedInit_, "variates seed init");
            releaseKernel(variatesKernelTwist_, "variates twist");
            releaseKernel(variatesKernelGenerate_, "variates generate");
            releaseProgram(variatesProgram_, "variates");
        }

        for (Size i = 0; i < kernel_.size(); ++i) {
            if (disposed_[i])
                continue;
            releaseKernel(kernel_[i], "ore kernel");
        }

        for (Size i = 0; i < program_.size(); ++i) {
            if (disposed_[i])
                continue;
            releaseProgram(program_[i], "ore program");
        }

        if (err = clReleaseCommandQueue(queue_); err != CL_SUCCESS) {
            std::cerr << "OpenClContext: error during clReleaseCommandQueue: " + errorText(err) << std::endl;
        }

        if (err = clReleaseContext(context_); err != CL_SUCCESS) {
            std::cerr << "OpenClContext: error during clReleaseContext: " + errorText(err) << std::endl;
        }
    }
}

void OpenClContext::releaseMem(cl_mem& m, const std::string& description) {
    cl_int err;
    if (err = clReleaseMemObject(m); err != CL_SUCCESS) {
        std::cerr << "OpenClContext: error during clReleaseMemObject '" << description << "': " + errorText(err)
                  << std::endl;
    }
}

void OpenClContext::releaseKernel(cl_kernel& k, const std::string& description) {
    cl_int err;
    if (err = clReleaseKernel(k); err != CL_SUCCESS) {
        std::cerr << "OpenClContext: error during clReleaseKernel'" << description << "': " + errorText(err)
                  << std::endl;
    }
}

void OpenClContext::releaseProgram(cl_program& p, const std::string& description) {
    cl_int err;
    if (err = clReleaseProgram(p); err != CL_SUCCESS) {
        std::cerr << "OpenClContext: error during clReleaseProgram'" << description << "': " + errorText(err)
                  << std::endl;
    }
}

std::string OpenClContext::runHealthCheckProgram(const std::string& source, const std::string& kernelName) {

    struct CleanUp {
        std::vector<cl_program> p;
        std::vector<cl_kernel> k;
        std::vector<cl_mem> m;
        ~CleanUp() {
            for (auto& pgm : p)
                OpenClContext::releaseProgram(pgm, "health check");
            for (auto& krn : k)
                OpenClContext::releaseKernel(krn, "health check");
            for (auto& mem : m)
                OpenClContext::releaseMem(mem, "health check");
        }
    } cleanup;

    const char* programPtr = source.c_str();

    cl_int err;

    cl_program program = clCreateProgramWithSource(context_, 1, &programPtr, NULL, &err);
    if (err != CL_SUCCESS) {
        return errorText(err);
    }
    cleanup.p.push_back(program);

    err = clBuildProgram(program, 1, &device_, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        return errorText(err);
    }

    cl_kernel kernel = clCreateKernel(program, kernelName.c_str(), &err);
    if (err != CL_SUCCESS) {
        return errorText(err);
    }
    cleanup.k.push_back(kernel);

    cl_mem resultBuffer = clCreateBuffer(context_, CL_MEM_READ_WRITE, sizeof(cl_ulong), NULL, &err);
    if (err != CL_SUCCESS) {
        return errorText(err);
    }
    cleanup.m.push_back(resultBuffer);

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &resultBuffer);

    cl_event runEvent;
    constexpr std::size_t sizeOne = 1;
    err = clEnqueueNDRangeKernel(queue_, kernel, 1, NULL, &sizeOne, NULL, 0, NULL, &runEvent);
    if (err != CL_SUCCESS) {
        return errorText(err);
    }

    cl_ulong result;
    err = clEnqueueReadBuffer(queue_, resultBuffer, CL_TRUE, 0, sizeof(cl_ulong), &result, 1, &runEvent, NULL);
    if (err != CL_SUCCESS) {
        return errorText(err);
    }

    return std::to_string(result);
}

void OpenClContext::runHealthChecks() {
    deviceInfo_.push_back(std::make_pair("host_sizeof(cl_uint)", std::to_string(sizeof(cl_uint))));
    deviceInfo_.push_back(std::make_pair("host_sizeof(cl_ulong)", std::to_string(sizeof(cl_ulong))));
    deviceInfo_.push_back(std::make_pair("host_sizeof(cl_float)", std::to_string(sizeof(cl_float))));
    deviceInfo_.push_back(std::make_pair("host_sizeof(cl_double)", std::to_string(sizeof(cl_double))));

    std::string kernelGetUintSize =
        "__kernel void ore_get_uint_size(__global ulong* result) { result[0] = sizeof(uint); }";
    std::string kernelGetUlongSize =
        "__kernel void ore_get_ulong_size(__global ulong* result) { result[0] = sizeof(ulong); }";
    std::string kernelGetFloatSize =
        "__kernel void ore_get_float_size(__global ulong* result) { result[0] = sizeof(float); }";
    std::string kernelGetDoubleSize =
        "__kernel void ore_get_double_size(__global ulong* result) { result[0] = sizeof(double); }";

    deviceInfo_.push_back(
        std::make_pair("device_sizeof(uint)", runHealthCheckProgram(kernelGetUintSize, "ore_get_uint_size")));
    deviceInfo_.push_back(
        std::make_pair("device_sizeof(ulong)", runHealthCheckProgram(kernelGetUlongSize, "ore_get_ulong_size")));
    deviceInfo_.push_back(
        std::make_pair("device_sizeof(float)", runHealthCheckProgram(kernelGetFloatSize, "ore_get_float_size")));
    deviceInfo_.push_back(
        std::make_pair("device_sizeof(double)", runHealthCheckProgram(kernelGetDoubleSize, "ore_get_double_size")));
}

void OpenClContext::init() {

    QL_REQUIRE(healthy_, "OpenClCOntext::init(): context is not healthy, check log for previous errors, aborting.");

    if (initialized_) {
        return;
    }

    debugInfo_.numberOfOperations = 0;
    debugInfo_.nanoSecondsDataCopy = 0;
    debugInfo_.nanoSecondsProgramBuild = 0;
    debugInfo_.nanoSecondsCalculation = 0;

    cl_int err;

    // create context and command queue

    healthy_ = false; // temporary

    for(Size attempt = 0; attempt < 10 && err != CL_SUCCESS; ++attempt) {
        context_ = clCreateContext(NULL, 1, &device_, &errorCallback, NULL, &err);
        if(err != CL_SUCCESS) {
            std::cerr << "error during clCreateContext(): " << errorText(err) << " - will retry after 10s, attempt "
                      << attempt + 1 << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
    QL_REQUIRE(err == CL_SUCCESS, "OpenClContext::OpenClContext(): error during clCreateContext(): " << errorText(err));

#if CL_VERSION_2_0
    queue_ = clCreateCommandQueueWithProperties(context_, device_, NULL, &err);
#else
    // deprecated in cl version 2_0
    queue_ = clCreateCommandQueue(context_, device_, 0, &err);
#endif
    QL_REQUIRE(err == CL_SUCCESS,
               "OpenClContext::OpenClContext(): error during clCreateCommandQueue(): " << errorText(err));

    healthy_ = initialized_ = true;

    runHealthChecks();
}

void OpenClContext::disposeCalculation(const std::size_t id) {
    QL_REQUIRE(!disposed_[id - 1], "OpenClContext::disposeCalculation(): id " << id << " was already disposed.");
    disposed_[id - 1] = true;
    releaseKernel(kernel_[id - 1], "kernel id " + std::to_string(id));
    releaseProgram(program_[id - 1], "program id " + std::to_string(id));
}

std::pair<std::size_t, bool> OpenClContext::initiateCalculation(const std::size_t n, const std::size_t id,
                                                                const std::size_t version, const Settings settings) {

    QL_REQUIRE(n > 0, "OpenClContext::initiateCalculation(): n must not be zero");

    bool newCalc = false;
    settings_ = settings;

    if (id == 0) {

        // initiate new calcaultion

        size_.push_back(n);
        disposed_.push_back(false);
        hasKernel_.push_back(false);
        version_.push_back(version);
        program_.push_back(cl_program());
        kernel_.push_back(cl_kernel());
        inputBufferSize_.push_back(0);
        nOutputVars_.push_back(0);

        currentId_ = hasKernel_.size();
        newCalc = true;

    } else {

        // initiate calculation on existing id

        QL_REQUIRE(id <= hasKernel_.size(),
                   "OpenClContext::initiateCalculation(): id (" << id << ") invalid, got 1..." << hasKernel_.size());
        QL_REQUIRE(size_[id - 1] == n, "OpenClContext::initiateCalculation(): size ("
                                           << size_[id - 1] << ") for id " << id << " does not match current size ("
                                           << n << ")");
        QL_REQUIRE(!disposed_[id - 1], "OpenClContext::initiateCalculation(): id ("
                                           << id << ") was already disposed, it can not be used any more.");

        if (version != version_[id - 1]) {
            hasKernel_[id - 1] = false;
            version_[id - 1] = version;
            releaseKernel(kernel_[id - 1], "kernel id " + std::to_string(id));
            releaseProgram(program_[id - 1], "program id " + std::to_string(id));
            newCalc = true;
        }

        currentId_ = id;
    }

    // reset variable info

    nVars_ = 0;

    inputVarOffset_.clear();
    inputVarIsScalar_.clear();
    inputVarValues32_.clear();
    inputVarValues64_.clear();

    if (newCalc) {
        freedVariables_.clear();
        outputVariables_.clear();
        nVariates_ = 0;

        // reset ssa

        currentSsa_.clear();
    }

    // set state

    currentState_ = ComputeState::createInput;

    // return calc id

    return std::make_pair(currentId_, newCalc);
}

std::size_t OpenClContext::createInputVariable(double v) {
    QL_REQUIRE(currentState_ == ComputeState::createInput,
               "OpenClContext::createInputVariable(): not in state createInput (" << static_cast<int>(currentState_)
                                                                                  << ")");
    std::size_t nextOffset = 0;
    if (!inputVarOffset_.empty()) {
        nextOffset = inputVarOffset_.back() + (inputVarIsScalar_.back() ? 1 : size_[currentId_ - 1]);
    }
    inputVarOffset_.push_back(nextOffset);
    inputVarIsScalar_.push_back(true);
    if (settings_.useDoublePrecision) {
        inputVarValues64_.push_back(v);
    } else {
        // ensure that v falls into the single precision range
        inputVarValues32_.push_back((float)std::max(std::min(v, (double)std::numeric_limits<float>::max()),
                                                    -(double)std::numeric_limits<float>::max()));
    }
    return nVars_++;
}

std::size_t OpenClContext::createInputVariable(double* v) {
    QL_REQUIRE(currentState_ == ComputeState::createInput,
               "OpenClContext::createInputVariable(): not in state createInput (" << static_cast<int>(currentState_)
                                                                                  << ")");
    std::size_t nextOffset = 0;
    if (!inputVarOffset_.empty()) {
        nextOffset = inputVarOffset_.back() + (inputVarIsScalar_.back() ? 1 : size_[currentId_ - 1]);
    }
    inputVarOffset_.push_back(nextOffset);
    inputVarIsScalar_.push_back(false);
    for (std::size_t i = 0; i < size_[currentId_ - 1]; ++i) {
        if (settings_.useDoublePrecision) {
            inputVarValues64_.push_back(v[i]);
        } else {
            inputVarValues32_.push_back((float)std::max(std::min(v[i], (double)std::numeric_limits<float>::max()),
                                                        -(double)std::numeric_limits<float>::max()));
        }
    }
    return nVars_++;
}

void OpenClContext::updateVariatesPool() {

    QL_REQUIRE(nVariates_ > 0, "OpenClContext::updateVariatesPool(): internal error, got nVariates_ == 0.");

    constexpr std::size_t mt_N = 624; // mersenne twister N

    std::size_t fpSize = settings_.useDoublePrecision ? sizeof(double) : sizeof(float);

    cl_event initEvent;
    if (variatesPoolSize_ == 0) {

        // build the kernels to fill the variates pool

        std::string fpTypeStr = settings_.useDoublePrecision ? "double" : "float";
        std::string fpSuffix = settings_.useDoublePrecision ? "" : "f";

        // clang-format off
        // ported from from QuantLib::InverseCumulativeNormal
        std::string sourceInvCumN = fpTypeStr + " ore_invCumN(const uint x0) {\n"
            "    const " + fpTypeStr + " a1_ = -3.969683028665376e+01" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " a2_ = 2.209460984245205e+02" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " a3_ = -2.759285104469687e+02" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " a4_ = 1.383577518672690e+02" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " a5_ = -3.066479806614716e+01" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " a6_ = 2.506628277459239e+00" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " b1_ = -5.447609879822406e+01" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " b2_ = 1.615858368580409e+02" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " b3_ = -1.556989798598866e+02" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " b4_ = 6.680131188771972e+01" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " b5_ = -1.328068155288572e+01" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " c1_ = -7.784894002430293e-03" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " c2_ = -3.223964580411365e-01" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " c3_ = -2.400758277161838e+00" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " c4_ = -2.549732539343734e+00" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " c5_ = 4.374664141464968e+00" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " c6_ = 2.938163982698783e+00" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " d1_ = 7.784695709041462e-03" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " d2_ = 3.224671290700398e-01" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " d3_ = 2.445134137142996e+00" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " d4_ = 3.754408661907416e+00" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " x_low_ = 0.02425" + fpSuffix + ";\n"
            "    const " + fpTypeStr + " x_high_ = 1.0" + fpSuffix + " - x_low_;\n"
            "    const " + fpTypeStr + " x = x0 / (" + fpTypeStr + ")UINT_MAX;\n"
            "    if (x < x_low_ || x_high_ < x) {\n"
            "        if (x0 == UINT_MAX) {\n"
            "          return 0x1.fffffep127" + fpSuffix + ";\n"
            "        } else if(x0 == 0) {\n"
            "          return -0x1.fffffep127" + fpSuffix + ";\n"
            "        }\n"
            "        " + fpTypeStr + " z;\n"
            "        if (x < x_low_) {\n"
            "            z = sqrt(-2.0" + fpSuffix + " * log(x));\n"
            "            z = (((((c1_ * z + c2_) * z + c3_) * z + c4_) * z + c5_) * z + c6_) /\n"
            "                ((((d1_ * z + d2_) * z + d3_) * z + d4_) * z + 1.0" + fpSuffix + ");\n"
            "        } else {\n"
            "            z = sqrt(-2.0f * log(1.0f - x));\n"
            "            z = -(((((c1_ * z + c2_) * z + c3_) * z + c4_) * z + c5_) * z + c6_) /\n"
            "                ((((d1_ * z + d2_) * z + d3_) * z + d4_) * z + 1.0" + fpSuffix + ");\n"
            "        }\n"
            "        return z;\n"
            "    } else {\n"
            "        " + fpTypeStr + " z = x - 0.5" + fpSuffix + ";\n"
            "        " + fpTypeStr + " r = z * z;\n"
            "        z = (((((a1_ * r + a2_) * r + a3_) * r + a4_) * r + a5_) * r + a6_) * z /\n"
            "            (((((b1_ * r + b2_) * r + b3_) * r + b4_) * r + b5_) * r + 1.0" + fpSuffix +");\n"
            "        return z;\n"
            "    }\n"
            "}\n\n";

        // from QuantLib::MersenneTwisterUniformRng

        std::string kernelSourceSeedInit = "__kernel void ore_seedInitialization(const ulong s, __global ulong* mt) {\n"
            "  const ulong N = 624;\n"
            "  mt[0]= s & 0xffffffffU;\n"
            "  for (ulong mti=1; mti<N; ++mti) {\n"
            "    mt[mti] = (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);\n"
            "    mt[mti] &= 0xffffffffUL;\n"
            "  }\n"
            "}\n\n";

        std::string kernelSourceTwist = "__kernel void ore_twist(__global ulong* mt) {\n"
            " const ulong N = 624;\n"
            " const ulong M = 397;\n"
            " const ulong MATRIX_A = 0x9908b0dfUL;\n"
            " const ulong UPPER_MASK=0x80000000UL;\n"
            " const ulong LOWER_MASK=0x7fffffffUL;\n"
            " const ulong mag01[2]={0x0UL, MATRIX_A};\n"
            " ulong kk;\n"
            " ulong y;\n"
            " for (kk=0;kk<N-M;++kk) {\n"
            "     y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);\n"
            "     mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];\n"
            " }\n"
            " for (;kk<N-1;kk++) {\n"
            "     y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);\n"
            "     mt[kk] = mt[(kk+M)-N] ^ (y >> 1) ^ mag01[y & 0x1UL];\n"
            " }\n"
            " y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);\n"
            " mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];\n"
            "}\n\n";

        std::string kernelSourceGenerate =
            "__kernel void ore_generate(const ulong offset, __global ulong* mt, __global " + fpTypeStr + "* output) {\n"
            "   ulong mti = get_global_id(0);\n"
            "   ulong y = mt[mti];\n"
            "   y ^= (y >> 11);\n"
            "   y ^= (y << 7) & 0x9d2c5680U;\n"
            "   y ^= (y << 15) & 0xefc60000U;\n"
            "   y ^= (y >> 18);\n"
            "   output[offset + mti] = ore_invCumN((uint)y);\n"
            "}\n\n";
        // clang-format on

        std::string programSource = sourceInvCumN + kernelSourceSeedInit + kernelSourceTwist + kernelSourceGenerate;

        // std::cerr << "generated variates program:\n" + programSource << std::endl;

        const char* programSourcePtr = programSource.c_str();
        cl_int err;
        variatesProgram_ = clCreateProgramWithSource(context_, 1, &programSourcePtr, NULL, &err);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::updateVariatesPool(): error creating program: " << errorText(err));
        err = clBuildProgram(variatesProgram_, 1, &device_, NULL, NULL, NULL);
        if (err != CL_SUCCESS) {
            char buffer[MAX_BUILD_LOG];
            clGetProgramBuildInfo(variatesProgram_, device_, CL_PROGRAM_BUILD_LOG, MAX_BUILD_LOG * sizeof(char), buffer,
                                  NULL);
            QL_FAIL("OpenClContext::updateVariatesPool(): error during program build: "
                    << errorText(err) << ": " << std::string(buffer).substr(MAX_BUILD_LOG_LOGFILE));
        }

        variatesKernelSeedInit_ = clCreateKernel(variatesProgram_, "ore_seedInitialization", &err);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::updateVariatesPool(): error creating kernel seedInit: " << errorText(err));

        variatesKernelTwist_ = clCreateKernel(variatesProgram_, "ore_twist", &err);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::updateVariatesPool(): error creating kernel twist: " << errorText(err));

        variatesKernelGenerate_ = clCreateKernel(variatesProgram_, "ore_generate", &err);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::updateVariatesPool(): error creating kernel generate: " << errorText(err));

        variatesMtStateBuffer_ = clCreateBuffer(context_, CL_MEM_READ_WRITE, sizeof(cl_ulong) * mt_N, NULL, &err);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::updateVariatesPool(): error creating mt state buffer: " << errorText(err));

        cl_ulong tmpSeed = (cl_ulong)settings_.rngSeed;
        err = clSetKernelArg(variatesKernelSeedInit_, 0, sizeof(cl_ulong), &tmpSeed);
        err |= clSetKernelArg(variatesKernelSeedInit_, 1, sizeof(cl_mem), &variatesMtStateBuffer_);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::updateVariatesPool(): error setting kernel args seed init: " << errorText(err));

        constexpr std::size_t sizeOne = 1;
        err = clEnqueueNDRangeKernel(queue_, variatesKernelSeedInit_, 1, NULL, &sizeOne, NULL, 0, NULL, &initEvent);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::updateVariatesPool(): error running kernel seed init: " << errorText(err));
    }

    // if the variates pool is big enough, we exit early

    if (variatesPoolSize_ >= nVariates_ * size_[currentId_ - 1]) {
        if (variatesPoolSize_ == 0)
            clWaitForEvents(1, &initEvent);
        return;
    }

    // create new buffer to hold the variates and copy the current buffer contents to the new buffer

    Size alignedSize =
        624 * (nVariates_ * size_[currentId_ - 1] / 624 + (nVariates_ * size_[currentId_ - 1] % 624 == 0 ? 0 : 1));

    cl_int err;

    cl_mem oldBuffer = variatesPool_;

    struct OldBufferReleaser {
        OldBufferReleaser(cl_mem b, bool active) : b(b), active(active) {}
        ~OldBufferReleaser() {
            if (active)
                OpenClContext::releaseMem(b, "expired variates buffer");
        }
        cl_mem b;
        bool active;
    } oldBufferReleaser(oldBuffer, variatesPoolSize_ > 0);

    variatesPool_ = clCreateBuffer(context_, CL_MEM_READ_WRITE, fpSize * alignedSize, NULL, &err);
    QL_REQUIRE(err == CL_SUCCESS, "OpenClContext::updateVariatesPool(): error creating variates buffer with size "
                                      << fpSize * alignedSize << " bytes: " << errorText(err));
    cl_event copyEvent;
    if (variatesPoolSize_ > 0) {
        err = clEnqueueCopyBuffer(queue_, oldBuffer, variatesPool_, 0, 0, fpSize * variatesPoolSize_, 0, NULL,
                                  &copyEvent);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::updateVariatesPool(): error copying existing variates buffer to new buffer: "
                       << errorText(err));
    }

    // fill in the new variates

    std::size_t currentPoolSize;
    cl_event generateEvent;
    bool haveGenerated = false;
    for (currentPoolSize = variatesPoolSize_; currentPoolSize < nVariates_ * size_[currentId_ - 1];
         currentPoolSize += mt_N) {
        err = clSetKernelArg(variatesKernelTwist_, 0, sizeof(cl_mem), &variatesMtStateBuffer_);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::updateVariatesPool(): error setting args for kernel twist: " << errorText(err));
        cl_event twistEvent;
        err = clEnqueueNDRangeKernel(
            queue_, variatesKernelTwist_, 1, NULL, &mt_N, NULL, variatesPoolSize_ == 0 || haveGenerated ? 1 : 0,
            variatesPoolSize_ == 0 ? &initEvent : (haveGenerated ? &generateEvent : NULL), &twistEvent);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::updateVariatesPool(): error running kernel twist: " << errorText(err));

        err = clSetKernelArg(variatesKernelGenerate_, 0, sizeof(cl_ulong), &currentPoolSize);
        err |= clSetKernelArg(variatesKernelGenerate_, 1, sizeof(cl_mem), &variatesMtStateBuffer_);
        err |= clSetKernelArg(variatesKernelGenerate_, 2, sizeof(cl_mem), &variatesPool_);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::updateVariatesPool(): error settings args for kernel generate: " << errorText(err));
        err = clEnqueueNDRangeKernel(queue_, variatesKernelGenerate_, 1, NULL, &mt_N, NULL, 1, &twistEvent,
                                     &generateEvent);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::updateVariatesPool(): error running kernel generate: " << errorText(err));
        haveGenerated = true;
    }

    // wait for events to finish

    std::vector<cl_event> waitList;
    if (variatesPoolSize_ > 0)
        waitList.push_back(copyEvent);
    if (haveGenerated)
        waitList.push_back(generateEvent);
    if (!waitList.empty())
        clWaitForEvents(waitList.size(), &waitList[0]);

    // update current variates pool size

    QL_REQUIRE(currentPoolSize == alignedSize, "OpenClContext::updateVariatesPool(): internal error, currentPoolSize = "
                                                   << currentPoolSize << " does not match alignedSize " << alignedSize);
    variatesPoolSize_ = currentPoolSize;
}

std::vector<std::vector<std::size_t>> OpenClContext::createInputVariates(const std::size_t dim,
                                                                         const std::size_t steps) {
    QL_REQUIRE(currentState_ == ComputeState::createInput || currentState_ == ComputeState::createVariates,
               "OpenClContext::createInputVariable(): not in state createInput or createVariates ("
                   << static_cast<int>(currentState_) << ")");
    QL_REQUIRE(currentId_ > 0, "OpenClContext::applyOperation(): current id is not set");
    QL_REQUIRE(!hasKernel_[currentId_ - 1], "OpenClContext::createInputVariates(): id ("
                                                << currentId_ << ") in version " << version_[currentId_ - 1]
                                                << " has a kernel already, input variates can not be regenerated.");
    currentState_ = ComputeState::createVariates;
    std::vector<std::vector<std::size_t>> resultIds(dim, std::vector<std::size_t>(steps));
    for (std::size_t j = 0; j < steps; ++j) {
        for (std::size_t i = 0; i < dim; ++i) {
            resultIds[i][j] = nVars_++;
        }
    }
    nVariates_ += dim * steps;
    updateVariatesPool();
    return resultIds;
}

std::size_t OpenClContext::applyOperation(const std::size_t randomVariableOpCode,
                                          const std::vector<std::size_t>& args) {
    QL_REQUIRE(currentState_ == ComputeState::createInput || currentState_ == ComputeState::createVariates ||
                   currentState_ == ComputeState::calc,
               "OpenClContext::applyOperation(): not in state createInput or calc (" << static_cast<int>(currentState_)
                                                                                     << ")");
    currentState_ = ComputeState::calc;
    QL_REQUIRE(currentId_ > 0, "OpenClContext::applyOperation(): current id is not set");
    QL_REQUIRE(!hasKernel_[currentId_ - 1], "OpenClContext::applyOperation(): id (" << currentId_ << ") in version "
                                                                                    << version_[currentId_ - 1]
                                                                                    << " has a kernel already.");

    // determine variable id to use for result

    std::size_t resultId;
    bool resultIdNeedsDeclaration;
    if (!freedVariables_.empty()) {
        resultId = freedVariables_.back();
        freedVariables_.pop_back();
        resultIdNeedsDeclaration = false;
    } else {
        resultId = nVars_++;
        resultIdNeedsDeclaration = true;
    }

    // determine arg variable names

    std::vector<std::string> argStr(args.size());
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] < inputVarOffset_.size()) {
            argStr[i] = "input[" + std::to_string(inputVarOffset_[args[i]]) + "U" +
                        (inputVarIsScalar_[args[i]] ? "]" : " + i]");
        } else if (args[i] < inputVarOffset_.size() + nVariates_) {
            argStr[i] = "rn[" + std::to_string((args[i] - inputVarOffset_.size()) * size_[currentId_ - 1]) + "U + i]";
        } else {
            // variable is an (intermediate) result
            argStr[i] = "v" + std::to_string(args[i]);
        }
    }

    // generate ssa entry

    std::string fpTypeStr = settings_.useDoublePrecision ? "double" : "float";

    std::string ssaLine =
        (resultIdNeedsDeclaration ? fpTypeStr + " " : "") + std::string("v") + std::to_string(resultId) + " = ";

    switch (randomVariableOpCode) {
    case RandomVariableOpCode::None: {
        break;
    }
    case RandomVariableOpCode::Add: {
        ssaLine += argStr[0] + " + " + argStr[1] + ";";
        break;
    }
    case RandomVariableOpCode::Subtract: {
        ssaLine += argStr[0] + " - " + argStr[1] + ";";
        break;
    }
    case RandomVariableOpCode::Negative: {
        ssaLine += "-" + argStr[0] + ";";
        break;
    }
    case RandomVariableOpCode::Mult: {
        ssaLine += argStr[0] + " * " + argStr[1] + ";";
        break;
    }
    case RandomVariableOpCode::Div: {
        ssaLine += argStr[0] + " / " + argStr[1] + ";";
        break;
    }
    case RandomVariableOpCode::IndicatorEq: {
        ssaLine += "ore_indicatorEq(" + argStr[0] + "," + argStr[1] + ");";
        break;
    }
    case RandomVariableOpCode::IndicatorGt: {
        ssaLine += "ore_indicatorGt(" + argStr[0] + "," + argStr[1] + ");";
        break;
    }
    case RandomVariableOpCode::IndicatorGeq: {
        ssaLine += "ore_indicatorGeq(" + argStr[0] + "," + argStr[1] + ");";
        break;
    }
    case RandomVariableOpCode::Min: {
        ssaLine += "fmin(" + argStr[0] + "," + argStr[1] + ");";
        break;
    }
    case RandomVariableOpCode::Max: {
        ssaLine += "fmax(" + argStr[0] + "," + argStr[1] + ");";
        break;
    }
    case RandomVariableOpCode::Abs: {
        ssaLine += "fabs(" + argStr[0] + ");";
        break;
    }
    case RandomVariableOpCode::Exp: {
        ssaLine += "exp(" + argStr[0] + ");";
        break;
    }
    case RandomVariableOpCode::Sqrt: {
        ssaLine += "sqrt(" + argStr[0] + ");";
        break;
    }
    case RandomVariableOpCode::Log: {
        ssaLine += "log(" + argStr[0] + ");";
        break;
    }
    case RandomVariableOpCode::Pow: {
        ssaLine += "pow(" + argStr[0] + "," + argStr[1] + ");";
        break;
    }
    default: {
        QL_FAIL("OpenClContext::executeKernel(): no implementation for op code "
                << randomVariableOpCode << " (" << getRandomVariableOpLabels()[randomVariableOpCode] << ") provided.");
    }
    }

    // add entry to global ssa

    currentSsa_ += "  " + ssaLine + "\n";

    // update num of ops in debug info

    if (settings_.debug)
        debugInfo_.numberOfOperations += 1 * size_[currentId_ - 1];

    // return result id

    return resultId;
}

void OpenClContext::freeVariable(const std::size_t id) {
    QL_REQUIRE(currentState_ == ComputeState::calc,
               "OpenClContext::free(): not in state calc (" << static_cast<int>(currentState_) << ")");
    QL_REQUIRE(currentId_ > 0, "OpenClContext::freeVariable(): current id is not set");
    QL_REQUIRE(!hasKernel_[currentId_ - 1], "OpenClContext::freeVariable(): id ("
                                                << currentId_ << ") in version " << version_[currentId_ - 1]
                                                << " has a kernel already, variables can not be freed.");

    // we do not free input variables, only variables that were added during the calc

    if (id < inputVarOffset_.size() + nVariates_)
        return;

    freedVariables_.push_back(id);
}

void OpenClContext::declareOutputVariable(const std::size_t id) {
    QL_REQUIRE(currentState_ != ComputeState::idle, "OpenClContext::declareOutputVariable(): state is idle");
    QL_REQUIRE(currentId_ > 0, "OpenClContext::declareOutputVariable(): current id not set");
    QL_REQUIRE(!hasKernel_[currentId_ - 1], "OpenClContext::declareOutputVariable(): id ("
                                                << currentId_ << ") in version " << version_[currentId_ - 1]
                                                << " has a kernel already, output variables can not be declared.");
    outputVariables_.push_back(id);
    nOutputVars_[currentId_ - 1]++;
}

void OpenClContext::finalizeCalculation(std::vector<double*>& output) {
    struct exitGuard {
        exitGuard() {}
        ~exitGuard() {
            *currentState = ComputeState::idle;
            for (auto& m : mem)
                clReleaseMemObject(m);
        }
        ComputeState* currentState;
        std::vector<cl_mem> mem;
    } guard;

    guard.currentState = &currentState_;

    QL_REQUIRE(currentId_ > 0, "OpenClContext::finalizeCalculation(): current id is not set");
    QL_REQUIRE(output.size() == nOutputVars_[currentId_ - 1],
               "OpenClContext::finalizeCalculation(): output size ("
                   << output.size() << ") inconsistent to kernel output size (" << nOutputVars_[currentId_ - 1] << ")");
    QL_REQUIRE(!settings_.useDoublePrecision || supportsDoublePrecision(),
               "OpenClContext::finalizeCalculation(): double precision is configured for this calculation, but not "
               "supported by the device. Switch to single precision or use an appropriate device.");

    boost::timer::cpu_timer timer;
    boost::timer::nanosecond_type timerBase;

    // create input and output buffers

    std::size_t fpSize = settings_.useDoublePrecision ? sizeof(double) : sizeof(float);

    if (settings_.debug) {
        timerBase = timer.elapsed().wall;
    }

    std::size_t inputBufferSize = 0;
    if (!inputVarOffset_.empty())
        inputBufferSize = inputVarOffset_.back() + (inputVarIsScalar_.back() ? 1 : size_[currentId_ - 1]);
    cl_int err;
    cl_mem inputBuffer;
    if (inputBufferSize > 0) {
        inputBuffer = clCreateBuffer(context_, CL_MEM_READ_WRITE, fpSize * inputBufferSize, NULL, &err);
        guard.mem.push_back(inputBuffer);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::finalizeCalculation(): creating input buffer fails: " << errorText(err));
    }

    std::size_t outputBufferSize = nOutputVars_[currentId_ - 1] * size_[currentId_ - 1];
    cl_mem outputBuffer;
    if (outputBufferSize > 0) {
        outputBuffer = clCreateBuffer(context_, CL_MEM_READ_WRITE, fpSize * outputBufferSize, NULL, &err);
        guard.mem.push_back(outputBuffer);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::finalizeCalculation(): creating output buffer fails: " << errorText(err));
    }

    if (settings_.debug) {
        debugInfo_.nanoSecondsDataCopy += timer.elapsed().wall - timerBase;
    }

    // build kernel if necessary

    if (!hasKernel_[currentId_ - 1]) {

        std::string fpTypeStr = settings_.useDoublePrecision ? "double" : "float";
        std::string fpEpsStr = settings_.useDoublePrecision ? "0x1.0p-52" : "0x1.0p-23f";
        std::string fpSuffix = settings_.useDoublePrecision ? std::string() : "f";

        // clang-format off
        const std::string includeSource =
            "bool ore_closeEnough(const " + fpTypeStr + " x, const " + fpTypeStr + " y) {\n"
            "    const " + fpTypeStr + " tol = 42.0" + fpSuffix + " * " + fpEpsStr + ";\n"
            "    " + fpTypeStr + " diff = fabs(x - y);\n"
            "    if (x == 0.0" + fpSuffix + " || y == 0.0" + fpSuffix + ")\n"
            "        return diff < tol * tol;\n"
            "    return diff <= tol * fabs(x) || diff <= tol * fabs(y);\n"
            "}\n"
            "\n" +
            fpTypeStr + " ore_indicatorEq(const " + fpTypeStr + " x, const " + fpTypeStr + " y) "
                                                "{ return ore_closeEnough(x, y) ? 1.0" + fpSuffix + " : 0.0" + fpSuffix +"; }\n\n" +
            fpTypeStr + " ore_indicatorGt(const " + fpTypeStr + " x, const " + fpTypeStr + " y) " +
                                                "{ return x > y && !ore_closeEnough(x, y); }\n\n" +
            fpTypeStr + " ore_indicatorGeq(const " + fpTypeStr + " x, const " + fpTypeStr + " y) { return x > y || ore_closeEnough(x, y); }\n\n";
        // clang-format on

        std::string kernelName =
            "ore_kernel_" + std::to_string(currentId_) + "_" + std::to_string(version_[currentId_ - 1]);

        std::vector<std::string> inputArgs;
        if (inputBufferSize > 0)
            inputArgs.push_back("__global " + fpTypeStr + "* input");
        if (nVariates_ > 0)
            inputArgs.push_back("__global " + fpTypeStr + "* rn");
        if (outputBufferSize > 0)
            inputArgs.push_back("__global " + fpTypeStr + "* output");

        std::string kernelSource = includeSource + "__kernel void " + kernelName + "(" + boost::join(inputArgs, ",") +
                                   ") {\n"
                                   "unsigned int i = get_global_id(0);\n"
                                   "if(i < " +
                                   std::to_string(size_[currentId_ - 1]) + "U) {\n";

        kernelSource += currentSsa_;

        for (std::size_t i = 0; i < nOutputVars_[currentId_ - 1]; ++i) {
            std::size_t offset = i * size_[currentId_ - 1];
            std::string output;
            if (outputVariables_[i] < inputVarOffset_.size()) {
                output = "input[" + std::to_string(outputVariables_[i]) + "U" +
                         (inputVarIsScalar_[outputVariables_[i]] ? "]" : " + i] ");
            } else if (outputVariables_[i] < inputVarOffset_.size() + nVariates_) {
                output = "rn[" +
                         std::to_string((outputVariables_[i] - inputVarOffset_.size()) * size_[currentId_ - 1]) +
                         "U + i]";
            } else {
                output = "v" + std::to_string(outputVariables_[i]);
            }
            std::string ssaLine = "  output[" + std::to_string(offset) + "UL + i] = " + output + ";";
            kernelSource += ssaLine + "\n";
        }

        kernelSource += "  }\n"
                        "}\n";

        // std::cerr << "generated kernel: \n" + kernelSource + "\n";

        if (settings_.debug) {
            timerBase = timer.elapsed().wall;
        }

        cl_int err;
        const char* kernelSourcePtr = kernelSource.c_str();
        program_[currentId_ - 1] = clCreateProgramWithSource(context_, 1, &kernelSourcePtr, NULL, &err);
        QL_REQUIRE(err == CL_SUCCESS, "OpenClContext::finalizeCalculation(): error during clCreateProgramWithSource(): "
                                          << errorText(err));
        err = clBuildProgram(program_[currentId_ - 1], 1, &device_, NULL, NULL, NULL);
        if (err != CL_SUCCESS) {
            char buffer[MAX_BUILD_LOG];
            clGetProgramBuildInfo(program_[currentId_ - 1], device_, CL_PROGRAM_BUILD_LOG, MAX_BUILD_LOG * sizeof(char),
                                  buffer, NULL);
            QL_FAIL("OpenClContext::finalizeCalculation(): error during program build for kernel '"
                    << kernelName << "': " << errorText(err) << ": "
                    << std::string(buffer).substr(MAX_BUILD_LOG_LOGFILE));
        }
        kernel_[currentId_ - 1] = clCreateKernel(program_[currentId_ - 1], kernelName.c_str(), &err);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::finalizeCalculation(): error during clCreateKernel(): " << errorText(err));

        hasKernel_[currentId_ - 1] = true;
        inputBufferSize_[currentId_ - 1] = inputBufferSize;

        if (settings_.debug) {
            debugInfo_.nanoSecondsProgramBuild += timer.elapsed().wall - timerBase;
        }
    } else {
        QL_REQUIRE(inputBufferSize == inputBufferSize_[currentId_ - 1],
                   "OpenClContext::finalizeCalculation(): input buffer size ("
                       << inputBufferSize << ") inconsistent to kernel input buffer size ("
                       << inputBufferSize_[currentId_ - 1] << ")");
    }

    // write input data to input buffer (asynchronously)

    if (settings_.debug) {
        timerBase = timer.elapsed().wall;
    }

    cl_event inputBufferEvent;
    if (inputBufferSize > 0) {
        err = clEnqueueWriteBuffer(queue_, inputBuffer, CL_FALSE, 0, fpSize * inputBufferSize,
                                   settings_.useDoublePrecision ? (void*)&inputVarValues64_[0]
                                                                : (void*)&inputVarValues32_[0],
                                   0, NULL, &inputBufferEvent);
        QL_REQUIRE(err == CL_SUCCESS,
                   "OpenClContext::finalizeCalculation(): writing to input buffer fails: " << errorText(err));
    }

    if (settings_.debug) {
        err = clFinish(queue_);
        QL_REQUIRE(err == CL_SUCCESS, "OpenClContext::clFinish(): error in debug mode: " << errorText(err));
        debugInfo_.nanoSecondsDataCopy += timer.elapsed().wall - timerBase;
    }

    // set kernel args

    std::size_t kidx = 0;
    err = 0;
    if (inputBufferSize > 0) {
        err |= clSetKernelArg(kernel_[currentId_ - 1], kidx++, sizeof(cl_mem), &inputBuffer);
    }
    if (nVariates_ > 0) {
        err |= clSetKernelArg(kernel_[currentId_ - 1], kidx++, sizeof(cl_mem), &variatesPool_);
    }
    if (outputBufferSize > 0) {
        err |= clSetKernelArg(kernel_[currentId_ - 1], kidx++, sizeof(cl_mem), &outputBuffer);
    }
    QL_REQUIRE(err == CL_SUCCESS, "OpenClContext::finalizeCalculation(): set kernel args fails: " << errorText(err));

    // execute kernel

    if (settings_.debug) {
        err = clFinish(queue_);
        timerBase = timer.elapsed().wall;
    }

    std::vector<cl_event> runWaitEvents;
    if (inputBufferSize > 0)
        runWaitEvents.push_back(inputBufferEvent);

    cl_event runEvent;
    err = clEnqueueNDRangeKernel(queue_, kernel_[currentId_ - 1], 1, NULL, &size_[currentId_ - 1], NULL,
                                 runWaitEvents.size(), runWaitEvents.empty() ? NULL : &runWaitEvents[0], &runEvent);
    QL_REQUIRE(err == CL_SUCCESS, "OpenClContext::finalizeCalculation(): enqueue kernel fails: " << errorText(err));

    if (settings_.debug) {
        err = clFinish(queue_);
        QL_REQUIRE(err == CL_SUCCESS, "OpenClContext::clFinish(): error in debug mode: " << errorText(err));
        debugInfo_.nanoSecondsCalculation += timer.elapsed().wall - timerBase;
    }

    // copy the results (asynchronously)

    if (settings_.debug) {
        timerBase = timer.elapsed().wall;
    }

    std::vector<cl_event> outputBufferEvents;
    if (outputBufferSize > 0) {
        std::vector<std::vector<float>> outputFloat;
        if (!settings_.useDoublePrecision) {
            outputFloat.resize(output.size(), std::vector<float>(size_[currentId_ - 1]));
        }
        for (std::size_t i = 0; i < output.size(); ++i) {
            outputBufferEvents.push_back(cl_event());
            err = clEnqueueReadBuffer(queue_, outputBuffer, CL_FALSE, fpSize * i * size_[currentId_ - 1],
                                      fpSize * size_[currentId_ - 1],
                                      settings_.useDoublePrecision ? (void*)&output[i][0] : (void*)&outputFloat[i][0],
                                      1, &runEvent, &outputBufferEvents.back());
            QL_REQUIRE(err == CL_SUCCESS,
                       "OpenClContext::finalizeCalculation(): writing to output buffer fails: " << errorText(err));
        }
        err = clWaitForEvents(outputBufferEvents.size(), outputBufferEvents.empty() ? nullptr : &outputBufferEvents[0]);
        QL_REQUIRE(
            err == CL_SUCCESS,
            "OpenClContext::finalizeCalculation(): wait for output buffer events to finish fails: " << errorText(err));
        if (!settings_.useDoublePrecision) {
            // copy from float to double
            for (std::size_t i = 0; i < output.size(); ++i) {
                std::copy(outputFloat[i].begin(), outputFloat[i].end(), output[i]);
            }
        }
    }

    if (settings_.debug) {
        err = clFinish(queue_);
        QL_REQUIRE(err == CL_SUCCESS, "OpenClContext::clFinish(): error in debug mode: " << errorText(err));
        debugInfo_.nanoSecondsDataCopy += timer.elapsed().wall - timerBase;
    }
}

const ComputeContext::DebugInfo& OpenClContext::debugInfo() const { return debugInfo_; }

std::vector<std::pair<std::string, std::string>> OpenClContext::deviceInfo() const { return deviceInfo_; }
bool OpenClContext::supportsDoublePrecision() const { return supportsDoublePrecision_; }

#endif

#ifndef ORE_ENABLE_OPENCL
OpenClFramework::OpenClFramework() {}
OpenClFramework::~OpenClFramework() {}
#endif

std::set<std::string> OpenClFramework::getAvailableDevices() const {
    std::set<std::string> tmp;
    for (auto const& [name, _] : contexts_)
        tmp.insert(name);
    return tmp;
}

ComputeContext* OpenClFramework::getContext(const std::string& deviceName) {
    auto c = contexts_.find(deviceName);
    if (c != contexts_.end()) {
        return c->second;
    }
    QL_FAIL("OpenClFrameWork::getContext(): device '"
            << deviceName << "' not found. Available devices: " << boost::join(getAvailableDevices(), ","));
}
}; // namespace QuantExt
