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
#include "electroslag/graphics/double_buffer_queue.hpp"

namespace electroslag {
    namespace graphics {
        // 320 bytes is a heuristic pick. Seems like a good choice.
        int const double_buffer_queue::initial_buffer_bytes = 320;
        // Max out at a 32K per side; storing more than this shouldn't be necessary.
        int const double_buffer_queue::max_buffer_bytes = 32 * 1024;
        // Golden ratio (phi) might be easier on the allocator.
        float const double_buffer_queue::buffer_growth_factor = 1.61803399f;

        double_buffer_queue::double_buffer_queue()
        {
            // Allocate a buffer to start enqueing in.
            m_enqueue_buffer.begin = static_cast<byte*>(std::malloc(initial_buffer_bytes));
            if (!m_enqueue_buffer.begin) {
                throw std::bad_alloc();
            }
            m_enqueue_buffer.next = m_enqueue_buffer.begin;
            m_enqueue_buffer.end = m_enqueue_buffer.begin + initial_buffer_bytes;

            // Allocate a pair of the same size. The buffer will start off empty
            // as there is nothing to dequeue.
            m_dequeue_buffer.begin = static_cast<byte*>(std::malloc(initial_buffer_bytes));
            if (!m_dequeue_buffer.begin) {
                throw std::bad_alloc();
            }
            m_dequeue_buffer.next = m_dequeue_buffer.begin;
            m_dequeue_buffer.last = m_dequeue_buffer.begin;
            m_dequeue_buffer.end = m_dequeue_buffer.begin + initial_buffer_bytes;
        }

        double_buffer_queue::~double_buffer_queue()
        {
            if (m_dequeue_buffer.begin) {
                std::free(m_dequeue_buffer.begin);
                m_dequeue_buffer.begin = 0;
                m_dequeue_buffer.end = 0;
                m_dequeue_buffer.next = 0;
                m_dequeue_buffer.last = 0;
            }

            if (m_enqueue_buffer.begin) {
                std::free(m_enqueue_buffer.begin);
                m_enqueue_buffer.begin = 0;
                m_enqueue_buffer.end = 0;
                m_enqueue_buffer.next = 0;
            }
        }

        void* double_buffer_queue::enqueue(int bytes, int alignment)
        {
            ELECTROSLAG_CHECK(bytes < max_buffer_bytes && bytes > 0);
            ELECTROSLAG_CHECK(is_aligned(m_enqueue_buffer.next, alignof(enqueue_header)));

            byte* next_enqueue = m_enqueue_buffer.next;
            byte* unaligned_start = next_enqueue + sizeof(enqueue_header);
            byte* aligned_start = align_up(unaligned_start, alignment);
            byte* after_enqueue = align_up(aligned_start + bytes, alignof(enqueue_header));

            if (after_enqueue > m_enqueue_buffer.end) {
                // There is not enough space in the enqueue buffer, so it has to be grown.
                int current_size = static_cast<int>(m_enqueue_buffer.end - m_enqueue_buffer.begin);
                int current_offset = static_cast<int>(m_enqueue_buffer.next - m_enqueue_buffer.begin);
                int new_size = max(
                    static_cast<int>(current_size * buffer_growth_factor),
                    static_cast<int>(current_size + (after_enqueue - m_enqueue_buffer.next))
                    );
                if (new_size > max_buffer_bytes) {
                    throw std::runtime_error("enqueue buffer overflow");
                }

                m_enqueue_buffer.begin = reinterpret_cast<byte*>(realloc(m_enqueue_buffer.begin, new_size));
                if (!m_enqueue_buffer.begin) {
                    throw std::bad_alloc();
                }
                m_enqueue_buffer.end = m_enqueue_buffer.begin + new_size;

                // Now, proceed with the enqueue in the newly allocated memory.
                next_enqueue = m_enqueue_buffer.begin + current_offset;
                unaligned_start = next_enqueue + sizeof(enqueue_header);
                aligned_start = align_up(unaligned_start, alignment);
                after_enqueue = align_up(aligned_start + bytes, alignof(enqueue_header));
            }

            enqueue_header* header = reinterpret_cast<enqueue_header*>(next_enqueue);
            header->size = after_enqueue - next_enqueue;
            header->pad_to_align = aligned_start - unaligned_start;

            m_enqueue_buffer.next = after_enqueue;

            return (aligned_start);
        }

        void* double_buffer_queue::dequeue()
        {
            if (m_dequeue_buffer.next < m_dequeue_buffer.last) {
                enqueue_header* dequeue_header = reinterpret_cast<enqueue_header*>(m_dequeue_buffer.next);
                byte* dequeue_pointer = m_dequeue_buffer.next + sizeof(enqueue_header);

                m_dequeue_buffer.next = dequeue_pointer + dequeue_header->size;
                return (dequeue_pointer + dequeue_header->pad_to_align);
            }
            else {
                return (0);
            }
        }

        void double_buffer_queue::swap()
        {
            std::swap(m_enqueue_buffer.begin, m_dequeue_buffer.begin);
            std::swap(m_enqueue_buffer.end, m_dequeue_buffer.end);

            m_dequeue_buffer.next = m_dequeue_buffer.begin;
            m_dequeue_buffer.last = m_enqueue_buffer.next;

            m_enqueue_buffer.next = m_enqueue_buffer.begin;
        }
    }
}
