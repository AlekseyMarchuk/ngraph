/*******************************************************************************
* Copyright 2017-2018 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <dlfcn.h>
#include <regex>
#include <sstream>

#include "ngraph/file_util.hpp"
#include "ngraph/runtime/backend.hpp"
#include "ngraph/runtime/backend_manager.hpp"
#include "ngraph/util.hpp"

using namespace std;
using namespace ngraph;

// This doodad finds the full path of the containing shared library
static string find_my_file()
{
    Dl_info dl_info;
    dladdr(reinterpret_cast<void*>(find_my_file), &dl_info);
    return dl_info.dli_fname;
}

void* runtime::BackendManager::open_shared_library(string type)
{
    string ext = SHARED_LIB_EXT;

    void* handle = nullptr;

    // strip off attributes, IE:CPU becomes IE
    auto colon = type.find(":");
    if (colon != type.npos)
    {
        type = type.substr(0, colon);
    }

    string library_name = "lib" + to_lower(type) + "_backend" + string(SHARED_LIB_EXT);
    string my_directory = file_util::get_directory(find_my_file());
    string library_path = file_util::path_join(my_directory, library_name);
    handle = dlopen(library_path.c_str(), RTLD_NOW | RTLD_GLOBAL);

    return handle;
}

shared_ptr<runtime::Backend> runtime::BackendManager::create(const string& type)
{
    shared_ptr<runtime::Backend> rc;
    void* handle = open_shared_library(type);
    if (!handle)
    {
        throw runtime_error("Backend '" + type + "' not found");
    }
    else
    {
        function<const char*()> get_ngraph_version_string =
            reinterpret_cast<const char* (*)()>(dlsym(handle, "get_ngraph_version_string"));
        if (!get_ngraph_version_string)
        {
            dlclose(handle);
            throw runtime_error("Backend '" + type +
                                "' does not implement get_ngraph_version_string");
        }

        function<runtime::Backend*(const char*)> new_backend =
            reinterpret_cast<runtime::Backend* (*)(const char*)>(dlsym(handle, "new_backend"));
        if (!new_backend)
        {
            dlclose(handle);
            throw runtime_error("Backend '" + type + "' does not implement new_backend");
        }

        function<void(runtime::Backend*)> delete_backend =
            reinterpret_cast<void (*)(runtime::Backend*)>(dlsym(handle, "delete_backend"));
        if (!delete_backend)
        {
            dlclose(handle);
            throw runtime_error("Backend '" + type + "' does not implement delete_backend");
        }

        runtime::Backend* backend = new_backend(type.c_str());
        rc = shared_ptr<runtime::Backend>(backend, [=](runtime::Backend* b) {
            delete_backend(b);
            // dlclose(handle);
        });
    }
    return rc;
}

map<string, string> runtime::BackendManager::get_registered_device_map()
{
    map<string, string> rc;
    string my_directory = file_util::get_directory(find_my_file());
    vector<string> backend_list;
    regex reg("^lib(.+)_backend" + string(SHARED_LIB_EXT));
    smatch result;

    auto f = [&](const string& file, bool is_dir) {
        string name = file_util::get_file_name(file);
        if (regex_match(name, result, reg))
        {
            auto handle = dlopen(file.c_str(), RTLD_LAZY | RTLD_LOCAL);
            if (handle)
            {
                if (dlsym(handle, "new_backend") && dlsym(handle, "delete_backend"))
                {
                    function<const char*()> get_ngraph_version_string =
                        reinterpret_cast<const char* (*)()>(
                            dlsym(handle, "get_ngraph_version_string"));
                    if (get_ngraph_version_string &&
                        get_ngraph_version_string() == string(NGRAPH_VERSION))
                    {
                        rc.insert({to_upper(result[1]), file});
                    }
                }

                dlclose(handle);
            }
        }
    };
    file_util::iterate_files(my_directory, f, false, true);
    return rc;
}

vector<string> runtime::BackendManager::get_registered_devices()
{
    map<string, string> m = get_registered_device_map();
    vector<string> rc;
    for (const pair<string, string>& p : m)
    {
        rc.push_back(p.first);
    }
    return rc;
}