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
#include "electroslag/application/application.hpp"

int main(int argc, char** argv)
{
    int return_value = EXIT_SUCCESS;
    try {
        electroslag::application::application app(argc, argv);
        return_value = app.run();
    }
    catch (std::logic_error const& e) {
        std::printf("Logic error exception caught: %s\n", e.what());
        return_value = EXIT_FAILURE;
    }
    catch (std::runtime_error const& e) {
        std::printf("Runtime error exception caught: %s\n", e.what());
        return_value = EXIT_FAILURE;
    }
    catch (...) {
        std::printf("Unknown exception caught!\n");
        return_value = EXIT_FAILURE;
    }

    return (return_value);
}
