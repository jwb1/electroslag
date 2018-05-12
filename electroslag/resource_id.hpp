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

#pragma once

// This header is shared between the windows resource compiler, and the library
// source code that needs to load from the windows resource section, which
// include it directly.

// The data resource type constant and ids are hard coded BY VALUE in
// build_content.bat

// Data files use a user-defined resource type.
#define DATA_RESOURCE_TYPE 512

// Application icon = offset 2000
#define IDI_APP_WINDOW 2000

// Data files = offset 9000
#define ID_LOADING_SCREEN 9001
