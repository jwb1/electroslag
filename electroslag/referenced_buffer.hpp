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
#include "electroslag/logger.hpp"

namespace electroslag {
    // A referenced_buffer is a class that encapsulates a memory block that
    // may be safely shared between threads. Lifetime is ensured via
    // reference counting, and a RAII accessor class defines the scopes
    // where the data may be accessed, and thus the necessary locking.
    class referenced_buffer_interface : public referenced_object {
    public:
        typedef reference<referenced_buffer_interface> ref;

        class accessor {
        public:
            explicit accessor(referenced_buffer_interface::ref const& buffer_ref)
                : m_buffer_ref(buffer_ref)
                , m_pointer(0)
            {
                if (m_buffer_ref.is_valid()) {
                    m_pointer = m_buffer_ref->lock();
                }
            }

            ~accessor()
            {
                if (m_pointer) {
                    m_buffer_ref->unlock();
                    m_pointer = 0;
                }
            }

            void const* get_pointer() const
            {
                return (m_pointer);
            }

            void* get_pointer()
            {
                return (m_pointer);
            }

            int get_sizeof() const
            {
                return (m_buffer_ref->get_sizeof());
            }
        private:
            referenced_buffer_interface::ref m_buffer_ref;
            void* m_pointer;

            // Disallowed operations:
            accessor();
            explicit accessor(accessor const&);
            accessor& operator =(accessor const&);
        };

        // The base class relies on derived classes to provide the policy for accessing the buffer.
        virtual void* lock() = 0;
        virtual void unlock() = 0;
        virtual int get_sizeof() const = 0;
    };

    class referenced_buffer : public referenced_buffer_interface {
    public:
        virtual void* lock()
        {
            buffer_lock_state expected_state = buffer_lock_state_unlocked;
            // memory_order_seq_cst is the order default on atomic operation.
            if (m_lock_state.compare_exchange_strong(expected_state, buffer_lock_state_locked)) {
                return (m_buffer);
            }
            else {
                return (0);
            }
        }

        virtual void unlock()
        {
            buffer_lock_state expected_state = buffer_lock_state_locked;
            // memory_order_seq_cst is the order default on atomic operation.
            m_lock_state.compare_exchange_strong(expected_state, buffer_lock_state_unlocked);
        }

        virtual int get_sizeof() const
        {
            return (m_sizeof);
        }

    protected:
        referenced_buffer()
            : m_buffer(0)
            , m_sizeof(0)
            , m_lock_state(buffer_lock_state_unlocked)
        {}

        referenced_buffer(byte* buffer, int buffer_sizeof)
            : m_buffer(buffer)
            , m_sizeof(buffer_sizeof)
            , m_lock_state(buffer_lock_state_unlocked)
        {}

        virtual ~referenced_buffer()
        {
            // memory_order_seq_cst is the order default on atomic operation.
            if (m_lock_state.load() != buffer_lock_state_unlocked) {
                ELECTROSLAG_LOG_ERROR("referenced_buffer deleted while still locked.");
            }
        }

        byte* m_buffer;
        int m_sizeof;

        // TODO: const specializations to avoid the lock?
        enum buffer_lock_state {
            buffer_lock_state_unlocked,
            buffer_lock_state_locked
        };
        std::atomic<buffer_lock_state> m_lock_state;

    private:
        // Disallowed operations:
        explicit referenced_buffer(referenced_buffer const&);
        referenced_buffer& operator =(referenced_buffer const&);
    };

    class referenced_buffer_from_ptr : public referenced_buffer {
    public:
        typedef void (*delete_fn_ptr)(void*);

        static ref create(void* buffer, int buffer_sizeof, delete_fn_ptr delete_fn = 0)
        {
            return (ref(new referenced_buffer_from_ptr(
                static_cast<byte*>(buffer),
                buffer_sizeof,
                delete_fn
                )));
        }

    protected:
        referenced_buffer_from_ptr(
            byte* buffer,
            int buffer_sizeof,
            delete_fn_ptr delete_fn
            )
            : referenced_buffer(buffer, buffer_sizeof)
            , m_delete_fn(delete_fn)
        {}

        virtual ~referenced_buffer_from_ptr()
        {
            if (m_delete_fn) {
                if (!referenced_buffer::m_buffer) {
                    ELECTROSLAG_LOG_ERROR("referenced_buffer_from_ptr deleting 0 pointer.");
                }
                m_delete_fn(referenced_buffer::m_buffer);
                m_delete_fn = 0;
            }
        }

    private:
        delete_fn_ptr m_delete_fn;

        // Disallowed operations:
        referenced_buffer_from_ptr();
        explicit referenced_buffer_from_ptr(referenced_buffer_from_ptr const&);
        referenced_buffer_from_ptr& operator =(referenced_buffer_from_ptr const&);
    };

    class referenced_buffer_from_sizeof : public referenced_buffer {
    public:
        static ref create(int buffer_sizeof)
        {
            if (buffer_sizeof <= 0) {
                throw parameter_failure("buffer_sizeof");
            }

            byte* buffer = new byte[buffer_sizeof];

            return (ref(new referenced_buffer_from_sizeof(buffer, buffer_sizeof)));
        }

    protected:
        referenced_buffer_from_sizeof(
            byte* buffer,
            int buffer_sizeof
            )
            : referenced_buffer(buffer, buffer_sizeof)
        {}

        virtual ~referenced_buffer_from_sizeof()
        {
            delete[] m_buffer;
        }

    private:
        // Disallowed operations:
        referenced_buffer_from_sizeof();
        explicit referenced_buffer_from_sizeof(referenced_buffer_from_sizeof const&);
        referenced_buffer_from_sizeof& operator =(referenced_buffer_from_sizeof const&);
    };

    class derived_referenced_buffer : public referenced_buffer_interface {
    public:
        static ref create(ref const& parent_buffer, int offset, int sizeof_buffer)
        {
            return (ref(new derived_referenced_buffer(parent_buffer, offset, sizeof_buffer)));
        }

        virtual void* lock()
        {
            byte* parent_pointer = static_cast<byte*>(m_parent_buffer->lock());
            if (parent_pointer) {
                return (parent_pointer + m_offset);
            }
            else {
                return (0);
            }
        }

        virtual void unlock()
        {
            m_parent_buffer->unlock();
        }

        virtual int get_sizeof() const
        {
            return (m_sizeof);
        }

    protected:
        derived_referenced_buffer(ref const& parent_buffer, int offset, int sizeof_buffer)
            : m_parent_buffer(parent_buffer)
            , m_offset(offset)
            , m_sizeof(sizeof_buffer)
        {
            int parent_sizeof = parent_buffer->get_sizeof();
            if (offset < 0 || sizeof_buffer <= 0 || offset > parent_sizeof || offset + sizeof_buffer > parent_sizeof) {
                throw parameter_failure("invalid offset and count");
            }
        }
        derived_referenced_buffer(); // Not implemented

        virtual ~derived_referenced_buffer()
        {}

    private:
        ref m_parent_buffer;
        int m_offset;
        int m_sizeof;
    };

    class cloned_referenced_buffer : public referenced_buffer {
    public:
        static ref create(referenced_buffer_interface::ref const& clone_object)
        {
            referenced_buffer_interface::accessor clone_buffer(clone_object);

            int buffer_sizeof = clone_buffer.get_sizeof();
            byte* buffer = new byte[buffer_sizeof];
            memcpy(buffer, clone_buffer.get_pointer(), buffer_sizeof);

            return (ref(new cloned_referenced_buffer(buffer, buffer_sizeof)));
        }

    protected:
        cloned_referenced_buffer(
            byte* buffer,
            int buffer_sizeof
            )
            : referenced_buffer(buffer, buffer_sizeof)
        {}

        virtual ~cloned_referenced_buffer()
        {
            delete[] m_buffer;
        }

    private:
        // Disallowed operations:
        cloned_referenced_buffer();
        explicit cloned_referenced_buffer(cloned_referenced_buffer const&);
        cloned_referenced_buffer& operator =(cloned_referenced_buffer const&);
    };
}
