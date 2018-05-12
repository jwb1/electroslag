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

namespace electroslag {
    namespace graphics {
        // This data structure is tuned for a particular use: a single producer
        // thread, a single consumer thread, variable sized data blocks, and
        // an external synchronization mechanism. The external mechanism must:
        //  1. Stop the consumer and producer threads.
        //  2. Call "swap" to move the produced data to the consumer.
        //  3. Restart consumer and producer threads.
        class double_buffer_queue {
        public:
            static int const initial_buffer_bytes;
            static int const max_buffer_bytes;
            static float const buffer_growth_factor; // ~phi, aka the golden ratio

            double_buffer_queue();
            ~double_buffer_queue(); // not virtual; don't expect classes derived from this.

            void* enqueue(int bytes, int alignment);
            void* dequeue();

            void swap();

        private:
            // Each data enqueue is prefixed by the number of bytes in the data blob.
            struct enqueue_header {
                unsigned int size:24;
                unsigned int pad_to_align:8;
            };

            // These pointers are used by the enqueue thread; the boundaries of the buffer
            // enqueue is placed in, and where the next enqueue will start.
            struct enqueue_buffer {
                enqueue_buffer()
                    : begin(0)
                    , end(0)
                    , next(0)
                {}

                byte* begin; // Buffer boundary: low address
                byte* end;   // Buffer boundary: high address
                byte* next;  // Where to enqueue next; moving low to high
            } m_enqueue_buffer;

            // These pointer are used by the dequeue thread; the boundaries of the buffer
            // dequeues come from, where the next dequeue will start, and boundary of valid
            // data within the buffer (might not have filled it up to the end.)
            struct dequeue_buffer {
                dequeue_buffer()
                    : begin(0)
                    , end(0)
                    , next(0)
                    , last(0)
                {}

                byte* begin; // Buffer boundary: low address
                byte* end;   // Buffer boundary: high address
                byte* next;  // Where to dequeue next; moving low to last
                byte* last;  // The end of valid enqueued data.
            } m_dequeue_buffer;

            // Disallowed operations:
            explicit double_buffer_queue(double_buffer_queue const&);
            double_buffer_queue& operator =(double_buffer_queue const&);
        };
    }
}
