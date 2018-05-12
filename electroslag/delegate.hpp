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
    template<class ReturnType, class... Params>
    class delegate {
    private:
        typedef ReturnType(*stub_pointer)(void* object, Params... params);

        struct delegate_fields {
            delegate_fields(void* new_object, stub_pointer new_stub)
                : object(new_object)
                , stub(new_stub)
            {}

            void* object;
            stub_pointer const stub;
        };

    public:
        typedef byte delegate_storage[sizeof(delegate_fields)];

        template<ReturnType(*Function)(Params... params)>
        static delegate* emplace_from_function(
            delegate_storage* storage
            )
        {
            return (new (storage) delegate(0, &function_stub<Function>));
        }

        template<class T, ReturnType(T::*TMethod)(Params... params)>
        static delegate* emplace_from_method(
            delegate_storage* storage,
            T* object
            )
        {
            return (new (storage) delegate(object, &method_stub<T, TMethod>));
        }

        template<class T, ReturnType(T::*TMethod)(Params... params) const>
        static delegate* emplace_from_const_method(
            delegate_storage* storage,
            T const* object
            )
        {
            return (new (storage) delegate(const_cast<T*>(object), &const_method_stub<T, TMethod>));
        }

        template<ReturnType(*Function)(Params... params)>
        static delegate* create_from_function()
        {
            return (new delegate(0, &function_stub<Function>));
        }

        template<class T, ReturnType(T::*TMethod)(Params... params)>
        static delegate* create_from_method(T* object)
        {
            return (new delegate(object, &method_stub<T, TMethod>));
        }

        template<class T, ReturnType(T::*TMethod)(Params... params) const>
        static delegate* create_from_const_method(T const* object)
        {
            return (new delegate(const_cast<T*>(object), &const_method_stub<T, TMethod>));
        }

        ReturnType invoke(Params... params) const
        {
            return ((*m_fields.stub)(m_fields.object, params...));
        }

        ReturnType operator ()(Params... params) const
        {
            return ((*m_fields.stub)(m_fields.object, params...));
        }

    private:
        delegate(void* object, stub_pointer stub)
            : m_fields(object, stub)
        {}

        template<ReturnType(*Function)(Params... params)>
        static ReturnType function_stub(void*, Params... params)
        {
            return ((Function)(params...));
        }

        template<class T, ReturnType(T::*TMethod)(Params... params)>
        static ReturnType method_stub(void* object, Params... params)
        {
            T* typed_object = reinterpret_cast<T*>(object);
            return ((typed_object->*TMethod)(params...));
        }

        template<class T, ReturnType(T::*TMethod)(Params... params) const>
        static ReturnType const_method_stub(void* object, Params... params)
        {
            T const* typed_object = reinterpret_cast<T const*>(object);
            return ((typed_object->*TMethod)(params...));
        }

        delegate_fields m_fields;

        // Disallowed operations:
        delegate();
        explicit delegate(delegate const&);
        delegate& operator =(delegate const&);
    };
}
