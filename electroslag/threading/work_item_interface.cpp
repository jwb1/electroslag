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
#include "electroslag/threading/work_item_interface.hpp"
#include "electroslag/threading/worker_thread.hpp"

namespace electroslag {
    namespace threading {
        // Avoid the circular header dependency.
        void work_item_interface::wait_for_done() const
        {
            m_worker->wait_for_work_done(this);
        }
    }
}
