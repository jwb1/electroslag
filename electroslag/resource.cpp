//  Electroslag Interactive Graphics System
//  Copyright 2018 Joshua Buckman
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#include "electroslag/precomp.hpp"
#include "electroslag/resource.hpp"
#include "electroslag/resource_id.hpp"

namespace electroslag {
    referenced_buffer_interface::ref resource::create(unsigned int id)
    {
        HINSTANCE hinstance = GetModuleHandle(0);

        // Find the specified resource.
        HRSRC res = FindResourceEx(
            hinstance,
            MAKEINTRESOURCE(DATA_RESOURCE_TYPE),
            MAKEINTRESOURCE(id),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)
            );
        if (!res) {
            return (ref(0));
        }

        // Load it.
        HGLOBAL global = LoadResource(hinstance, res);
        if (!global) {
            throw win32_api_failure("LoadResource");
        }

        // Get the resource size.
        int size = SizeofResource(hinstance, res);
        if (size <= 0) {
            throw win32_api_failure("SizeofResource");
        }

        // Get a pointer to the resource.
        byte* pointer = static_cast<byte*>(LockResource(global));
        if (!pointer) {
            throw win32_api_failure("LockResource");
        }

        // Wrap a referenced buffer around the resource.
        return (ref(new resource(pointer, size)));
    }
}
