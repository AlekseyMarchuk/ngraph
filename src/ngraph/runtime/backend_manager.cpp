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
#include <sstream>

#include "ngraph/file_util.hpp"
#include "ngraph/runtime/backend_manager.hpp"
#include "ngraph/util.hpp"

using namespace std;
using namespace ngraph;

unordered_map<string, runtime::new_backend_t>& runtime::BackendManager::get_registry()
{
    static unordered_map<string, new_backend_t> s_registered_backend;
    return s_registered_backend;
}

void runtime::BackendManager::register_backend(const string& name, new_backend_t new_backend)
{
    get_registry()[name] = new_backend;
}

vector<string> runtime::BackendManager::get_registered_backends()
{
    vector<string> rc;
    for (const auto& p : get_registry())
    {
        rc.push_back(p.first);
    }
    return rc;
}

shared_ptr<runtime::Backend> runtime::BackendManager::create_backend(const std::string& config)
{
    shared_ptr<runtime::Backend> rc;
    string type = config;

    // strip off attributes, IE:CPU becomes IE
    auto colon = type.find(":");
    if (colon != type.npos)
    {
        type = type.substr(0, colon);
    }

    auto registry = get_registry();
    auto it = registry.find(type);
    if (it == registry.end())
    {
        stringstream ss;
        ss << "Backend '" << type << "' not registered";
        throw runtime_error(ss.str());
    }
    new_backend_t new_backend = it->second;
    rc = new_backend(config);
    return rc;
}

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

map<string, string> runtime::BackendManager::get_registered_device_map()
{
    map<string, string> rc;
    string my_directory = file_util::get_directory(find_my_file());
    vector<string> backend_list;

    auto f = [&](const string& file, bool is_dir) {
        string name = file_util::get_file_name(file);
        string backend_name;
        if (is_backend_name(name, backend_name))
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
                        rc.insert({to_upper(backend_name), file});
                    }
                }

                dlclose(handle);
            }
        }
    };
    file_util::iterate_files(my_directory, f, false, true);
    return rc;
}

bool runtime::BackendManager::is_backend_name(const string& file, string& backend_name)
{
    string name = file_util::get_file_name(file);
    string ext = SHARED_LIB_EXT;
    bool rc = false;
    if (!name.compare(0, 3, "lib"))
    {
        if (!name.compare(name.size() - ext.size(), ext.size(), ext))
        {
            auto pos = name.find("_backend");
            if (pos != name.npos)
            {
                backend_name = name.substr(3, pos - 3);
                rc = true;
            }
        }
    }
    return rc;
}
