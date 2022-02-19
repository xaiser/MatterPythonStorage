
/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "controller/python/ChipDeviceController-StorageDelegate.h"

#include <cstring>
#include <map>
#include <string>
#include <fstream>
#include <memory>

#include <lib/core/CHIPEncoding.h>
#include <lib/core/CHIPPersistentStorageDelegate.h>
#include <lib/support/logging/CHIPLogging.h>
#include <lib/support/Base64.h>

using String   = std::basic_string<char>;
using Section  = std::map<String, String>;
using Sections = std::map<String, Section>;

namespace {

std::string StringToBase64(const std::string & value)
{
    std::unique_ptr<char[]> buffer(new char[BASE64_ENCODED_LEN(value.length())]);

    uint32_t len =
        chip::Base64Encode32(reinterpret_cast<const uint8_t *>(value.data()), static_cast<uint32_t>(value.length()), buffer.get());
    if (len == UINT32_MAX)
    {
        return "";
    }

    return std::string(buffer.get(), len);
}

std::string Base64ToString(const std::string & b64Value)
{
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[BASE64_MAX_DECODED_LEN(b64Value.length())]);

    uint32_t len = chip::Base64Decode32(b64Value.data(), static_cast<uint32_t>(b64Value.length()), buffer.get());
    if (len == UINT32_MAX)
    {
        return "";
    }

    return std::string(reinterpret_cast<const char *>(buffer.get()), len);
}

} // namespace

namespace chip {
namespace Controller {

//constexpr const char kStorageFile = "/tmp/chip_ctrl"
const char * kFilename = "/Users/x/CS/project/PersonalWeatherSensor/chip_ctrl_test.ini";
const char * kStorageFile_tmp = "/Users/x/CS/project/PersonalWeatherSensor/chip_ctrl_test_tmp";
constexpr const char kDefaultSectionName[] = "Default";

CHIP_ERROR PythonPersistentStorageDelegate::Init()
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    std::ifstream ifs;
    ifs.open(kFilename, std::ifstream::in);
    if (!ifs.good())
    {
        CommitConfig();
        ifs.open(kFilename, std::ifstream::in);
    }
	if ( false == ifs.is_open() )
	{
		return CHIP_ERROR_OPEN_FAILED;
	}

    mConfig.parse(ifs);
    ifs.close();

//exit:
    return err;
}

CHIP_ERROR PythonPersistentStorageDelegate::SyncGetKeyValue(const char * key, void * value, uint16_t & size)
{
    std::string iniValue;

    auto section = mConfig.sections[kDefaultSectionName];
    auto it      = section.find(key);
    ReturnErrorCodeIf(it == section.end(), CHIP_ERROR_KEY_NOT_FOUND);

    ReturnErrorCodeIf(!inipp::extract(section[key], iniValue), CHIP_ERROR_INVALID_ARGUMENT);

    iniValue = Base64ToString(iniValue);

    uint16_t dataSize = static_cast<uint16_t>(iniValue.size());
    if (dataSize > size)
    {
        size = dataSize;
        return CHIP_ERROR_BUFFER_TOO_SMALL;
    }

    size = dataSize;
    memcpy(value, iniValue.data(), dataSize);

    return CHIP_NO_ERROR;
}

CHIP_ERROR PythonPersistentStorageDelegate::SyncSetKeyValue(const char * key, const void * value, uint16_t size)
{
    auto section = mConfig.sections[kDefaultSectionName];
    section[key] = StringToBase64(std::string(static_cast<const char *>(value), size));

    mConfig.sections[kDefaultSectionName] = section;
    return CommitConfig();
}

CHIP_ERROR PythonPersistentStorageDelegate::SyncDeleteKeyValue(const char * key)
{
    auto section = mConfig.sections[kDefaultSectionName];
    section.erase(key);

    mConfig.sections[kDefaultSectionName] = section;
    return CommitConfig();
}

CHIP_ERROR PythonPersistentStorageDelegate::CommitConfig()
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    std::ofstream ofs;
    std::string tmpPath = kFilename;
    tmpPath.append(".tmp");
    ofs.open(tmpPath, std::ofstream::out | std::ofstream::trunc);
    VerifyOrExit(ofs.good(), err = CHIP_ERROR_WRITE_FAILED);

    mConfig.generate(ofs);
    ofs.close();
    VerifyOrExit(ofs.good(), err = CHIP_ERROR_WRITE_FAILED);

    VerifyOrExit(rename(tmpPath.c_str(), kFilename) == 0, err = CHIP_ERROR_WRITE_FAILED);

exit:
    return err;
}

} // namespace Controller
} // namespace chip
